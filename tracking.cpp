#include "tracking.h"
#ifdef USE_MPI
#include "mpi.h"
#endif

#ifdef USE_ERRHANDLER
#include <execinfo.h>
#include <sys/types.h>
#include <unistd.h>

#if !__cpp_exceptions
namespace std {
void __throw_system_error(int) { abort(); }
void __throw_bad_function_call() { abort(); }
} // namespace std
#endif

#ifdef USE_MPI
static MPI_Errhandler CommErrHandler{MPI_ERRHANDLER_NULL};

static void myMpiErrHandler(MPI_Comm *comm, int *errCode, ...) {
  printf("Handling MPI Error %i at pid %i\n", *errCode, getpid());
  sleep(10);
  disable_signalhandlers();
  abort();
}
void createErrHandler() {
  if (!analysis_flags->stacktrace)
    return;
  PMPI_Comm_create_errhandler(myMpiErrHandler, &CommErrHandler);
}
void registerErrHandler(MPI_Comm comm) {
  if (!analysis_flags->stacktrace)
    return;
  PMPI_Comm_set_errhandler(comm, CommErrHandler);
}
#endif

#ifdef FORTRAN_SUPPORT
#ifdef HANDLE_OP
template <> void OpData::init(MPI_Op op) {
  handle = op;
  fHandle = PMPI_Op_c2f(op);
}
#ifndef MPI_Op_c2f
MPI_Fint MPI_Op_c2f(MPI_Op op) { return of.c2f(op); }
MPI_Op MPI_Op_f2c(MPI_Fint op) { return of.f2c(op); }
#endif
#endif
#ifdef HANDLE_WIN
template <> void WinData::init(MPI_Win win) {
  handle = win;
  fHandle = PMPI_Win_c2f(win);
}
#ifndef MPI_Win_c2f
MPI_Fint MPI_Win_c2f(MPI_Win win) { return wf.c2f(win); }
MPI_Win MPI_Win_f2c(MPI_Fint win) { return wf.f2c(win); }
#endif
#endif
#ifdef HANDLE_TYPE
template <> void TypeData::init(MPI_Datatype type) {
  handle = type;
  fHandle = PMPI_Type_c2f(type);
}
#ifndef MPI_Type_c2f
MPI_Fint MPI_Type_c2f(MPI_Datatype type) { return tf.c2f(type); }
MPI_Datatype MPI_Type_f2c(MPI_Fint type) { return tf.f2c(type); }
#endif
#endif
#ifdef HANDLE_FILE
template <> void FileData::init(MPI_File file) {
  handle = file;
  fHandle = PMPI_File_c2f(file);
}
#ifndef MPI_File_c2f
MPI_Fint MPI_File_c2f(MPI_File file) { return ff.c2f(file); }
MPI_File MPI_File_f2c(MPI_Fint file) { return ff.f2c(file); }
#endif
#endif
#ifdef HANDLE_COMM
#ifndef MPI_Comm_c2f
MPI_Fint MPI_Comm_c2f(MPI_Comm comm) { return cf.c2f(comm); }
MPI_Comm MPI_Comm_f2c(MPI_Fint comm) { return cf.f2c(comm); }
#endif
#endif
#ifdef HANDLE_GROUP
template <> void GroupData::init(MPI_Group group) {
  handle = group;
  fHandle = PMPI_Group_c2f(group);
}
#ifndef MPI_Group_c2f
MPI_Fint MPI_Group_c2f(MPI_Group group) { return gf.c2f(group); }
MPI_Group MPI_Group_f2c(MPI_Fint group) { return gf.f2c(group); }
#endif
#endif
#endif

#ifdef HANDLE_WIN
template <> MPI_Win WinData::nullHandle{MPI_WIN_NULL};
WinFactory wf;
#endif
#ifdef HANDLE_FILE
template <> MPI_File FileData::nullHandle{MPI_FILE_NULL};
FileFactory ff;
#endif
#ifdef HANDLE_COMM
template <>
bool AbstractHandleFactory<MPI_Comm, CommData, toolCommData>::isPredefined(
    MPI_Comm handle) {
  return handle == MPI_COMM_NULL || handle == MPI_COMM_WORLD ||
         handle == MPI_COMM_SELF;
}
template <>
void AbstractHandleFactory<MPI_Comm, CommData, toolCommData>::initPredefined() {
  predefHandles[MPI_COMM_NULL] = newData()->init(MPI_COMM_NULL);
  predefHandles[MPI_COMM_WORLD] = newData()->init(MPI_COMM_WORLD);
  predefHandles[MPI_COMM_SELF] = newData()->init(MPI_COMM_SELF);
#ifdef FORTRAN_SUPPORT
  predefFHandles[predefHandles[MPI_COMM_NULL]->fHandle] =
      predefHandles[MPI_COMM_NULL];
  predefFHandles[predefHandles[MPI_COMM_WORLD]->fHandle] =
      predefHandles[MPI_COMM_WORLD];
  predefFHandles[predefHandles[MPI_COMM_SELF]->fHandle] =
      predefHandles[MPI_COMM_SELF];
#endif
}
MPI_Comm CommData::nullHandle{MPI_COMM_NULL};
CommFactory cf;
#endif
#ifdef HANDLE_MESSAGE
template <>
bool AbstractHandleFactory<MPI_Message, MessageData,
                           toolMessageData>::isPredefined(MPI_Message handle) {
  return handle == MPI_MESSAGE_NULL || handle == MPI_MESSAGE_NO_PROC;
}
template <>
void AbstractHandleFactory<MPI_Message, MessageData,
                           toolMessageData>::initPredefined() {}
MessageFactory mf;
#endif
#ifdef HANDLE_REQUEST
MPI_Request RequestData::nullHandle{MPI_REQUEST_NULL};
RequestFactory rf;
#endif
#if defined(HAVE_SESSION) && defined(HANDLE_SESSION)
template <> MPI_Session SessionData::nullHandle{MPI_SESSION_NULL};
SessionFactory sf;
#endif

int ipcData::num_uc_double{NUM_UC_DOUBLE + 1};
int ipcData::num_uc_int64{NUM_UC_INT64};
MPI_Datatype ipcData::ipcMpiType{MPI_DATATYPE_NULL};
MPI_Op ipcData::ipcMpiOp{MPI_OP_NULL};

void maxloc(depMetric *, depMetric *, int *, MPI_Datatype *);

void maxloc(depMetric *invec, depMetric *inoutvec, int *len,
            MPI_Datatype *dtype) {
  DCHECK_EQ(ipcData::ipcMpiType, *dtype);
  for (int i = 0; i < *len; i++) {
    if (inoutvec[i].fvalues[0] < invec[i].fvalues[0]) {
      for (int j = 0; j < ipcData::num_uc_double; j++)
        inoutvec[i].fvalues[j] = invec[i].fvalues[j];
      for (int j = 0; j < ipcData::num_uc_int64; j++)
        inoutvec[i].ivalues[j] = invec[i].ivalues[j];
    }
  }
}

void ipcData::initIpcData() {
  if (num_uc_int64 > 0) {
    depMetric tempData{};
    MPI_Aint displs[2];
    PMPI_Get_address(tempData.fvalues, displs);
    PMPI_Get_address(tempData.ivalues, displs + 1);
    MPI_Datatype struType = MPI_DATATYPE_NULL;
    displs[1] -= displs[0];
    displs[0] = 0;
    MPI_Datatype types[] = {MPI_DOUBLE, MPI_INT64_T};
    int blengths[] = {num_uc_double, num_uc_int64};
    PMPI_Type_create_struct(2, blengths, displs, types, &struType);
    PMPI_Type_create_resized(struType, 0, sizeof(depMetric), &ipcMpiType);
    PMPI_Type_commit(&ipcMpiType);
    PMPI_Type_free(&struType);
    PMPI_Op_create((MPI_User_function *)maxloc, 1, &ipcMpiOp);
  } else {
    ipcMpiType = MPI_DOUBLE;
    ipcMpiOp = MPI_MAX;
  }
}
void ipcData::finiIpcData() {
  if (ipcMpiType != MPI_DATATYPE_NULL && ipcMpiType != MPI_DOUBLE)
    PMPI_Type_free(&ipcMpiType);
  if (ipcMpiOp != MPI_MAX)
    PMPI_Op_free(&ipcMpiOp);
}
#endif // USE_MPI
