# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

cmake_minimum_required(VERSION 3.27)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_BRANCH GIT_COMMIT_HASH)
get_git_commit_message(GIT_COMMIT_MESSAGE)
get_git_commit_timestamp(GIT_COMMIT_TIMESTAMP)
message(STATUS "Revision: ${GIT_BRANCH} -m \"${GIT_COMMIT_MESSAGE}\" (${GIT_COMMIT_HASH}) ${GIT_COMMIT_TIMESTAMP}")

project(Peano
  DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes"
  LANGUAGES
    C
    CXX
    #Fortran
)

include(Ccache)
include(CheckIncludeFiles)
include(CheckIncludeFileCXX)

include(TestBigEndian)
test_big_endian(BIG_ENDIAN)

include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)

set(CMAKE_C_STANDARD 17 CACHE STRING "")
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "")
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")

include(GNUInstallDirs)
set(DEFAULT_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include(CTest)
enable_testing()

add_library(PeanoOptions INTERFACE)
target_compile_features(PeanoOptions INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
install(TARGETS PeanoOptions EXPORT ${PROJECT_NAME}Targets)

include(DetectCPUArch)
include(Mimalloc)
include(PeanoLibrary)
include(PeanoExecutable)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  target_compile_options(PeanoOptions INTERFACE
    -W
    -Wall
    -Wextra
    -Kieee
    #-Minfo=all
  )
else()
  target_compile_options(PeanoOptions INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wformat=2
    # -Wshadow
    # -Wconversion
    -Wcast-align
    -Wcast-qual
    -Wreturn-type
    -Wdouble-promotion
    -Wnull-dereference
    -Woverloaded-virtual
    -Wnon-virtual-dtor
    -Wno-unused-parameter
    -Wunused-but-set-variable
  )
endif()

option(WITH_MULTITHREADING "Build with multithreading (OpenMP) support" ON)
option(WITH_MPI "Build with Message Passing Interface (MPI) support" ON)
option(WITH_HDF5 "Build with HDF5 support" ON)
option(WITH_NETCDF "Build with NetCDF support" ON)
option(WITH_EASI "Build with easi support" OFF)
option(WITH_ZLIB "Build with ZLIB compression support" ON)
option(WITH_LIBXSMM "Build with LIBXSMM support" OFF)
option(WITH_ACPP "Build with AdaptiveCpp (SYCL) backend" OFF)
option(WITH_KOKKOS "Build with Kokkos backend" OFF)
option(WITH_OMPT "Build with OpenMP target backend" OFF)
option(WITH_USM "Build with unified shared memory support" OFF)

set(SUPPORTED_ACCELERATOR_ARCHITECTURES
  none
  sm_70 sm_72 sm_75 sm_80 sm_86 sm_87 sm_89 sm_90 sm_100 sm_103 sm_120 sm_121
  gfx906 gfx908 gfx90a gfx942 gfx950
  spir64
  CACHE STRING "Editable list of supported accelerator architectures"
)
mark_as_advanced(SUPPORTED_ACCELERATOR_ARCHITECTURES)

set(WITH_GPU_ARCH "none" CACHE STRING "${SUPPORTED_ACCELERATOR_ARCHITECTURES}")
set_property(CACHE WITH_GPU_ARCH PROPERTY STRINGS ${SUPPORTED_ACCELERATOR_ARCHITECTURES})

# TODO: Change to supported loggers
set(SUPPORTED_TOOLCHAINS
  no
  intel nvidia scorep chrome gnu
  CACHE STRING "Editable list of supported toolchains"
)
mark_as_advanced(SUPPORTED_TOOLCHAINS)

set(WITH_TOOLCHAIN "no" CACHE STRING "${SUPPORTED_TOOLCHAINS}")
set_property(CACHE WITH_TOOLCHAIN PROPERTY STRINGS ${SUPPORTED_TOOLCHAINS})

if(WITH_TOOLCHAIN STREQUAL "scorep")
  find_library(PAPI libpapi.a papi)
  find_library(CUBE4W libcube4w.a cube4w)
  target_link_libraries(PeanoOptions INTERFACE ${PAPI} ${CUBE4W} ${SCOREP_LIBRARIES})
  target_link_options(PeanoOptions INTERFACE ${SCOREP_LINK_OPTION})
  target_compile_options(PeanoOptions INTERFACE ${SCOREP_COMPILE_FLAGS})
  target_compile_definitions(PeanoOptions INTERFACE UseScoreP UseLogService=ScorePLogger)
elseif(WITH_TOOLCHAIN STREQUAL "itac")
  target_link_libraries(PeanoOptions INTERFACE VT::VT)
  target_include_directories(PeanoOptions SYSTEM INTERFACE ${VT_INCLUDE_DIRS})
  target_link_directories(PeanoOptions INTERFACE ${VT_LIBRARIES_DIR})
  target_compile_definitions(PeanoOptions INTERFACE UseITAC UseLogService=ITACLogger)
elseif(WITH_TOOLCHAIN STREQUAL "nvidia")
  check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(PeanoOptions INTERFACE CUDA::nvtx3)
  target_compile_definitions(PeanoOptions INTERFACE UseNVTX UseLogService=NVTXLogger)
endif()

option(ENABLE_TOOLBOX_PARTICLES "Enable the particles toolbox" ON)
option(ENABLE_TOOLBOX_BLOCKSTRUCTURED "Enable the blockstructured toolbox" ON)
option(ENABLE_TOOLBOX_LOADBALANCING "Enable the loadbalancing toolbox" ON)
option(ENABLE_TOOLBOX_CURVI "Enable the Curvi toolbox" OFF)

option(ENABLE_EXAHYPE "Enable the ExaHyPE 2 extension" ON)

set(SUPPORTED_DIMENSIONS
  2 3
  CACHE STRING "Editable list of supported dimensions"
)
mark_as_advanced(SUPPORTED_DIMENSIONS)

set(CMAKE_BUILD_TYPE "" CACHE INTERNAL "")
set(SUPPORTED_BUILD_TYPES
  Release Debug Trace Asserts
  CACHE STRING "Editable list of supported build types"
)
mark_as_advanced(SUPPORTED_BUILD_TYPES)

set(PEANO_CXX_FLAGS_RELEASE -O3 -fomit-frame-pointer CACHE INTERNAL "")
set(PEANO_RELEASE_DEFINITIONS -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_ASSERTS -g -O3 CACHE INTERNAL "")
set(PEANO_ASSERTS_DEFINITIONS -DPEANO_DEBUG=2 CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_TRACE -g -O2 CACHE INTERNAL "")
set(PEANO_TRACE_DEFINITIONS -DPEANO_DEBUG=1 -DNDEBUG CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_DEBUG -g -O0 CACHE INTERNAL "")
set(PEANO_DEBUG_DEFINITIONS -DPEANO_DEBUG=4 CACHE INTERNAL "")

set(PEANO_EXTRA_CXX_FLAGS   "" CACHE STRING "Extra C++ flags (semicolon-separated)")
set(PEANO_EXTRA_DEFINITIONS "" CACHE STRING "Extra preprocessor definitions (semicolon-separated)")
set(PEANO_EXTRA_LINK_FLAGS  "" CACHE STRING "Extra linker flags (semicolon-separated)")

target_compile_options(PeanoOptions INTERFACE
  $<$<COMPILE_LANGUAGE:CXX>:${PEANO_EXTRA_CXX_FLAGS}>
)
target_compile_definitions(PeanoOptions INTERFACE ${PEANO_EXTRA_DEFINITIONS})
target_link_options(PeanoOptions INTERFACE ${PEANO_EXTRA_LINK_FLAGS})

if(WITH_MULTITHREADING)
  find_package(OpenMP REQUIRED)
  check_include_files(omp.h HAVE_OMP_H)
  target_link_libraries(PeanoOptions INTERFACE OpenMP::OpenMP_CXX)
  target_compile_definitions(PeanoOptions INTERFACE WITH_OPENMP WITH_MULTITHREADING)
endif()

if(WITH_MPI)
  find_package(MPI REQUIRED)
  target_link_libraries(PeanoOptions INTERFACE MPI::MPI_CXX)
  target_compile_definitions(PeanoOptions INTERFACE WITH_MPI)
endif()

if(WITH_LIBXSMM)
  find_package(LIBXSMM REQUIRED)
  target_link_libraries(PeanoOptions INTERFACE LIBXSMM::LIBXSMM)
  target_compile_definitions(PeanoOptions INTERFACE WITH_LIBXSMM)
endif()

if(WITH_HDF5)
  if(WITH_MPI)
    set(HDF5_PREFER_PARALLEL True)
  endif()
  find_package(HDF5 COMPONENTS C) # CXX
  if(HDF5_FOUND)
    target_link_libraries(PeanoOptions INTERFACE HDF5::HDF5)
    target_compile_definitions(PeanoOptions INTERFACE WITH_HDF5)
    if(WITH_MPI AND NOT HDF5_IS_PARALLEL)
      message(FATAL_ERROR "MPI is enabled but found HDF5 installation does not support parallel I/O.")
    endif()
  else()
    set(WITH_HDF5 OFF CACHE BOOL "Build with HDF5 support" FORCE)
  endif()
endif()

if(WITH_NETCDF)
  find_package(NetCDF)
  if(NetCDF_FOUND)
    if(NOT NetCDF_HAS_HDF5)
      message(FATAL_ERROR "Found NetCDF installation was built without HDF5 support (required for NetCDF-4 support).")
    endif()
    if(WITH_MPI AND NOT NetCDF_IS_PARALLEL)
      message(FATAL_ERROR "MPI is enabled, but the found NetCDF installation does not support parallel I/O.")
    endif()
    target_link_libraries(PeanoOptions INTERFACE ${NetCDF_LIBRARIES})
    target_include_directories(PeanoOptions INTERFACE ${NetCDF_INCLUDE_DIRS})
    target_compile_definitions(PeanoOptions INTERFACE WITH_NETCDF)
  else()
    set(WITH_NETCDF OFF CACHE BOOL "Build with NetCDF support" FORCE)
  endif()
endif()

if(WITH_EASI)
  find_package(easi REQUIRED)
  target_link_libraries(PeanoOptions INTERFACE easi::easi)
  target_compile_definitions(PeanoOptions INTERFACE WITH_EASI WITH_ASAGI)
endif()

if(WITH_ZLIB)
  find_package(ZLIB)
  if(ZLIB_FOUND)
    target_link_libraries(PeanoOptions INTERFACE ZLIB::ZLIB)
    target_compile_definitions(PeanoOptions INTERFACE USE_ZLIB)
  else()
    set(WITH_ZLIB OFF CACHE BOOL "Build with ZLIB compression support" FORCE)
  endif()
endif()

if(WITH_USM)
  target_compile_definitions(PeanoOptions INTERFACE WITH_USM)
endif()

set(WITH_GPU "no" CACHE INTERNAL "Selected GPU backend" FORCE)

if(WITH_GPU_ARCH MATCHES "^spir")
  set(WITH_GPU_VENDOR "intel")
elseif(WITH_GPU_ARCH MATCHES "^sm_")
  set(WITH_GPU_VENDOR "nvidia")
elseif(WITH_GPU_ARCH MATCHES "^gfx")
  set(WITH_GPU_VENDOR "amd")
endif()

if(WITH_ACPP)
  unset(CMAKE_C_COMPILER_LAUNCHER)
  unset(CMAKE_CXX_COMPILER_LAUNCHER)
  include(AdaptiveCpp)
  set(WITH_GPU "sycl" CACHE INTERNAL "" FORCE)
elseif(WITH_KOKKOS)
  unset(CMAKE_C_COMPILER_LAUNCHER)
  unset(CMAKE_CXX_COMPILER_LAUNCHER)
  set(IPO_SUPPORTED FALSE)
  message(STATUS "Disabling IPO/LTO for Kokkos compatability")
  include(Kokkos)
  set(WITH_GPU "kokkos" CACHE INTERNAL "" FORCE)
elseif(WITH_OMPT)
  include(OMPT)
  set(WITH_GPU "ompt" CACHE INTERNAL "" FORCE)
elseif(WITH_GPU_VENDOR STREQUAL "intel")
  include(SYCL)
  set(WITH_GPU "sycl" CACHE INTERNAL "" FORCE)
elseif(WITH_GPU_VENDOR STREQUAL "nvidia")
  include(CUDA)
  set(WITH_GPU "cuda" CACHE INTERNAL "" FORCE)
elseif(WITH_GPU_VENDOR STREQUAL "amd")
  include(HIP)
  set(WITH_GPU "hip" CACHE INTERNAL "" FORCE)
endif()

if(ENABLE_TOOLBOX_LOADBALANCING)
  target_compile_definitions(PeanoOptions INTERFACE ENABLE_TOOLBOX_LOADBALANCING)
endif()

if(ENABLE_TOOLBOX_BLOCKSTRUCTURED)
  target_compile_definitions(PeanoOptions INTERFACE ENABLE_TOOLBOX_BLOCKSTRUCTURED)
endif()

if(ENABLE_TOOLBOX_PARTICLES)
  target_compile_definitions(PeanoOptions INTERFACE ENABLE_TOOLBOX_PARTICLES)
endif()

if(ENABLE_TOOLBOX_CURVI)
  target_compile_definitions(PeanoOptions INTERFACE ENABLE_TOOLBOX_CURVI)
endif()

if(ENABLE_EXAHYPE)
  target_compile_definitions(PeanoOptions INTERFACE ENABLE_EXAHYPE)
endif()

target_include_directories(PeanoOptions INTERFACE
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_subdirectory(src)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${GIT_COMMIT_HASH}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  FILES_MATCHING PATTERN "*.cmake"
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/python
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

export(EXPORT ${PROJECT_NAME}Targets
  FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
)

install(EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${CMAKE_INSTALL_PREFIX}
  NAMESPACE ${PROJECT_NAME}::
)

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_SOURCE_DIR}/README.md
  ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md
  ${CMAKE_SOURCE_DIR}/CODE_OF_CONDUCT.md
  ${CMAKE_SOURCE_DIR}/CITATION.cff
  ${CMAKE_BINARY_DIR}/CMakeCache.txt
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)
