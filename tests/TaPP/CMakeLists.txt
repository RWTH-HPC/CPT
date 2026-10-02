if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  return()
endif()

set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(CMAKE_EXECUTABLE_SUFFIX_C .c.tmp)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .cpp.tmp)

set(tappTests
    coarsegrained_single_creator
    independent_uniform_create_chains
    independent_uniform_dep_chains
    independent_uniform_taskwait_chains
    tree_create_chains
    tree_dep_chains
    tree_taskwait_chains
)

FOREACH(tests ${tappTests})
    add_executable(${tests} ${tests}.c delay.c)
    target_include_directories(${tests} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${tests} PUBLIC OpenMP::OpenMP_C m)
    add_dependencies(build-tapp-tests ${tests})
ENDFOREACH()
