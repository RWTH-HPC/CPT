#include <mpi.h>
#include <stdio.h>

#include "fiberpool.h"
#include "tracking.h"
#include "criticalPath.h"
#include "mpi-critical.h"

{{fn fn_name MPI_Init MPI_Init_thread}}
  InitializeOtfcptFlags();
  if (analysis_flags->verbose)
     printf("Starting critPathAnalysis tool\n");
  useMpi = true;

  mpiTimer mt{true, __func__};

  if (analysis_flags->verbose)
     printf("MPI Init\n");
   {{ret_val}} = P{{fn_name}}({{args}});

#ifdef USE_ERRHANDLER
  if (analysis_flags->stacktrace) {
    createErrHandler();
    registerErrHandler(MPI_COMM_WORLD);
  }
#endif
#ifdef HANDLE_WIN
   wf.initPredefined();
#endif
#ifdef HANDLE_FILE
   ff.initPredefined();
#endif
#ifdef HANDLE_COMM
   cf.initPredefined();
#endif
#ifdef HANDLE_REQUEST
   rf.initPredefined();
#endif
#if defined(HAVE_SESSION) && defined(HANDLE_SESSION)
   sf.initPredefined();
#endif
  ipcData::initIpcData();
  init_processes(mt);
{{endfn}}

{{fnallntype fn_name MPI_Request MPI_Comm_free MPI_Comm_disconnect MPI_Win_free MPI_Session_finalize 
 MPI_Finalize MPI_Init MPI_Init_thread MPI_Pcontrol
 MPI_Barrier MPI_Barrier_c 
 MPI_Allgather MPI_Allgather_c MPI_Allgatherv MPI_Allgatherv_c 
 MPI_Alltoall MPI_Alltoall_c MPI_Alltoallv MPI_Alltoallv_c MPI_Alltoallw MPI_Alltoallw_c
 MPI_Allreduce MPI_Allreduce_c 
 MPI_Reduce_scatter_block MPI_Reduce_scatter_block_c MPI_Reduce_scatter MPI_Reduce_scatter_c 
 MPI_Scan MPI_Scan_c MPI_Exscan MPI_Exscan_c 
 MPI_Neighbor_alltoallw MPI_Neighbor_alltoallw_c 
 MPI_Neighbor_allgather MPI_Neighbor_allgather_c 
 MPI_Neighbor_allgatherv MPI_Neighbor_allgatherv_c 
 MPI_Neighbor_alltoall MPI_Neighbor_alltoall_c 
 MPI_Neighbor_alltoallv MPI_Neighbor_alltoallv_c 
 MPI_Gather MPI_Gather_c MPI_Gatherv MPI_Gatherv_c 
 MPI_Reduce MPI_Reduce_c 
 MPI_Bcast MPI_Bcast_c 
 MPI_Scatter MPI_Scatter_c MPI_Scatterv MPI_Scatterv_c 
 MPI_Bsend MPI_Rsend MPI_Ssend MPI_Send MPI_Recv 
 MPI_Bsend_c MPI_Rsend_c MPI_Ssend_c MPI_Send_c MPI_Recv_c 
 MPI_Mrecv MPI_Mrecv_c MPI_Mprobe MPI_Improbe
 MPI_Sendrecv MPI_Sendrecv_c MPI_Sendrecv_replace MPI_Sendrecv_replace_c
 MPI_Isendrecv MPI_Isendrecv_c MPI_Isendrecv_replace MPI_Isendrecv_replace_c
 }}
   mpiTimer mt{false, __func__};
   {{apply_to_type MPI_Comm preComm}} 
   {{ret_val}} = P{{fn_name}}({{args}});
   {{apply_to_type MPI_Comm* errHandlerComm}}
   {{apply_to_type MPI_Comm* postComm}} 
{{endfnallntype}}
