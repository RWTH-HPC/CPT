#include <mpi.h>
#include <stdio.h>

#include "criticalPath.h"
#include "mpi-critical.h"
#include "tracking.h"

/* Start & Completion */

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvla-cxx-extension"
#endif

_EXTERN_C_ int MPI_Start(MPI_Request *request) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->pers++;
  MPI_Request myReq = rf.startRequest(*request);
  return PMPI_Start(&myReq);
}

_EXTERN_C_ int MPI_Startall(int count, MPI_Request array_of_requests[]) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->pers++;
  MPI_Request myReqs[count];
  {
    for (int i = 0; i < count; i++) {
      myReqs[i] = rf.startRequest(array_of_requests[i]);
    }
  }
  return PMPI_Startall(count, myReqs);
}

_EXTERN_C_ int MPI_Request_free(MPI_Request *request) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->pers++;
  MPI_Request myReq = rf.getHandle(*request);
  int ret = PMPI_Request_free(&myReq);
  *request = rf.freeHandle(*request);
  return ret;
}

_EXTERN_C_ int MPI_Cancel(MPI_Request *request) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->pers++;
  MPI_Request &myReq = rf.getHandle(*request);
  int ret = PMPI_Cancel(&myReq);
  rf.cancelRequest(*request);
  return ret;
}

_EXTERN_C_ int MPI_Wait(MPI_Request *request, MPI_Status *status) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->wait++;
  MPI_Request &myReq = rf.getHandle(*request);
  MPI_Status tmpstatus;
  if (status == MPI_STATUS_IGNORE)
    status = &tmpstatus;
  int ret = PMPI_Wait(&myReq, status);
  *request = rf.completeRequest(*request, status);
  return ret;
}

_EXTERN_C_ int MPI_Waitall(int count, MPI_Request array_of_requests[],
                           MPI_Status array_of_statuses[]) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->wait++;
  MPI_Request myReqs[count];
  MPI_Status tmpstatuses[count];
  if (array_of_statuses == MPI_STATUSES_IGNORE)
    array_of_statuses = tmpstatuses;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < count; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Waitall(count, myReqs, array_of_statuses);
  {
    for (int i = 0; i < count; i++) {
      array_of_requests[i] = rf.completeRequest(array_of_requests[i], myReqs[i],
                                                array_of_statuses + i);
    }
  }
  return ret;
}

_EXTERN_C_ int MPI_Waitany(int count, MPI_Request array_of_requests[],
                           int *indx, MPI_Status *status) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->wait++;
  MPI_Request myReqs[count];
  MPI_Status tmpstatus;
  if (status == MPI_STATUS_IGNORE)
    status = &tmpstatus;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < count; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Waitany(count, myReqs, indx, status);
  {
    array_of_requests[*indx] =
        rf.completeRequest(array_of_requests[*indx], myReqs[*indx], status);
  }
  return ret;
}

_EXTERN_C_ int MPI_Waitsome(int incount, MPI_Request array_of_requests[],
                            int *outcount, int array_of_indices[],
                            MPI_Status array_of_statuses[]) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->wait++;
  MPI_Request myReqs[incount];
  MPI_Status tmpstatuses[incount];
  if (array_of_statuses == MPI_STATUSES_IGNORE)
    array_of_statuses = tmpstatuses;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < incount; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Waitsome(incount, myReqs, outcount, array_of_indices,
                          array_of_statuses);
  {
    for (int i = 0; i < *outcount; i++) {
      array_of_requests[array_of_indices[i]] = rf.completeRequest(
          array_of_requests[array_of_indices[i]], myReqs[array_of_indices[i]],
          array_of_statuses + i);
    }
  }
  return ret;
}

_EXTERN_C_ int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->test++;
  MPI_Request &myReq = rf.getHandle(*request);
  int ret = PMPI_Test(&myReq, flag, status);
  if (*flag) {
    *request = rf.completeRequest(*request, status);
  }
  return ret;
}

_EXTERN_C_ int MPI_Testall(int count, MPI_Request array_of_requests[],
                           int *flag, MPI_Status array_of_statuses[]) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->test++;
  MPI_Request myReqs[count];
  MPI_Status tmpstatuses[count];
  if (array_of_statuses == MPI_STATUSES_IGNORE)
    array_of_statuses = tmpstatuses;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < count; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Testall(count, myReqs, flag, array_of_statuses);
  if (*flag) {
    for (int i = 0; i < count; i++) {
      array_of_requests[i] = rf.completeRequest(array_of_requests[i], myReqs[i],
                                                array_of_statuses + i);
    }
  }
  return ret;
}

_EXTERN_C_ int MPI_Testany(int count, MPI_Request array_of_requests[],
                           int *indx, int *flag, MPI_Status *status) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->test++;
  MPI_Request myReqs[count];
  MPI_Status tmpstatus;
  if (status == MPI_STATUS_IGNORE)
    status = &tmpstatus;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < count; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Testany(count, myReqs, indx, flag, status);
  if (*flag) {
    array_of_requests[*indx] =
        rf.completeRequest(array_of_requests[*indx], myReqs[*indx], status);
  }
  return ret;
}

_EXTERN_C_ int MPI_Testsome(int incount, MPI_Request array_of_requests[],
                            int *outcount, int array_of_indices[],
                            MPI_Status array_of_statuses[]) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    thread_local_clock->test++;
  MPI_Request myReqs[incount];
  MPI_Status tmpstatuses[incount];
  if (array_of_statuses == MPI_STATUSES_IGNORE)
    array_of_statuses = tmpstatuses;
  {
    auto lock = rf.getSharedLock();
    for (int i = 0; i < incount; i++) {
      myReqs[i] = rf.getHandleLocked(array_of_requests[i]);
    }
  }
  int ret = PMPI_Testsome(incount, myReqs, outcount, array_of_indices,
                          array_of_statuses);
  {
    for (int i = 0; i < *outcount; i++) {
      array_of_requests[array_of_indices[i]] = rf.completeRequest(
          array_of_requests[array_of_indices[i]], myReqs[array_of_indices[i]],
          array_of_statuses + i);
    }
  }
  return ret;
}
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
