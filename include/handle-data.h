#ifndef HANDLE_DATA_H
#define HANDLE_DATA_H 1

#include <cassert>
#include <functional>
#include <mpi.h>
#include <stdlib.h>

#include "ipc-data.h"

enum nbFunction {
  nbf_unknown = -1,
  nbf_MPI_Isend,
  nbf_MPI_Issend,
  nbf_MPI_Irsend,
  nbf_MPI_Ibsend,
  nbf_MPI_Irecv,

  nbf_MPI_Send_init,
  nbf_MPI_Ssend_init,
  nbf_MPI_Rsend_init,
  nbf_MPI_Bsend_init,
  nbf_MPI_Recv_init,

  nbf_MPI_Iallgather,
  nbf_MPI_Iallgatherv,
  nbf_MPI_Iallreduce,
  nbf_MPI_Ialltoall,
  nbf_MPI_Ialltoallv,
  nbf_MPI_Ialltoallw,
  nbf_MPI_Ibarrier,
  nbf_MPI_Ibcast,
  nbf_MPI_Iexscan,
  nbf_MPI_Igather,
  nbf_MPI_Igatherv,
  nbf_MPI_Ireduce,
  nbf_MPI_Ireduce_scatter,
  nbf_MPI_Ireduce_scatter_block,
  nbf_MPI_Iscan,
  nbf_MPI_Iscatter,
  nbf_MPI_Iscatterv,

  nbf_MPI_Allgather_init,
  nbf_MPI_Allgatherv_init,
  nbf_MPI_Allreduce_init,
  nbf_MPI_Alltoall_init,
  nbf_MPI_Alltoallv_init,
  nbf_MPI_Alltoallw_init,
  nbf_MPI_Barrier_init,
  nbf_MPI_Bcast_init,
  nbf_MPI_Exscan_init,
  nbf_MPI_Gather_init,
  nbf_MPI_Gatherv_init,
  nbf_MPI_Reduce_init,
  nbf_MPI_Reduce_scatter_init,
  nbf_MPI_Reduce_scatter_block_init,
  nbf_MPI_Scan_init,
  nbf_MPI_Scatter_init,
  nbf_MPI_Scatterv_init,

  nbf_MPI_Neighbor_iallgather,
  nbf_MPI_Neighbor_iallgatherv,
  nbf_MPI_Neighbor_iallreduce,
  nbf_MPI_Neighbor_ialltoall,
  nbf_MPI_Neighbor_ialltoallv,
  nbf_MPI_Neighbor_ialltoallw,
  nbf_MPI_Neighbor_ibarrier,
  nbf_MPI_Neighbor_ibcast,
  nbf_MPI_Neighbor_iexscan,
  nbf_MPI_Neighbor_igather,
  nbf_MPI_Neighbor_igatherv,
  nbf_MPI_Neighbor_ireduce,
  nbf_MPI_Neighbor_ireduce_scatter,
  nbf_MPI_Neighbor_ireduce_scatter_block,
  nbf_MPI_Neighbor_iscan,
  nbf_MPI_Neighbor_iscatter,
  nbf_MPI_Neighbor_iscatterv,

  nbf_MPI_Neighbor_allgather_init,
  nbf_MPI_Neighbor_allgatherv_init,
  nbf_MPI_Neighbor_allreduce_init,
  nbf_MPI_Neighbor_alltoall_init,
  nbf_MPI_Neighbor_alltoallv_init,
  nbf_MPI_Neighbor_alltoallw_init,
  nbf_MPI_Neighbor_barrier_init,
  nbf_MPI_Neighbor_bcast_init,
  nbf_MPI_Neighbor_exscan_init,
  nbf_MPI_Neighbor_gather_init,
  nbf_MPI_Neighbor_gatherv_init,
  nbf_MPI_Neighbor_reduce_init,
  nbf_MPI_Neighbor_reduce_scatter_init,
  nbf_MPI_Neighbor_reduce_scatter_block_init,
  nbf_MPI_Neighbor_scan_init,
  nbf_MPI_Neighbor_scatter_init,
  nbf_MPI_Neighbor_scatterv_init
};

template <typename M, auto E> class alignas(64) HandleData {
protected:
public:
  static M nullHandle;
  M handle{nullHandle};
#ifdef FORTRAN_SUPPORT
  int fHandle{-1};
#endif
  HandleData() {}
  /*  HandleData(M group
  #ifdef FORTRAN_SUPPORT
               ,
               MPI_Fint fGroup
  #endif
               )
        : handle(group)
  #ifdef FORTRAN_SUPPORT
          ,
          fHandle(fGroup)
  #endif
    {
    }*/
  HandleData(const HandleData &o) { this->init(o.handle); }
  HandleData(HandleData &&o) noexcept { this->init(o.handle); }
  virtual HandleData<M, E> *init(M group
#ifdef FORTRAN_SUPPORT
                                 ,
                                 MPI_Fint fGroup
#endif
  ) {
    handle = group;
#ifdef FORTRAN_SUPPORT
    fHandle = fGroup;
#endif
    return this;
  }
  /*virtual void init(const HandleData &o) {
    handle = o.handle;
#ifdef FORTRAN_SUPPORT
    fHandle = o.fHandle;
#endif
  }*/
  virtual void fini() {}

  // define new/delete operator to avoid dependency to libstdc++
  void *operator new(size_t size, void *ptr) { return ptr; }

private:
  // operator new/delete must never be called directly
  void *operator new(size_t size) = delete;
  void operator delete(void *p) = delete;
};

class alignas(64) CommData {
  MPI_Comm dupComm{MPI_COMM_NULL};
  int size{0};
  int rank{-1};

public:
  static MPI_Comm nullHandle;
  MPI_Comm handle{nullHandle};
#ifdef FORTRAN_SUPPORT
  int fHandle{-1};
#endif
  CommData *init(MPI_Comm comm
#ifdef FORTRAN_SUPPORT
                 ,
                 MPI_Fint fComm = -1
#endif
  ) {
    this->handle = comm;
#ifdef FORTRAN_SUPPORT
    if (fComm == -1)
      fHandle = PMPI_Comm_c2f(comm);
    else
      fHandle = fComm;
#endif
    if (comm == MPI_COMM_NULL)
      return this;
    PMPI_Comm_dup(comm, &dupComm);
    PMPI_Comm_size(comm, &size);
    PMPI_Comm_rank(comm, &rank);
    return this;
  }
  void fini() { PMPI_Comm_free(&dupComm); }
  int getSize() { return size; }
  int getRank() { return rank; }
  MPI_Comm &getDupComm() { return dupComm; }

  // define new/delete operator to avoid dependency to libstdc++
  void *operator new(size_t size, void *ptr) { return ptr; }

  // private:
  void *operator new(size_t size) { // static_assert(false, "operator new must
                                    // never be called directly");
    DCHECK(false && "operator new must never be called directly");
    return malloc(size);
  }
  void operator delete(void *p) { // static_assert(false, "operator delete must
                                  // never be called directly");
    DCHECK(false && "operator delete must never be called directly");
  }
};

typedef enum {
  ISEND = 0,
  IRECV,
  IMRECV,
  MPROBE,
  ICOLL,
  IREDUCE,
  IALLREDUCE,
  IBCAST,
  PSEND,
  PRECV,
  PCOLL,
  PREDUCE,
  PALLREDUCE,
  PBCAST
} KIND;

class ipcData {
public:
  ipcMetric values[NUM_UC_VALUES];
  static int num_uc_double;
  static int num_uc_int64;
  static MPI_Datatype ipcMpiType;
  static MPI_Op ipcMpiOp;
  static void initIpcData();
  static void finiIpcData();
  /* NOTE: See copy-assignment constructor of RequestData why this is explictly
   * defined. */
  ipcData &operator=(const ipcData &rhs) {
    for (int i = 0; i < NUM_UC_VALUES; ++i)
      values[i] = rhs.values[i];
    return *this;
  }
  void IBcast(int root, CommData *cData, MPI_Request *reqs) {
    PMPI_Ibcast(values, NUM_UC_VALUES, ipcMpiType, root, cData->getDupComm(),
                reqs);
  }
  void IAllreduce(CommData *cData, MPI_Request *reqs) {
    PMPI_Iallreduce(MPI_IN_PLACE, values, NUM_UC_VALUES, ipcMpiType, ipcMpiOp,
                    cData->getDupComm(), reqs);
  }
  void Allreduce(CommData *cData) {
    PMPI_Allreduce(MPI_IN_PLACE, values, NUM_UC_VALUES, ipcMpiType, ipcMpiOp,
                   cData->getDupComm());
  }
  void IReduce(int root, CommData *cData, MPI_Request *reqs) {
    if (root == cData->getRank()) {
      PMPI_Ireduce(MPI_IN_PLACE, values, NUM_UC_VALUES, ipcMpiType, ipcMpiOp,
                   root, cData->getDupComm(), reqs);
    } else {
      PMPI_Ireduce(values, NULL, NUM_UC_VALUES, ipcMpiType, ipcMpiOp, root,
                   cData->getDupComm(), reqs);
    }
  }
#ifdef HAVE_PCOLL
  void BcastInit(int root, CommData *cData, MPI_Request *reqs) {
    PMPI_Bcast_init(values, NUM_UC_VALUES, ipcMpiType, root,
                    cData->getDupComm(), MPI_INFO_NULL, reqs);
  }
  void AllreduceInit(CommData *cData, MPI_Request *reqs) {
    PMPI_Allreduce_init(MPI_IN_PLACE, values, NUM_UC_VALUES, ipcMpiType,
                        ipcMpiOp, cData->getDupComm(), MPI_INFO_NULL, reqs);
  }
  void ReduceInit(int root, CommData *cData, MPI_Request *reqs) {
    if (root == cData->getRank()) {
      PMPI_Reduce_init(MPI_IN_PLACE, values, NUM_UC_VALUES, ipcMpiType,
                       ipcMpiOp, root, cData->getDupComm(), MPI_INFO_NULL,
                       reqs);
    } else {
      PMPI_Reduce_init(values, NULL, NUM_UC_VALUES, ipcMpiType, ipcMpiOp, root,
                       cData->getDupComm(), MPI_INFO_NULL, reqs);
    }
  }
#endif
};

class alignas(64) RequestData : public ipcData {
  bool persistent{false};
  std::function<void(RequestData *)> startCallback{};
  std::function<void(RequestData *)> cancelCallback{};
  std::function<void(RequestData *, MPI_Status *)> completionCallback{};
  bool freed{true};
  bool cancelled{false};

protected:
public:
  KIND kind{ISEND};
  MPI_Request pb_reqs[2]{MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  int remote{-42};
  int tag{-42};
  int root{-42};
  CommData *comm{nullptr};

  static MPI_Request nullHandle;
  RequestData() = default;
  ~RequestData() = default;
  /* NOTE: We explictly define this since implicit definition might use memcpy
   * which causes a segfault when triggered in mpiRecvPB(MPI_Message...) because
   * the memory might be registered with UCX. In this case, the generated memcpy
   * operations do not adhere to UCX's memory restrictions and cause undefined
   * behavior. Same holds for ipcData.
   */
  RequestData &operator=(const RequestData &rhs) {
    if (this == &rhs)
      return *this;
    ipcData::operator=(rhs);
    persistent = rhs.persistent;
    startCallback = rhs.startCallback;
    cancelCallback = rhs.cancelCallback;
    completionCallback = rhs.completionCallback;
    freed = rhs.freed;
    cancelled = rhs.cancelled;
    kind = rhs.kind;
    pb_reqs[0] = rhs.pb_reqs[0];
    pb_reqs[1] = rhs.pb_reqs[1];
    remote = rhs.remote;
    tag = rhs.tag;
    root = rhs.root;
    comm = rhs.comm;
    return *this;
  }
  MPI_Request handle{MPI_REQUEST_NULL};
#ifdef FORTRAN_SUPPORT
  MPI_Fint fHandle{-1};
#endif

  RequestData *init(MPI_Request request, KIND _kind, int _remote = -1,
                    int _tag = -1, CommData *_comm = nullptr, int _root = -1,
                    bool _persistent = false) {
    handle = request;
#ifdef FORTRAN_SUPPORT
    fHandle = -1;
#endif
    kind = _kind;
    remote = _remote;
    tag = _tag;
    comm = _comm;
    root = _root;
    persistent = _persistent;
    freed = false;
    cancelled = false;
    return this;
  }

  template <typename M>
  RequestData *init(M request,
#ifdef FORTRAN_SUPPORT
                    MPI_Fint fRequest = -1,
#endif
                    bool _persistent = false) {
    persistent = _persistent;
    handle = (MPI_Request)request;
#ifdef FORTRAN_SUPPORT
    if (fRequest == -1)
      fHandle = PMPI_Request_c2f((MPI_Request)request);
    else
      fHandle = fRequest;
#endif
    freed = false;
    cancelled = false;
    return this;
  }

  void start() {
    DCHECK(!freed);
    if (startCallback) {
      startCallback(this);
    }
  }
  void cancel() {
    DCHECK(!freed);
    DCHECK(!cancelled);
    cancelled = true;
    if (cancelCallback) {
      cancelCallback(this);
    }
  }
  void complete(MPI_Status *status) {
    DCHECK(!freed);
    if (completionCallback) {
      completionCallback(this, status);
    }
  }
  void fini(MPI_Status *status) {
    complete(status);
    fini();
  }
  void fini() {
    handle = MPI_REQUEST_NULL;
#ifdef FORTRAN_SUPPORT
    fHandle = -1;
#endif
    pb_reqs[0] = MPI_REQUEST_NULL;
    pb_reqs[1] = MPI_REQUEST_NULL;
    completionCallback = nullptr;
    startCallback = nullptr;
    cancelCallback = nullptr;
    remote = -42;
    tag = -42;
    root = -42;
    kind = ISEND;
    freed = true;
    cancelled = false;
  }
  bool isFreed() { return freed; }
  bool isPersistent() {
    DCHECK(!freed);
    return persistent;
  }
  bool isCancelled() {
    bool ret = cancelled;
    cancelled = false;
    return ret;
  }
  void setStartCallback(std::function<void(RequestData *)> sc) {
    persistent = true;
    startCallback = sc;
  }
  void setCancelCallback(std::function<void(RequestData *)> cc) {
    cancelCallback = cc;
  }
  void
  setCompletionCallback(std::function<void(RequestData *, MPI_Status *)> cc) {
    completionCallback = cc;
  }
};

#endif
