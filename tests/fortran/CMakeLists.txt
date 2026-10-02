set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Output/)

option(CPT_ENABLE_FORTRAN_TESTS "Enable Fortran tests" ${CPT_HAVE_Fortran})
if(NOT CPT_ENABLE_FORTRAN_TESTS)
  return()
endif()

if(NOT CPT_ENABLE_FORTRAN_WRAPPERS)
  enable_language(Fortran)
  find_package(MPI REQUIRED)
endif()

set(fortranTests
    mpi-balanced 
    mpi-imbalance
    #mpi-message
    #mpi-serialization
    #mpi-bcast-balanced
    #mpi-bcast-serialization
)

add_library(m_time OBJECT m_time.f90)

FOREACH(tests ${fortranTests})
        set_source_files_properties(${tests}.F90 PROPERTIES Fortran_PREPROCESS ON)
        add_executable(${tests}.f90.tmp ${tests}.F90 ../expected-metrics.c)
        target_link_libraries(${tests}.f90.tmp PRIVATE CPT)
        target_link_libraries(${tests}.f90.tmp PUBLIC MPI::MPI_Fortran)
        target_link_libraries(${tests}.f90.tmp PRIVATE m_time)
        add_dependencies(build-fortran-tests ${tests}.f90.tmp)
ENDFOREACH()

