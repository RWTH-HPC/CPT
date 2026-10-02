cmake_minimum_required(VERSION 3.13.4...3.27.4)

project(CPT C CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Installation directory" FORCE)
  message(WARNING "No build type selected. OTF-CPT will be built as Release.
Available options are:
  * -DCMAKE_BUILD_TYPE=Release - For an optimized build with no assertions or debug info.
  * -DCMAKE_BUILD_TYPE=Debug - For an unoptimized build with assertions and debug info.
  * -DCMAKE_BUILD_TYPE=RelWithDebInfo - For an optimized build with no assertions but with debug info.
  * -DCMAKE_BUILD_TYPE=MinSizeRel - For a build optimized for size instead of speed.
Learn more about these options in LLVM's documentation at https://llvm.org/docs/CMake.html#cmake-build-type
")
endif()

SET(CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmakemodules"
    ${CMAKE_MODULE_PATH}
    )

include(TestMpiFeatures)
set_compiler_test_features()

include(CheckLanguage)

find_package(OpenMP REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# By default, 1 integer dependent metric and 15 double dependent metrics can be measured alongside the CP. If -DNUM_DEP_INT_METRICS=X or -DNUM_DEP_DBL_METRICS=Y is passed to cmake call, user can change these values to X and Y, respectively.
set(NUM_DEP_INT_METRICS 1 CACHE STRING "Sets how many integer dependent metrics can be measured alongside the CP.")
set(NUM_DEP_DBL_METRICS 15 CACHE STRING "Sets how many double dependent metrics can be measured alongside the CP.")

###Configure cptConfig.h
configure_file(cptConfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/cptConfig.h")

include_directories(${CPT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

set(CPT_OMP_SOURCES
    critical-core.cpp
    ompt-critical.cpp
    fiberpool.cpp
    parse_flags.cpp
    debug.cpp
    dependentMetrics.cpp
)

option(CPT_DEBUG_CLOCKS "Detailed clock printing at every sync point" OFF)
if (CPT_DEBUG_CLOCKS)
  add_compile_definitions(-DDEBUG_CLOCKS)
endif()

list(APPEND GnuCompatible "Clang" "IntelLLVM" "GNU")
list(APPEND ClangCompatible "Clang" "IntelLLVM")
option(CPT_USE_BACKWARD "Use Backward-CPP for stacktraces" OFF)
if ("${CMAKE_CXX_COMPILER_ID}" IN_LIST GnuCompatible)
  set(NOSTDLIB_FLAG "")
  # set(NOSTDLIB_FLAG "-nostdlib++")
  #if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
    #set(NOSTDLIB_FLAG -nodefaultlibs -lc)
  #endif ()
  set(STDCPLUS_COMPILE_FLAGS -fno-rtti -fno-exceptions -ffunction-sections -fdata-sections)
  set(STDCPLUS_LINK_FLAGS -fno-rtti -fno-exceptions ${NOSTDLIB_FLAG} -Wl,--gc-sections)

  if ("${CMAKE_CXX_COMPILER_ID}" IN_LIST ClangCompatible)
    option(CPT_USE_LLVM_LIBCPP "Use LLVM libc++, which might reduce portability." OFF)
    if (CPT_USE_LLVM_LIBCPP)
      message(STATUS "Building with LLVM libc++ was requested, which might limit portability and compatibility with application/library code compiled with gcc.")
      set(STDCPLUS_COMPILE_FLAGS -stdlib=libc++)
      set(STDCPLUS_LINK_FLAGS -stdlib=libc++)
      if(CPT_USE_BACKWARD)
        set(LINK_EXTERNAL_LIBRARIES Backward::Interface)
        add_compile_definitions(-DUSE_BACKWARD)
      endif()
      add_compile_definitions(-DUSE_STL)
    else ()
      message(STATUS "For portability, OTF-CPT is built without dependency to a C++ runtime library")
      set(STDCPLUS_COMPILE_FLAGS ${STDCPLUS_COMPILE_FLAGS} -stdlib=libstdc++)
      set(STDCPLUS_LINK_FLAGS ${STDCPLUS_LINK_FLAGS} -stdlib=libstdc++)
    endif ()
  endif ()
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(STDCPLUS_COMPILE_FLAGS -fno-exceptions)
#  set(STDCPLUS_LINK_FLAGS -cxxlib-nostd)
endif ()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include_directories(external/ompt)
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
  if (CPT_USE_BACKWARD)
    add_compile_definitions(-DUSE_BACKWARD)
    set(LINK_EXTERNAL_LIBRARIES Backward::Interface)
  endif()
else()
  if(CPT_USE_BACKWARD AND NOT CPT_USE_LLVM_LIBCPP)
    message(WARNING "CPT_USE_GNU_LIBCPP or CPT_USE_LLVM_LIBCPP should be enabled in order to use BackwardCPP, ignoring.")
  endif()
endif()

set(WRAP_FLAGS -s -n)

if(CPT_ENABLE_FORTRAN_WRAPPERS)
  set(WRAP_FLAGS -n -f)
endif()

if (CPT_USE_BACKWARD)
  add_subdirectory(${CMAKE_SOURCE_DIR}/external/backward-cpp)
endif()

add_library(CPT_omp SHARED ${CPT_OMP_SOURCES})
add_library(CPT SHARED ${CPT_OMP_SOURCES})
target_link_libraries(CPT PUBLIC CPT_omp)
target_link_libraries(CPT_omp PUBLIC ${LINK_EXTERNAL_LIBRARIES})
target_link_options(CPT_omp PUBLIC ${STDCPLUS_LINK_FLAGS} "LINKER:-as-needed")
target_compile_options(CPT_omp PUBLIC ${STDCPLUS_COMPILE_FLAGS})

add_library(CPT_nohb SHARED ${CPT_OMP_SOURCES})
target_link_libraries(CPT_nohb PUBLIC ${LINK_EXTERNAL_LIBRARIES})
target_compile_definitions(CPT_nohb PUBLIC -DNOHB=1)
target_link_options(CPT_nohb PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>" "LINKER:-as-needed")
target_compile_options(CPT_nohb PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(CPT_nohb_notime SHARED ${CPT_OMP_SOURCES})
target_link_libraries(CPT_nohb_notime PUBLIC ${LINK_EXTERNAL_LIBRARIES})
target_compile_definitions(CPT_nohb_notime PUBLIC -DNOHB=1 -DNOTIME=1)
target_link_options(CPT_nohb_notime PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>" "LINKER:-as-needed")
target_compile_options(CPT_nohb_notime PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(CPT_empty SHARED ${CPT_OMP_SOURCES})
target_link_libraries(CPT_empty PUBLIC ${LINK_EXTERNAL_LIBRARIES})
target_compile_definitions(CPT_empty PUBLIC -DEMPTYCB=1)
target_link_options(CPT_empty PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_LINK_FLAGS}>" "LINKER:-as-needed")
target_compile_options(CPT_empty PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${STDCPLUS_COMPILE_FLAGS}>")

add_library(KMPC SHARED kmpc-intercept.c)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/external/FileCheck/CMakeLists.txt)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/external/FileCheck EXCLUDE_FROM_ALL)
endif()

add_subdirectory(tests)


install(TARGETS 
        CPT_omp
	CPT_nohb
	CPT_nohb_notime
	CPT_empty
        )
