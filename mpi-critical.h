#include <mpi.h>

#include "ipc-data.h"
#ifndef MPI_CRITICAL_H
#define MPI_CRITICAL_H 1

#define NUM_UC_TIMERS 4
#define REF_RANK 0

#include "criticalPath.h"
#include "tracking.h"

void MpiHappensAfter(ipcData *uc, int remote = 0);
void MpiHappensAfter(ipcData &uc, int remote = 0);
ipcMetric *MpiHappensBefore(ipcData *uc, int remote = -1);
ipcMetric *MpiHappensBefore(ipcData &uc, int remote = -1);

void completePBWC(RequestData *uc, MPI_Status *status);
void completePBHB(RequestData *uc, MPI_Status *status);
void completePersPBHB(RequestData *uc, MPI_Status *status);
void startPersPBHB(RequestData *uc);

void cancelPB(RequestData *uc);

void completePBnoHB(RequestData *uc, MPI_Status *status);
void completePersPBnoHB(RequestData *uc, MPI_Status *status);
void startPersPBnoHB(RequestData *uc);

struct mpiTimer;
void init_processes(mpiTimer &mt);

/*
 * RAII class for timing MPI functions
 */
struct mpiTimer {
  const char *loc;
  mpiTimer(bool openmp_thread = false, const char *loc = NULL) : loc(loc) {
    // CPT might not be initialized for the current thread yet
    if (thread_local_clock == nullptr) {
      OmpFiberPoolInit();
      thread_local_clock = THREAD_CLOCK::New(
          my_next_id(), 0, ompt_thread_unknown, openmp_thread);
      thread_local_clock->enterState(STATE_USEFUL, __func__);
    }
    thread_local_clock->enterState(STATE_MPI, loc);
  }
  ~mpiTimer() { thread_local_clock->exitState(STATE_MPI, STATE_USEFUL, loc); }
};

/*
 * RAII classes for MPI piggybacking
 */
#define OnlyActivePB 1

struct mpiIcollBcastPBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;

  mpiIcollBcastPBImpl(MPI_Comm comm, MPI_Request *request, int root) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();

    if (root == rank)
      MpiHappensBefore(rData, REF_RANK);
    rData->IBcast(root, cData, rData->pb_reqs);
  }
  ~mpiIcollBcastPBImpl() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      rData->init(*send_req, IBCAST, REF_RANK);
      rData->setCompletionCallback(completePBHB);
    }
    *send_req = rf.newRequest(*send_req, rData, false);
  }
};

#ifdef HAVE_PCOLL /* Have persistent collective support in MPI */
struct mpiCollBcastInitPBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;

  mpiCollBcastInitPBImpl(MPI_Comm comm, MPI_Request *request, int root) {
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    send_req = request;
    rData = rf.newData();

    if (root == rank)
      rData->setStartCallback(startPersPBHB);
    else
      rData->setStartCallback(startPersPBnoHB);
    rData->setCompletionCallback(completePersPBHB);
    rData->BcastInit(root, cData, rData->pb_reqs);
  }
  ~mpiCollBcastInitPBImpl() {
    rData->init(*send_req, PBCAST, REF_RANK);
    *send_req = rf.newRequest(*send_req, rData, true);
  }
};
#endif

struct mpiCollBcastPBImpl {
  RequestData rData;
  int rank;

  mpiCollBcastPBImpl(MPI_Comm comm, int root) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    if (root == rank)
      MpiHappensBefore(rData, REF_RANK);
    rData.IBcast(root, cData, rData.pb_reqs);
  }
  ~mpiCollBcastPBImpl() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
    MpiHappensAfter(rData, REF_RANK);
  }
};

struct mpiIcollAllreducePBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;

  mpiIcollAllreducePBImpl(MPI_Comm comm, MPI_Request *request, int root = 0) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    MpiHappensBefore(rData, REF_RANK);
    rData->IAllreduce(cData, rData->pb_reqs);
  }
  ~mpiIcollAllreducePBImpl() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      rData->init(*send_req, IALLREDUCE, REF_RANK);
      rData->setCompletionCallback(completePBHB);
    }
    *send_req = rf.newRequest(*send_req, rData, false);
  }
};

#ifdef HAVE_PCOLL /* Have persistent collective support in MPI */
struct mpiCollAllreduceInitPBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;

  mpiCollAllreduceInitPBImpl(MPI_Comm comm, MPI_Request *request,
                             int root = 0) {
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    send_req = request;
    rData = rf.newData();
    rData->setStartCallback(startPersPBHB);
    rData->setCompletionCallback(completePersPBHB);
    rData->AllreduceInit(cData, rData->pb_reqs);
  }
  ~mpiCollAllreduceInitPBImpl() {
    rData->init(*send_req, PALLREDUCE, REF_RANK);
    *send_req = rf.newRequest(*send_req, rData, true);
  }
};
#endif

struct mpiCollAllreducePBImpl {
  RequestData rData;
  int rank;

  mpiCollAllreducePBImpl(MPI_Comm comm, int root = 0) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    MpiHappensBefore(rData, REF_RANK);
    rData.IAllreduce(cData, rData.pb_reqs);
  }
  ~mpiCollAllreducePBImpl() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
    MpiHappensAfter(rData, REF_RANK);
  }
};

struct mpiIcollReducePBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;
  int root;

  mpiIcollReducePBImpl(MPI_Comm comm, MPI_Request *request, int root)
      : root(root) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    MpiHappensBefore(rData, REF_RANK);
    rData->IReduce(root, cData, rData->pb_reqs);
    if (root == rank) {
      rData->setCompletionCallback(completePBHB);
    } else {
      rData->setCompletionCallback(completePBnoHB);
    }
  }
  ~mpiIcollReducePBImpl() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      rData->init(*send_req, IREDUCE, REF_RANK, rank, nullptr, root);
    }
    *send_req = rf.newRequest(*send_req, rData, false);
  }
};

#ifdef HAVE_PCOLL /* Have persistent collective support in MPI */
struct mpiCollReduceInitPBImpl {
  RequestData *rData;
  MPI_Request *send_req;
  int rank;
  int root;

  mpiCollReduceInitPBImpl(MPI_Comm comm, MPI_Request *request, int root)
      : root(root) {
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    send_req = request;
    rData = rf.newData();
    rData->setStartCallback(startPersPBHB);
    rData->ReduceInit(root, cData, rData->pb_reqs);
    if (root == rank) {
      rData->setCompletionCallback(completePersPBHB);
    } else {
      rData->setCompletionCallback(completePersPBnoHB);
    }
  }
  ~mpiCollReduceInitPBImpl() {
    rData->init(*send_req, PREDUCE, REF_RANK, rank, nullptr, root);
    *send_req = rf.newRequest(*send_req, rData, true);
  }
};
#endif
struct mpiCollReducePBImpl {
  RequestData rData;
  int rank;
  int root;

  mpiCollReducePBImpl(MPI_Comm comm, int root) : root(root) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    MpiHappensBefore(rData, REF_RANK);
    rData.IReduce(root, cData, rData.pb_reqs);
  }
  ~mpiCollReducePBImpl() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
    if (root == rank)
      MpiHappensAfter(rData, REF_RANK);
  }
};

// Decide whether to use Allreduce or Bcast/Reduce for 1-all/all-1 collectives
#if 0
// Use Allreduce for all collective functions
typedef mpiCollAllreducePBImpl mpiCollReducePB;
typedef mpiIcollAllreducePBImpl mpiIcollReducePB;
typedef mpiCollAllreducePBImpl mpiCollAllreducePB;
typedef mpiIcollAllreducePBImpl mpiIcollAllreducePB;
typedef mpiCollAllreducePBImpl mpiCollBcastPB;
typedef mpiIcollAllreducePBImpl mpiIcollBcastPB;
#ifdef HAVE_PCOLL /* Have persistent collective support in MPI */
typedef mpiCollAllreduceInitPBImpl mpiCollReduceInitPB;
typedef mpiCollAllreduceInitPBImpl mpiCollAllreduceInitPB;
typedef mpiCollAllreduceInitPBImpl mpiCollBcastInitPB;
#endif
#else
// Use Bcast/Reduce for 1-all/all-1 collectives
typedef mpiCollReducePBImpl mpiCollReducePB;
typedef mpiIcollReducePBImpl mpiIcollReducePB;
typedef mpiCollAllreducePBImpl mpiCollAllreducePB;
typedef mpiIcollAllreducePBImpl mpiIcollAllreducePB;
typedef mpiCollBcastPBImpl mpiCollBcastPB;
typedef mpiIcollBcastPBImpl mpiIcollBcastPB;
#ifdef HAVE_PCOLL /* Have persistent collective support in MPI */
typedef mpiCollReduceInitPBImpl mpiCollReduceInitPB;
typedef mpiCollAllreduceInitPBImpl mpiCollAllreduceInitPB;
typedef mpiCollBcastInitPBImpl mpiCollBcastInitPB;
#endif
#endif

struct mpiIsendPB {
  ipcMetric *uc{nullptr};
  RequestData *rData;
  MPI_Request *send_req;
  int dest;
  int rank;

  mpiIsendPB(int dest, int tag, MPI_Comm comm, MPI_Request *request)
      : dest(dest) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (dest == MPI_PROC_NULL)
      return;
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    uc = MpiHappensBefore(rData);
    PMPI_Isend(uc, NUM_UC_VALUES, ipcData::ipcMpiType, dest, tag,
               cData->getDupComm(), rData->pb_reqs);
  }
  ~mpiIsendPB() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      if (dest != MPI_PROC_NULL) {
        rData->init(*send_req, ISEND, dest);
        rData->setCompletionCallback(completePBnoHB);
      }
    }
    *send_req = rf.newRequest(*send_req, rData, false);
    DCHECK(!rData->isPersistent());
  }
};

struct mpiSendInitPB {
  ipcMetric *uc{nullptr};
  RequestData *rData;
  MPI_Request *send_req;
  int dest;
  int rank;

  mpiSendInitPB(int dest, int tag, MPI_Comm comm, MPI_Request *request)
      : dest(dest) {
    rData = rf.newData();
    send_req = request;
    if (dest == MPI_PROC_NULL)
      return;
    auto cData = cf.findData(comm);
    uc = rData->values;
    rank = cData->getRank();
    rData->setStartCallback(startPersPBHB);
    rData->setCompletionCallback(completePersPBnoHB);
    PMPI_Send_init(uc, NUM_UC_VALUES, ipcData::ipcMpiType, dest, tag,
                   cData->getDupComm(), rData->pb_reqs);
  }
  ~mpiSendInitPB() {
    if (dest != MPI_PROC_NULL) {
      rData->init(*send_req, PSEND, dest);
    }
    *send_req = rf.newRequest(*send_req, rData, true);
    DCHECK(rData->isPersistent());
  }
};

struct mpiSendPB {
  ipcMetric *uc{nullptr};
  RequestData rData{};
  int dest;
  int rank;

  mpiSendPB(int dest, int tag, MPI_Comm comm) : dest(dest) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (dest == MPI_PROC_NULL)
      return;
    auto cData = cf.findData(comm);
    rank = cData->getRank();
    uc = MpiHappensBefore(rData);
    PMPI_Isend(uc, NUM_UC_VALUES, ipcData::ipcMpiType, dest, tag,
               cData->getDupComm(), rData.pb_reqs);
  }
  ~mpiSendPB() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (dest != MPI_PROC_NULL)
      return;
    PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
  }
};

struct mpiMprobePB {
  ipcMetric *uc{nullptr};
  MPI_Comm comm;
  MPI_Message *message;
  CommData *cData;
  MPI_Status tStatus;
  MPI_Status *pStatus;
  int *flag;
  int iflag{1};
  int src{MPI_ANY_SOURCE};
  int tag{MPI_ANY_TAG};
  int rank;

  mpiMprobePB(int src, int tag, MPI_Comm comm, MPI_Status **status,
              MPI_Message *message)
      : mpiMprobePB(src, tag, &iflag, comm, status, message) {}

  mpiMprobePB(int src, int tag, int *flag, MPI_Comm comm, MPI_Status **status,
              MPI_Message *message)
      : comm(comm), message(message), pStatus(*status), flag(flag), src(src),
        tag(tag) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL)
      return;
    if ((src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) &&
        (*status == MPI_STATUS_IGNORE)) {
      pStatus = *status = &tStatus;
    }
  }
  ~mpiMprobePB() {
    if (!*flag)
      return;
    auto rData = rf.newData();
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = rData->values;
    if (src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
      src = pStatus->MPI_SOURCE;
      tag = pStatus->MPI_TAG;
    }
    rData->init(rData->handle, MPROBE, src, tag, cData);
    *message = mf.newHandle(*message, rData);
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    PMPI_Irecv(uc, NUM_UC_VALUES, ipcData::ipcMpiType, src, tag,
               cData->getDupComm(), rData->pb_reqs);
  }
};

struct mpiRecvPB {
  ipcMetric *uc{nullptr};
  RequestData rData;
  CommData *cData;
  MPI_Status tStatus;
  MPI_Status *pStatus;
  int src;
  int tag;
  int rank;

  mpiRecvPB(MPI_Message *message, MPI_Status **status) : pStatus(*status) {
    auto *mData = mf.detachHandle(*message);
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      rData = *mData;
    }
    *message = (MPI_Message)mData->handle;
    mData->fini();
    mf.returnData(mData);
    src = rData.remote;
    tag = rData.tag;
  }

  mpiRecvPB(int src, int tag, MPI_Comm comm, MPI_Status **status)
      : pStatus(*status), src(src), tag(tag) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL)
      return;
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = rData.values;
    if (src != MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
      PMPI_Irecv(uc, NUM_UC_VALUES, ipcData::ipcMpiType, src, tag,
                 cData->getDupComm(), rData.pb_reqs);
    } else {
      if (*status == MPI_STATUS_IGNORE) {
        pStatus = *status = &tStatus;
      }
    }
  }
  ~mpiRecvPB() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL)
      return;
    if (src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
      src = pStatus->MPI_SOURCE;
      tag = pStatus->MPI_TAG;
      PMPI_Recv(uc, NUM_UC_VALUES, ipcData::ipcMpiType, src, tag,
                cData->getDupComm(), MPI_STATUS_IGNORE);
    } else {
      PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
    }
    MpiHappensAfter(rData, src);
  }
};

struct mpiIrecvPB {
  ipcMetric *uc{nullptr};
  RequestData *rData;
  CommData *cData;
  MPI_Request *send_req;
  int rank;
  KIND kind{IRECV};
  int src;
  int tag;

  mpiIrecvPB(MPI_Message *message, MPI_Request *request)
      : send_req(request), kind(IMRECV) {
    rData = mf.detachHandle(*message);
    *message = (MPI_Message)rData->handle;
    src = rData->remote;
  }

  mpiIrecvPB(int src, int tag, MPI_Comm comm, MPI_Request *request)
      : send_req(request), src(src), tag(tag) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL)
      return;
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = rData->values;
    if (src != MPI_ANY_SOURCE && tag != MPI_ANY_TAG) {
      PMPI_Irecv(uc, NUM_UC_VALUES, ipcData::ipcMpiType, src, tag,
                 cData->getDupComm(), rData->pb_reqs);
    }
  }
  ~mpiIrecvPB() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      if (src != MPI_PROC_NULL) {
        if (src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
          rData->init(*send_req, IRECV, src, tag, cData);
          rData->setCompletionCallback(completePBWC);
        } else {
          if (kind == IRECV)
            rData->init(*send_req, kind, src);
          rData->setCompletionCallback(completePBHB);
          rData->setCancelCallback(cancelPB);
        }
      }
    }
    *send_req = rf.newRequest(*send_req, rData, false);
    DCHECK(!rData->isPersistent());
  }
};

struct mpiRecvInitPB {
  ipcMetric *uc{nullptr};
  RequestData *rData;
  CommData *cData;
  MPI_Request *send_req;
  int rank;
  int src;
  int tag;

  mpiRecvInitPB(int src, int tag, MPI_Comm comm, MPI_Request *request)
      : send_req(request), src(src), tag(tag) {
    rData = rf.newData();
    send_req = request;
    if (src == MPI_PROC_NULL)
      return;
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = rData->values;
    if (src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
      rData->setCompletionCallback(completePBWC);
    } else {
      rData->setStartCallback(startPersPBnoHB);
      rData->setCompletionCallback(completePersPBHB);
      rData->setCancelCallback(cancelPB);
      PMPI_Recv_init(uc, NUM_UC_VALUES, ipcData::ipcMpiType, src, tag,
                     cData->getDupComm(), rData->pb_reqs);
    }
  }
  ~mpiRecvInitPB() {
    if (src != MPI_PROC_NULL) {
      if (src == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
        rData->init(*send_req, IRECV, src, tag, cData);
      } else {
        rData->init(*send_req, IRECV, src);
      }
    }
    *send_req = rf.newRequest(*send_req, rData, true);
    DCHECK(rData->isPersistent());
  }
};

#ifdef HAVE_ISRR

struct mpiSendrecvPB {
  ipcMetric *uc{nullptr};
  RequestData rData{};
  CommData *cData;
  MPI_Status tStatus;
  MPI_Status *pStatus;
  int dest;
  int src;
  int stag;
  int rtag;
  int rank;

  mpiSendrecvPB(int dest, int stag, int src, int rtag, MPI_Comm comm,
                MPI_Status **status)
      : pStatus(*status), dest(dest), src(src), stag(stag), rtag(rtag) {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL && dest == MPI_PROC_NULL)
      return;
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = MpiHappensBefore(rData);
    PMPI_Isendrecv_replace(uc, NUM_UC_VALUES, ipcData::ipcMpiType, dest, stag,
                           src, rtag, cData->getDupComm(), rData.pb_reqs);
  }
  ~mpiSendrecvPB() {
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL && dest == MPI_PROC_NULL)
      return;

    PMPI_Waitall(2, rData.pb_reqs, MPI_STATUSES_IGNORE);
    if (src != MPI_PROC_NULL)
      MpiHappensAfter(rData, src);
  }
};

struct mpiIsendrecvPB {
  ipcMetric *uc{nullptr};
  RequestData *rData;
  CommData *cData;
  MPI_Request *send_req;
  KIND kind{IRECV};
  int dest;
  int src;
  int stag;
  int rtag;
  int rank;

  mpiIsendrecvPB(int dest, int stag, int src, int rtag, MPI_Comm comm,
                 MPI_Request *request)
      : send_req(request), dest(dest), src(src), stag(stag), rtag(rtag) {
    rData = rf.newData();
    send_req = request;
#if OnlyActivePB
    if (!analysis_flags->running)
      return;
#endif
    if (src == MPI_PROC_NULL && dest == MPI_PROC_NULL)
      return;
    cData = cf.findData(comm);
    rank = cData->getRank();
    uc = MpiHappensBefore(rData);
    PMPI_Isendrecv_replace(uc, NUM_UC_VALUES, ipcData::ipcMpiType, dest, stag,
                           src, rtag, cData->getDupComm(), rData->pb_reqs);
  }
  ~mpiIsendrecvPB() {
#if OnlyActivePB
    if (analysis_flags->running)
#endif
    {
      rData->init(*send_req, kind, src);
      rData->setCancelCallback(cancelPB);
      if (src != MPI_PROC_NULL) {
        rData->setCompletionCallback(completePBHB);
      } else {
        rData->setCompletionCallback(completePBnoHB);
      }
    }
    *send_req = rf.newRequest(*send_req, rData, false);
    CHECK_NOT(rData->isPersistent());
  }
};

#endif

#endif
