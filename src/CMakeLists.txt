if(HAVE_MPI_SESSION)
    add_definitions(-DHAVE_SESSION)
endif()
if(HAVE_MPI_COUNT)
    add_definitions(-DHAVE_COUNT)
endif()
if(HAVE_MPI_BARRIER_INIT)
    add_definitions(-DHAVE_PCOLL)
endif()
if(HAVE_PMPI_ISENDRECV_REPLACE)
    add_definitions(-DHAVE_ISRR)
endif()

include_directories(${CPT_SOURCE_DIR}/include ${MPI_C_INCLUDE_PATH} ${MPI_CXX_INCLUDE_PATH})

set(CPT_SOURCES
    completion-wrappers.cpp
    critical-core.cpp
    man-wrappers.cpp
    mpi-critical.cpp
    ompt-critical.cpp
    tracking.cpp
    gen-nb-wrappers.cpp
    gen-wrappers.cpp
    parse_flags.cpp
    debug.cpp
)

set(CPT_OMP_SOURCES
    critical-core.cpp
    ompt-critical.cpp
    parse_flags.cpp
    debug.cpp
)

if(CPT_ENABLE_FORTRAN_WRAPPERS)
    add_definitions(-DFORTRAN_SUPPORT)
    list(APPEND CPT_SOURCES fort-wrappers.c)
endif()

option(CPT_DEBUG_CLOCKS "Detailed clock printing at every sync point" OFF)
if (CPT_DEBUG_CLOCKS)
  add_compile_definitions(-DDEBUG_CLOCKS)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include_directories(${CPT_SOURCE_DIR}/external/ompt)
endif()

option(CPT_USE_GNU_LIBCPP "Use GNU libstdc++, which might reduce portability." OFF)
if (CPT_USE_GNU_LIBCPP)
  if (CPT_USE_LLVM_LIBCPP)
    message(WARNING "CPT_USE_GNU_LIBCPP and CPT_USE_LLVM_LIBCPP are on at the same time, will build with GNU libstdc++")
  endif()
  if ("${CMAKE_CXX_COMPILER_ID}" IN_LIST ClangCompatible)
    set(STDCPLUS_COMPILE_FLAGS -stdlib=libstdc++)
    set(STDCPLUS_LINK_FLAGS -stdlib=libstdc++)
  else()
    set(STDCPLUS_COMPILE_FLAGS "")
    set(STDCPLUS_LINK_FLAGS "")
  endif()
  add_compile_definitions(-DUSE_STL)
endif()

if(CPT_USE_BACKWARD)
  set(LINK_EXTERNAL_LIBRARIES Backward::Interface)
  add_compile_definitions(USE_BACKWARD)
endif()

if (ENABLE_CUDA_SUPPORT)
  if (NOT TARGET CUDA::cupti)
    message(FATAL_ERROR "ENABLE_CUDA_SUPPORT requires CUPTI, but it was not found.")
  endif ()
  set(CPT_SOURCES ${CPT_SOURCES} cuda-tracking.cpp)
  set(CPT_OMP_SOURCES ${CPT_OMP_SOURCES} cuda-tracking.cpp)
endif ()

set(WRAP_FLAGS -s -n)

if(CPT_ENABLE_FORTRAN_WRAPPERS)
  set(WRAP_FLAGS -n -f)
endif()

add_wrapped_file(gen-nb-wrappers.cpp gen-nb-wrappers.w)
add_wrapped_file(gen-wrappers.cpp gen-wrappers.w)

add_library(CPT SHARED ${CPT_SOURCES})
target_link_libraries(CPT PUBLIC MPI::MPI_CXX ${LINK_EXTERNAL_LIBRARIES})
target_compile_definitions(CPT PUBLIC -DUSE_MPI=1)
if (ENABLE_CUDA_SUPPORT)
  target_link_libraries(CPT PUBLIC CUDA::cupti)
  target_compile_definitions(CPT PUBLIC -DUSE_CUDA=1)
endif ()
target_link_options(CPT PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>" "LINKER:-as-needed")
target_compile_options(CPT PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(CPT_omp SHARED ${CPT_OMP_SOURCES})
target_link_libraries(CPT_omp PUBLIC ${LINK_EXTERNAL_LIBRARIES})
if (ENABLE_CUDA_SUPPORT)
  target_link_libraries(CPT_omp PUBLIC CUDA::cupti)
  target_compile_definitions(CPT_omp PUBLIC -DUSE_CUDA=1)
endif ()
target_link_options(CPT_omp PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>" "LINKER:-as-needed")
target_compile_options(CPT_omp PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(CPT_static STATIC ${CPT_SOURCES})
target_link_libraries(CPT_static PUBLIC MPI::MPI_CXX ${LINK_EXTERNAL_LIBRARIES})
target_compile_definitions(CPT_static PUBLIC -DUSE_MPI=1)
if (ENABLE_CUDA_SUPPORT)
  target_link_libraries(CPT_static PUBLIC CUDA::cupti)
  target_compile_definitions(CPT_static PUBLIC -DUSE_CUDA=1)
endif ()
target_link_options(CPT_static PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>")
target_compile_options(CPT_static PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(KMPC SHARED kmpc-intercept.c)

install(TARGETS 
        CPT 
        CPT_omp
        CPT_static
        )

