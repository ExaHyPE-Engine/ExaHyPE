cmake_minimum_required(VERSION 3.20)

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH_SHORT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  set(GIT_BRANCH "")
  set(GIT_COMMIT_HASH "")
  set(GIT_COMMIT_HASH_SHORT "")
endif()

set(META_PROJECT_NAME "Peano")
set(META_PROJECT_DESCRIPTION
  "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes"
)
set(META_AUTHOR_ORGANIZATION
  "Durham University and Technische Universitaet Muenchen")
set(META_AUTHOR_DOMAIN "www.peano-framework.org")
set(META_VERSION_REVISION "${GIT_COMMIT_HASH_SHORT}")
set(META_GIT_BRANCH "${GIT_BRANCH}")
set(META_GIT_HASH "${GIT_COMMIT_HASH}")
set(META_GIT_BUGREPORT "https://gitlab.lrz.de/hpcsoftware/Peano/-/issues")
set(META_GIT_URL "https://gitlab.lrz.de/hpcsoftware/Peano")
message(STATUS "On Git Branch: ${GIT_BRANCH} (${GIT_COMMIT_HASH})")
set(META_COMPILER_VERSION "${CMAKE_SYSTEM_NAME} ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${META_PROJECT_NAME})

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(OPT_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
include(FetchContent)
FetchContent_Declare(
    _project_options
    GIT_REPOSITORY https://github.com/aminya/project_options.git
    GIT_TAG main
)
FetchContent_MakeAvailable(_project_options)

set(LANGUAGES C CXX)
include(CheckLanguage)
check_language(Fortran)
if(CMAKE_Fortran_COMPILER)
  list(APPEND LANGUAGES Fortran)
  enable_language(Fortran)
endif()

project(
  ${META_PROJECT_NAME}
  DESCRIPTION ${META_PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${META_AUTHOR_DOMAIN}
  LANGUAGES ${LANGUAGES})
file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")

include(${_project_options_SOURCE_DIR}/Index.cmake)
include(${_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)
dynamic_project_options()
target_compile_features(project_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

include(GNUInstallDirs)
set(DEFAULT_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(CMake)
include(MessageUtils)
#include(Clang-Format)
include(TestBigEndian)
test_big_endian(BIG_ENDIAN)
include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)
find_package(Threads REQUIRED)
include(CTest)
# https://stackoverflow.com/questions/31355692/how-do-i-enable-link-time-optimization-lto-with-cmake
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION 1)
else()
  message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
endif()

include(CheckIncludeFiles)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
check_include_files(omp.h HAVE_OMP_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdio.h HAVE_STDIO_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)

# TODO:
# Additionally check the headers defined above have C90 standards
# Requires "stddef.h;float.h"
# We also need free in stdlib, memchr in string
# An example where how to implement the check is here:
# https://github.com/LuaDist/libgd/tree/master/cmake/modules

list(APPEND TOOLCHAINS " " "intel" "nvidia" "scorep" "chrome" "gnu")
set(WITH_TOOLCHAIN "" CACHE STRING "${TOOLCHAINS}")
set_property(CACHE WITH_TOOLCHAIN PROPERTY STRINGS ${TOOLCHAINS})

list(APPEND MULTICORES_BACKENDS " " "cpp" "omp" "tbb")
set(WITH_MULTITHREADING " " CACHE STRING "${MULTICORES_BACKENDS}")
set_property(CACHE WITH_MULTITHREADING PROPERTY STRINGS ${MULTICORES_BACKENDS})
if(WITH_MULTITHREADING)
  string(TOUPPER ${WITH_MULTITHREADING} WITH_MULTITHREADING_UPPER)
  set(Shared${WITH_MULTITHREADING_UPPER} 1)
endif()

if(WITH_MULTITHREADING STREQUAL "omp")
  find_package(OpenMP REQUIRED)
  target_link_libraries(project_options INTERFACE OpenMP::OpenMP_CXX)
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
endif()

list(APPEND ACCELERATOR_BACKENDS " " "cpp" "omp" "sycl" "hip")
set(WITH_GPU "" CACHE STRING "${ACCELERATOR_BACKENDS}")
set_property(CACHE WITH_GPU PROPERTY STRINGS ${ACCELERATOR_BACKENDS})
if(WITH_GPU)
  string(TOUPPER ${WITH_GPU} WITH_GPU_UPPER)
  set(GPUOffloading${WITH_GPU_UPPER} 1)
endif()

list(APPEND ACCELERATOR_ARCHITECTURES
  " "
  "sm_20"
  "sm_30"
  "sm_35"
  "sm_37"
  "sm_50"
  "sm_53"
  "sm_52"
  "sm_60"
  "sm_61"
  "sm_62"
  "sm_70"
  "sm_72"
  "sm_75"
  "sm_80"
  "sm_86"
  "sm_90"
  "gfx906"
  "gfx908"
  "gfx90a"
)
set(WITH_GPU_ARCH "" CACHE STRING "GPU architecture sm_XX or gfxXXX")
list(APPEND WITH_GPU_ARCH ${ACCELERATOR_ARCHITECTURES})
set_property(CACHE WITH_GPU_ARCH PROPERTY STRINGS ${ACCELERATOR_ARCHITECTURES})

string(SUBSTRING "${WITH_GPU_ARCH}" 0 3 GPU_PREFIX)
if("${GPU_PREFIX}" STREQUAL "sm_")
  set(NVIDIA_GPU TRUE)
elseif("${GPU_PREFIX}" STREQUAL "gfx")
  set(AMD_GPU TRUE)
endif()

if(WITH_GPU STREQUAL "omp")
  find_package(OpenMP REQUIRED)
  find_package(OpenMPWithOffloading REQUIRED)
  target_link_libraries(project_options INTERFACE OpenMP::OpenMP_CXX)
  target_link_libraries(project_options INTERFACE OpenMP::OpenMP_CXX_with_offloading)
elseif(WITH_GPU STREQUAL "cpp")
  if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    target_link_libraries(project_options "-mp=gpu -gpu=${WITH_GPU_ARCH}")
    target_compile_options(project_options "-mp=gpu -gpu=${WITH_GPU_ARCH}")
  endif()
elseif(WITH_GPU STREQUAL "sycl")
elseif(WITH_GPU STREQUAL "hip")
endif()

#set(CMAKE_CXX_FLAGS "")
#set(CMAKE_CXX_FLAGS_RELEASE "")
#set(CMAKE_CXX_FLAGS_DEBUG "")
#set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "")
#set(CMAKE_CXX_FLAGS_MINSIZEREL "")
if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  set(RELEASE_FLAGS -O4)
else()
  set(RELEASE_FLAGS -O3)
endif()
set(RELEASE_DEFINITIONS -DPeanoDebug=0 -DNDEBUG)
if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  set(STATS_FLAGS -O4)
else()
  set(STATS_FLAGS -O3)
endif()
set(STATS_DEFINITIONS -DPeanoDebug=0 -DTrackStatistics)
set(ASSERTS_FLAGS -g -O0)
set(ASSERTS_DEFINITIONS -DPeanoDebug=2)
set(TRACE_FLAGS -g -O2)
set(TRACE_DEFINITIONS -DPeanoDebug=1 -DNDEBUG)
set(DEBUG_FLAGS -g -O0)
set(DEBUG_DEFINITIONS -DPeanoDebug=4)

set(RELEASE_LDFLAGS -g)
set(STATS_LDFLAGS -g)
set(ASSERTS_LDFLAGS -g)
set(TRACE_LDFLAGS -g)
set(DEBUG_LDFLAGS -g)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "Asserts" "Trace" "Stats" CACHE STRING "Enabled build types" FORCE)
if(NOT CMAKE_BUILD_TYPE OR NOT ${CMAKE_BUILD_TYPE} IN_LIST CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "" CACHE STRING "${CMAKE_CONFIGURATION_TYPES}" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
  list(APPEND BUILD_MODES ${CMAKE_CONFIGURATION_TYPES})
else()
  list(APPEND BUILD_MODES ${CMAKE_BUILD_TYPE})
endif()

#set(WITH_DIMENSIONS "2 3" CACHE STRING)
option(WITH_MPI "With MPI" OFF)
option(WITH_OTTER "Enable Otter" OFF)
option(ENABLE_EXAHYPE "Enable ExaHyPE2" OFF)
option(ENABLE_FEM "Enable the finite elements toolbox" OFF)
option(ENABLE_MULTIPRECISION "Enable the multi-precision toolbox" OFF)
option(ENABLE_LOADBALANCING "Enable the loadbalancing toolbox" OFF)
option(ENABLE_PARTICLES "Enable the particles toolbox" OFF)
option(ENABLE_BLOCKSTRUCTURED "Enable the blockstructured toolbox" OFF)

if(WITH_MPI)
  find_package(MPI REQUIRED)
  message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
  include_directories(SYSTEM ${MPI_INCLUDE_PATH})
  add_compile_definitions(OMPI_SKIP_MPICXX) # To prevent compiler warnings
  add_compile_definitions(Parallel)
  set(Parallel 1)
endif()

if(WITH_OTTER)
  find_package(OTF2 REQUIRED)
  find_package(Otter REQUIRED)
  set(UseOtter 1)
endif()

if(ENABLE_EXAHYPE)
  if(NOT ENABLE_LOADBALANCING)
    set(ENABLE_LOADBALANCING CACHE ON)
  endif()

  if(NOT ENABLE_BLOCKSTRUCTURED)
    set(ENABLE_BLOCKSTRUCTURED CACHE ON)
  endif()

  set(UseExaHyPE 1)
endif()

file(COPY "${PROJECT_SOURCE_DIR}/applications" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${PROJECT_SOURCE_DIR}/benchmarks" DESTINATION "${CMAKE_BINARY_DIR}")
file(COPY "${PROJECT_SOURCE_DIR}/python" DESTINATION "${CMAKE_BINARY_DIR}")

file(TOUCH ${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)
include(${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/GeneratedSubdirectories.cmake)

add_subdirectory(src)

display_separator("Project Configuration Summary")

display_variable(AVAILABLE_PROCESSOR_COUNT)

display_variable(CMAKE_BUILD_TYPE)
display_variable(CMAKE_CONFIGURATION_TYPES)
display_variable(CMAKE_CXX_FLAGS)
display_variable(CMAKE_CXX_FLAGS_DEBUG)
display_variable(CMAKE_CXX_FLAGS_RELEASE)
display_variable(CMAKE_CXX_FLAGS_MINSIZEREL)
display_variable(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
display_variable(CMAKE_VERBOSE_MAKEFILE)

display_subseparator("Options to Build")
display_variable(BUILD_SHARED_LIBS)

display_subseparator("Options to Install")
display_variable(CMAKE_INSTALL_FULL_BINDIR)
display_variable(CMAKE_INSTALL_FULL_LIBDIR)

display_subseparator("End of Summary")
