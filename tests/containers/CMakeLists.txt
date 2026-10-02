set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Output/)
set(CMAKE_EXECUTABLE_SUFFIX_C .c.tmp)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .cpp.tmp)

set(containerTests
    array
)

include_directories(../../)

FOREACH(tests ${containerTests})
    add_executable(${tests} ${tests}.cpp ../../debug.cpp ../../parse_flags.cpp)
    add_dependencies(build-container-tests ${tests})
ENDFOREACH()