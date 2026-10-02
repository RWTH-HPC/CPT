include(TestHelperMacros)

OPTION(LIT_FORWARD_FULL_ENV "Lit should forward all environmental variables to test execution" FALSE)
option(CPT_DISABLE_Fortran_TESTS "Build OTF-CPT with pmpi Fortran wrappers" ${CPT_HAVE_Fortran})

set(shipped_lit_executable "${CMAKE_SOURCE_DIR}/external/llvm-lit/lit.py")
if (EXISTS "${shipped_lit_executable}")
    # Use shipped lit if available but still respect user-defined paths to the executable.
    set(LLVM_LIT_PATH "${shipped_lit_executable}" CACHE FILEPATH "llvm-lit executable.")
endif()
unset(shipped_lit_executable)

set(CPT_Test_avail TRUE)

find_package(LIT 12.0.0)

if(NOT LIT_FOUND)
    message(STATUS "Could not find llvm's lit executable needed for testing. \
Please make sure that either the LLVM package is in your CMake module search \
path or that the executable is in your PATH environment variable. \
Alternatively you might want to set the CMake variable LLVM_TOOLS_BINARY_DIR \
to the path containing the lit executable.")
set(CPT_Test_avail FALSE)
endif()

set(shipped_filecheck_executable "$<TARGET_FILE:FileCheck_Standalone>")
set(BUILD_FILECHECK FALSE)
if (TARGET FileCheck_Standalone)
    # Use shipped FileCheck if available but still respect user-defined paths to the binary.
    set(LLVM_FILECHECK_PATH "${shipped_filecheck_executable}" CACHE FILEPATH "Filecheck executable.")

    if ("${LLVM_FILECHECK_PATH}" STREQUAL "${shipped_filecheck_executable}")
        set(BUILD_FILECHECK TRUE)
    endif()
endif()

find_package(FileCheck)

if(NOT FileCheck_FOUND)
    message(STATUS "Could not find llvm's FileCheck executable needed for \
    testing. Please make sure that either the LLVM package is in your CMake \
    module search path or that the executable is in your PATH environment \
    variable. Alternatively you might want to set the CMake variable \
    LLVM_TOOLS_BINARY_DIR to the path containing the FileCheck executable.")
set(CPT_Test_avail FALSE)
endif()

include_directories(./)

set(CPT_TEST_UB 1.01 CACHE STRING "Sets an upper bound for the relative error.")
set(CPT_TEST_LB 0.95 CACHE STRING "Sets a lower bound for the relative error.")
set(CPT_TEST_WORK 100000 CACHE STRING "Sets the time in micro seconds used to emulate work.")

add_compile_definitions(UB_FAK=${CPT_TEST_UB} LB_FAK=${CPT_TEST_LB} WORK=${CPT_TEST_WORK})

add_custom_target(build-tests)
#add_custom_target(build-mpi-only-tests)
add_custom_target(build-omp-only-tests)
#add_custom_target(build-hybrid-tests)
add_custom_target(build-tasking-tests)
add_custom_target(build-tapp-tests)
add_custom_target(build-container-tests)
add_dependencies(build-omp-only-tests CPT CPT_omp)
#add_dependencies(build-mpi-only-tests CPT CPT_omp)
#add_dependencies(build-hybrid-tests CPT CPT_omp)
add_dependencies(build-tasking-tests CPT CPT_omp)
add_dependencies(build-tapp-tests CPT CPT_omp)
#add_dependencies(build-tests build-mpi-only-tests build-omp-only-tests build-hybrid-tests build-tasking-tests build-tapp-tests build-container-tests)
add_dependencies(build-tests build-omp-only-tests build-tasking-tests build-tapp-tests build-container-tests)
if (TARGET FileCheck_Standalone AND BUILD_FILECHECK)
    add_dependencies(build-tests FileCheck_Standalone)
    add_dependencies(build-omp-only-tests FileCheck_Standalone)
    add_dependencies(build-tasking-tests FileCheck_Standalone)
    add_dependencies(build-tapp-tests FileCheck_Standalone)
#    add_dependencies(build-mpi-only-tests FileCheck_Standalone)
#    add_dependencies(build-hybrid-tests FileCheck_Standalone)
endif()
#IF(CMAKE_Fortran_COMPILER)
#    add_custom_target(build-fortran-tests)
#    add_dependencies(build-tests build-fortran-tests)
#    add_subdirectory(fortran)
#ENDIF()


#add_subdirectory(datastructures)
#add_subdirectory(mpi-only)

add_subdirectory(containers)
if(OpenMP_C_FOUND)
#    add_subdirectory(hybrid)
    add_subdirectory(omp-only)
    add_subdirectory(tasking)
    add_subdirectory(TaPP)
endif()

if(NOT CPT_Test_avail)
set(CPT_Test_avail FALSE)
    message(STATUS "No tests available with missing lit/FileCheck")
    return()
endif()


set(FILECHECK_EXECUTABLE "${LLVM_FILECHECK_PATH}")
set(LIT_EXECUTABLE "${LLVM_LIT_PATH}")
set(CPT_LIB $<TARGET_FILE:CPT>)
set(CPT_OMP_LIB $<TARGET_FILE:CPT_omp>)

set(LIT_TEST_WORKERS 1 CACHE STRING "Sets the amount of workers when running tests. (Note that this only sets the amount of lit workers of which each might spawn more processes.)")

set(TEST_C_COMPILER ${CMAKE_C_COMPILER} CACHE STRING "C Compiler to use for tests")
set(TEST_CXX_COMPILER ${CMAKE_CXX_COMPILER} CACHE STRING "C++ Compiler to use for tests")
set(TEST_Fortran_COMPILER ${CMAKE_Fortran_COMPILER} CACHE STRING "Fortran Compiler to use for tests")
set(TEST_OPENMP_FLAGS ${OpenMP_C_FLAGS})
mark_as_advanced(TEST_C_COMPILER TEST_CXX_COMPILER TEST_Fortran_COMPILER)

#list(TRANSFORM MPI_C_INCLUDE_DIRS PREPEND "-I" OUTPUT_VARIABLE _incs)
#list(TRANSFORM MPI_C_COMPILE_DEFINITIONS PREPEND "-D" OUTPUT_VARIABLE _defs)
#string(JOIN " " MPI_C_FLAGS ${_incs} ${_defs} ${MPI_C_COMPILE_OPTIONS} ${MPI_C_LIBRARIES})
#set(TEST_MPI_C_FLAGS ${MPI_C_FLAGS})

#list(TRANSFORM MPI_CXX_INCLUDE_DIRS PREPEND "-I" OUTPUT_VARIABLE _incs)
#list(TRANSFORM MPI_CXX_COMPILE_DEFINITIONS PREPEND "-D" OUTPUT_VARIABLE _defs)
#string(JOIN " " MPI_CXX_FLAGS ${_incs} ${_defs} ${MPI_CXX_COMPILE_OPTIONS} ${MPI_CXX_LIBRARIES})
#set(TEST_MPI_CXX_FLAGS ${MPI_CXX_FLAGS})

#list(TRANSFORM MPI_Fortran_INCLUDE_DIRS PREPEND "-I" OUTPUT_VARIABLE _incs)
#list(TRANSFORM MPI_Fortran_COMPILE_DEFINITIONS PREPEND "-D" OUTPUT_VARIABLE _defs)
#string(JOIN " " MPI_Fortran_FLAGS ${_incs} ${_defs} ${MPI_Fortran_COMPILE_OPTIONS} ${MPI_Fortran_LIBRARIES})
#set(TEST_MPI_Fortran_FLAGS ${MPI_Fortran_FLAGS})

pythonize_bool(OpenMP_FOUND)
pythonize_bool(LIT_FORWARD_FULL_ENV)
pythonize_bool(CPT_HAVE_Fortran)
pythonize_bool(CPT_ENABLE_FORTRAN_TESTS)

configure_file(lit.site.cfg.in lit.site.cfg.configured @ONLY)
file(GENERATE
     OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg"
     INPUT "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.configured"
)


add_check_target(NAME check SUITES . COMMENT "Run all OTF-CPT tests" DEPENDS build-tests)
add_check_target(NAME check-omp SUITES omp-only COMMENT "Run omp tests" DEPENDS build-omp-only-tests)
#add_check_target(NAME check-mpi SUITES mpi-only COMMENT "Run mpi tests" DEPENDS build-mpi-only-tests)
#add_check_target(NAME check-hybrid SUITES hybrid COMMENT "Run hyrbid tests" DEPENDS build-hybrid-tests)
add_check_target(NAME check-tasking SUITES tasking COMMENT "Run tasking-tests tests" DEPENDS build-tasking-tests)
add_check_target(NAME check-tapp SUITES TaPP COMMENT "Run TaPP-tests tests" DEPENDS build-tapp-tests)
