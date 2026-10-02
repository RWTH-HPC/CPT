if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  return()
endif()

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(CMAKE_EXECUTABLE_SUFFIX_C .c.tmp)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .cpp.tmp)

set(taskingTests
     tasking-equal-for-one
     omp-tasking-imbalance
     tasking-simple-depend
     tasking-multiple-parallel-regions
     omp-barrier-simple
     omp-barrier-taskwait
     omp-undeferred
     omp-taskgroup
     omp-final
     omp-mergeable
     omp-taskwait-barrier
     omp-taskwait-deps
     omp-taskwait-simple
)

FOREACH(tests ${taskingTests})
    add_executable(${tests} ${tests}.c)
    target_link_libraries(${tests} PUBLIC OpenMP::OpenMP_C)
    add_dependencies(build-tasking-tests ${tests})
ENDFOREACH()
