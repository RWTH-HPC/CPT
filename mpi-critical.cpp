/*
 * critPathAnalysis.cpp -- Critical path analysis runtime library, build for
 * hybrid OpenMp and MPI applications
 */

#include "criticalPath.h"
#include <algorithm>
#include <execinfo.h>
#include <mpi.h>

#include "handle-data.h"
#include "ipc-data.h"
#include "mpi-critical.h"

Vector<double> timeOffsets;

void MpiHappensAfter(ipcData &uc, int remote) { MpiHappensAfter(&uc, remote); }
void MpiHappensAfter(ipcData *uc, int remote) {
  if (!analysis_flags->running)
    return;
  DCHECK_EQ(thread_local_clock->GetState(), STATE_MPI);
  DCHECK(remote >= -1);
  thread_local_clock->clocks[CLOCK_USEFUL].critical.maxUpdate(
      BaseMetric{uc->values[0]});
}

ipcMetric *MpiHappensBefore(ipcData &uc, int remote) {
  return MpiHappensBefore(&uc, remote);
}

ipcMetric *MpiHappensBefore(ipcData *uc, int remote) {
  thread_local_clock->clocks[CLOCK_USEFUL].critical.loadValues(uc->values[0]);
  return uc->values;
}

// completion callback function for wild-card recv
void completePBWC(RequestData *uc, MPI_Status *status) {
  if (uc->isCancelled())
    return;
  DCHECK(status->MPI_SOURCE >= 0);
  DCHECK(status->MPI_TAG == uc->tag || MPI_ANY_TAG == uc->tag);
  DCHECK(status->MPI_SOURCE == uc->remote || MPI_ANY_SOURCE == uc->remote);
  DCHECK(uc->comm->getDupComm() != MPI_COMM_NULL);
  if (uc->remote == MPI_ANY_SOURCE)
    uc->remote = status->MPI_SOURCE;
  PMPI_Recv(uc->values, NUM_UC_VALUES, ipcData::ipcMpiType, status->MPI_SOURCE,
            status->MPI_TAG, uc->comm->getDupComm(), MPI_STATUS_IGNORE);
  MpiHappensAfter(uc, uc->remote);
}

// completion callback function with clock update
void completePBHB(RequestData *uc, MPI_Status *status) {
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Wait(uc->pb_reqs, MPI_STATUS_IGNORE);
  else
    PMPI_Waitall(2, uc->pb_reqs, MPI_STATUSES_IGNORE);
  if (!uc->isCancelled())
    MpiHappensAfter(uc, uc->remote);
}

// completion callback function without clock update
void completePBnoHB(RequestData *uc, MPI_Status *status) {
  uc->isCancelled();
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Wait(uc->pb_reqs, MPI_STATUS_IGNORE);
  else
    PMPI_Waitall(2, uc->pb_reqs, MPI_STATUSES_IGNORE);
}

// cancel callback function
void cancelPB(RequestData *uc) {
  if (uc->pb_reqs[0] != MPI_REQUEST_NULL)
    PMPI_Cancel(uc->pb_reqs);
  if (uc->pb_reqs[1] != MPI_REQUEST_NULL)
    PMPI_Cancel(uc->pb_reqs + 1);
}

// completion callback function for persistent request with clock update
void completePersPBHB(RequestData *uc, MPI_Status *status) {
#if OnlyActivePB
  if (!analysis_flags->running)
    return;
#endif
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Wait(uc->pb_reqs, MPI_STATUS_IGNORE);
  else
    PMPI_Waitall(2, uc->pb_reqs, MPI_STATUSES_IGNORE);
  if (!uc->isCancelled())
    MpiHappensAfter(uc, uc->remote);
}

// completion callback function for persistent request without clock update
void completePersPBnoHB(RequestData *uc, MPI_Status *status) {
  uc->isCancelled();
#if OnlyActivePB
  if (!analysis_flags->running)
    return;
#endif
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Wait(uc->pb_reqs, MPI_STATUS_IGNORE);
  else
    PMPI_Waitall(2, uc->pb_reqs, MPI_STATUSES_IGNORE);
}

// start callback function for persistent request with clock update
void startPersPBHB(RequestData *uc) {
#if OnlyActivePB
  if (!analysis_flags->running)
    return;
#endif
  MpiHappensBefore(uc);
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Start(uc->pb_reqs);
  else
    PMPI_Startall(2, uc->pb_reqs);
}

// start callback function for persistent request without clock update
void startPersPBnoHB(RequestData *uc) {
#if OnlyActivePB
  if (!analysis_flags->running)
    return;
#endif
  if (uc->pb_reqs[1] == MPI_REQUEST_NULL)
    PMPI_Start(uc->pb_reqs);
  else
    PMPI_Startall(2, uc->pb_reqs);
}

void init_timer_offsets() {
  auto cw_dup_data = cf.findData(MPI_COMM_WORLD);
  int size = cw_dup_data->getSize(), rank = cw_dup_data->getRank();
  auto cw_dup = cw_dup_data->getDupComm();

// build offset table
#define NTIMES 19
  Vector<double> localTimes, allTimes;
  double localTime, endTime, remoteTime;

  timeOffsets.Resize(size);
  localTimes.Resize(NTIMES);
  allTimes.Resize(size * NTIMES);

  for (int i = 0; i < NTIMES; i++) {
    if (rank == 0) {
      for (int j = 1; j < size; j++) {
        localTime = getTime();
        PMPI_Send(&localTime, 1, MPI_DOUBLE, j, 42, cw_dup);
        PMPI_Recv(&remoteTime, 1, MPI_DOUBLE, j, 42, cw_dup, MPI_STATUS_IGNORE);
        endTime = getTime();
        allTimes[j * NTIMES + i] = remoteTime - (localTime + endTime) / 2;
      }
    } else {
      PMPI_Recv(&remoteTime, 1, MPI_DOUBLE, 0, 42, cw_dup, MPI_STATUS_IGNORE);
      localTime = getTime();
      PMPI_Send(&localTime, 1, MPI_DOUBLE, 0, 42, cw_dup);
    }
  }
  if (rank == 0) {
    for (int i = 0; i < size; i++) {
      for (int j = 0; j < NTIMES; j++)
        allTimes[i * NTIMES + j] -= localTimes[j];
      std::sort(allTimes.begin() + i * NTIMES,
                allTimes.begin() + i * NTIMES + NTIMES);
      timeOffsets[i] = allTimes[i * NTIMES + NTIMES / 2];
    }
  }
  PMPI_Bcast(timeOffsets.begin(), size, MPI_DOUBLE, 0, cw_dup);
  localTimeOffset = localTime = timeOffsets[rank];
  if (rank == 0 && analysis_flags->verbose) {
    printf("Timer Offsets:");
    for (auto &v : timeOffsets) {
      v -= localTime;
      printf("%lf, ", v);
    }
    printf("\n");
  }
}

void init_processes(mpiTimer &mt) {
  PMPI_Comm_rank(MPI_COMM_WORLD, &myProcId);
  if (analysis_flags->verbose) {
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    PMPI_Get_processor_name(processor_name, &name_len);
    printf("Process %i at %s\n", myProcId, processor_name);
  }
  init_timer_offsets();
  if (!analysis_flags->running) {
    startTool(false, STATE_MPI);
  }
}

/************************
 *
 *  MPI WRAPPER FUNCTIONS
 *
 ************************/

extern "C" {

int MPI_Finalize(void) {
  mpiTimer mt{false, __func__};
  if (analysis_flags->running)
    DCHECK_EQ(thread_local_clock->GetState(), STATE_MPI);
  else
    DCHECK_EQ(thread_local_clock->GetState(), STATE_INIT);
  ipcData max_uc;
  MpiHappensBefore(max_uc, REF_RANK);
  max_uc.Allreduce(cf.findData(MPI_COMM_WORLD));
  thread_local_clock->clocks[CLOCK_USEFUL].critical.maxUpdate(
      BaseMetric{max_uc.values[0]});

  finishMeasurement();
  analysis_flags->running = false;
  ipcData::finiIpcData();
  return PMPI_Finalize();
}

/************************
 * Pcontrol
 ************************/

int MPI_Pcontrol(const int level, ...) {
  if (useMpi == false) {
    printf("MPI_Pcontrol called before MPI_Init or something is wrong with "
           "Fortran setup\n");
    MPI_Abort(MPI_COMM_SELF, -1);
  }
  if (level == 1) {
    if (analysis_flags->barrier)
      MPI_Barrier(MPI_COMM_WORLD);
    startTool();
  } else if (level == 0) {
    if (analysis_flags->barrier)
      MPI_Barrier(MPI_COMM_WORLD);
    stopTool();
  }
  return MPI_SUCCESS;
}

/* =============== Fortran Wrappers for MPI_Pcontrol =============== */
static void MPI_Pcontrol_fortran_wrapper(MPI_Fint *level) {
  MPI_Pcontrol(*level);
}

void MPI_PCONTROL(MPI_Fint *level) { MPI_Pcontrol_fortran_wrapper(level); }

void mpi_pcontrol(MPI_Fint *level) { MPI_Pcontrol_fortran_wrapper(level); }

void mpi_pcontrol_(MPI_Fint *level) { MPI_Pcontrol_fortran_wrapper(level); }

void mpi_pcontrol__(MPI_Fint *level) { MPI_Pcontrol_fortran_wrapper(level); }

/* ================= End Wrappers for MPI_Pcontrol ================= */

} // extern "C"

// #include "fortran-wrapper.h"
