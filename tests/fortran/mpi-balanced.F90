! ALLOW_RETRIES: 1
! RUN: %mpirunnp 2 %load_cpt %cpt_options_dump_stopped %t | %FileCheck \
! RUN: --check-prefixes=CHECK %metricfile
! RUN: %mpirunnp 4 %load_cpt %cpt_options_dump_stopped %t | %FileCheck \
! RUN: --check-prefixes=CHECK %metricfile
! REQUIRES: Fortran

program hello
   use mpi
   use M_time
   interface
      subroutine printMetrics(ulb, use, ute, mlb, mse, mte)
         integer,intent(in) :: ulb, use, ute, mlb, mse, mte
      end subroutine printMetrics
   end interface

   integer rank, size, ierror
   
   call MPI_INIT(ierror)
   call MPI_COMM_SIZE(MPI_COMM_WORLD, size, ierror)
   call MPI_COMM_RANK(MPI_COMM_WORLD, rank, ierror)
   if (rank.eq.0) then
     call printMetrics(1000, 1000, 1000, 1000, 1000, 1000);
   end if
   call MPI_BARRIER(MPI_COMM_WORLD, ierror)
   call MPI_PCONTROL(1)
   call usleep(WORK)
   call MPI_PCONTROL(0)
   call MPI_BARRIER(MPI_COMM_WORLD, ierror)
   
   call MPI_FINALIZE(ierror)
   
end program
