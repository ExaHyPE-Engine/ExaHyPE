# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
cmake_minimum_required(VERSION 3.21)

set(PEANO_PROJECT_NAME "Peano")
set(PEANO_PROJECT_DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes")
set(PEANO_AUTHOR_ORGANIZATION "Durham University and Technische Universitaet Muenchen")
set(PEANO_AUTHOR_DOMAIN "www.peano-framework.org")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_BRANCH GIT_COMMIT_HASH)
get_git_commit_message(GIT_COMMIT_MESSAGE)
get_git_commit_timestamp(GIT_COMMIT_TIMESTAMP)

set(PEANO_VERSION_REVISION "${GIT_COMMIT_HASH}")
set(PEANO_GIT_BRANCH "${GIT_BRANCH}")
set(PEANO_GIT_HASH "${GIT_COMMIT_HASH}")
set(PEANO_GIT_COMMIT_MESSAGE "${GIT_COMMIT_MESSAGE}")
set(PEANO_GIT_URL "https://gitlab.lrz.de/hpcsoftware/Peano")
message(STATUS "Revision: ${GIT_BRANCH} -m \"${GIT_COMMIT_MESSAGE}\" (${GIT_COMMIT_HASH}) ${GIT_COMMIT_TIMESTAMP}")

project(${PEANO_PROJECT_NAME}
  DESCRIPTION ${PEANO_PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${PEANO_AUTHOR_DOMAIN}
  LANGUAGES C CXX
)

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

include(Ccache)
include(CheckIncludeFiles)

include(TestBigEndian)
test_big_endian(BIG_ENDIAN)

include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)

include(CTest)
enable_testing()

# https://stackoverflow.com/questions/31355692/how-do-i-enable-link-time-optimization-lto-with-cmake
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION 1)
else()
  #message(WARNING "IPO/LTO not supported: ${IPO_ERROR}")
  message(WARNING "IPO/LTO not supported")
endif()

add_library(PeanoInterface INTERFACE)
target_compile_features(PeanoInterface INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
install(TARGETS PeanoInterface EXPORT PeanoTargets)

if(${CMAKE_CXX_COMPILER_ID} MATCHES ".*Clang|GNU|Intel|IntelLLVM")
  target_compile_options(PeanoInterface INTERFACE
    -w
    -W
    -Wall
    -Wextra
    -pedantic
    -Wpedantic
    -Wshadow
    -Wunused
    -Wno-attributes
    #-Wno-unknown-attributes
    -Wnon-virtual-dtor
    -Wreturn-type
    -Wno-system-headers
    #-flto # https://stackoverflow.com/questions/31688069/requirements-to-use-flto
    #-ffat-lto-objects # https://stackoverflow.com/questions/13799452/what-is-the-difference-in-gcc-between-lto-and-fat-lto-objects
    #-fno-elide-constructors # Disables RVO. For testing purposes only, should be disabled in production code. https://shaharmike.com/cpp/rvo/
    $<$<CONFIG:Debug>:-O0;-ggdb;-fno-elide-constructors>
  )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  target_compile_options(PeanoInterface INTERFACE
    -W
    -Wall
    --diag_suppress=unrecognized_attribute
  )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
  target_compile_options(PeanoInterface INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:/MP> # Use as many cores as available for compilation. https://blog.kitware.com/cmake-building-with-all-your-cores/
    $<$<COMPILE_LANGUAGE:CXX>:/W4> #/Wall # Baseline reasonable warnings.
    $<$<COMPILE_LANGUAGE:CXX>:/permissive-> # Standards conformance mode for MSVC compiler. https://docs.microsoft.com/de-de/cpp/build/reference/permissive-standards-conformance?view=vs-2019
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Oi>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/O2>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Ot>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GL>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GF>>
  )
  target_link_options(PeanoInterface INTERFACE
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/LTCG>
  )
endif()

option(WITH_MPI "Build with Message Passing Interface (MPI) support" OFF)
option(WITH_TARGETDART "Build with targetDART support" OFF)
option(WITH_OTTER "Build with Otter support" OFF)
option(WITH_PETSC "Build with PETSc support" OFF)
option(WITH_LIBXSMM "Build with LIBXSMM support" OFF)
option(WITH_VTK "Build with VTK support" OFF)
option(WITH_HDF5 "Build with HDF5 support" OFF)
option(WITH_NETCDF "Build with NetCDF support" OFF)

option(ENABLE_PARTICLES "Enable the particles toolbox" OFF)
option(ENABLE_FINITEELEMENTS "Enable the finite elements toolbox" OFF)
option(ENABLE_BLOCKSTRUCTURED "Enable the blockstructured toolbox" OFF)
option(ENABLE_LOADBALANCING "Enable the loadbalancing toolbox" OFF)
option(ENABLE_EXAHYPE "Enable ExaHyPE 2" OFF)
option(ENABLE_SWIFT "Enable Swift 2" OFF)

list(APPEND AVAILABLE_TOOLCHAINS
  "no"
  "intel"
  "nvidia"
  "scorep"
  "chrome"
  "gnu"
)
set(WITH_TOOLCHAIN "no" CACHE STRING "${AVAILABLE_TOOLCHAINS}")
set_property(CACHE WITH_TOOLCHAIN PROPERTY STRINGS ${AVAILABLE_TOOLCHAINS})

if(WITH_TOOLCHAIN STREQUAL "scorep")
  find_library(PAPI libpapi.a papi)
  find_library(CUBE4W libcube4w.a cube4w)
  target_link_libraries(PeanoInterface INTERFACE ${PAPI} ${CUBE4W} ${SCOREP_LIBRARIES})
  target_link_options(PeanoInterface INTERFACE ${SCOREP_LINK_OPTION})
  target_compile_options(PeanoInterface INTERFACE ${SCOREP_COMPILE_FLAGS})
  target_compile_definitions(PeanoInterface INTERFACE UseScoreP UseLogService=ScorePLogger)
elseif(WITH_TOOLCHAIN STREQUAL "itac")
  target_link_libraries(PeanoInterface INTERFACE VT::VT)
  target_include_directories(PeanoInterface SYSTEM INTERFACE ${VT_INCLUDE_DIRS})
  target_link_directories(PeanoInterface INTERFACE ${VT_LIBRARIES_DIR})
  target_compile_definitions(PeanoInterface INTERFACE UseITAC UseLogService=ITACLogger)
elseif(WITH_TOOLCHAIN STREQUAL "nvidia")
  check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(PeanoInterface INTERFACE CUDA::nvToolsExt)
  target_compile_definitions(PeanoInterface INTERFACE UseNVTX UseLogService=NVTXLogger)
endif()

list(APPEND AVAILABLE_MULTITHREADING
  "no"
  "omp"
  "sycl"
  "tbb"
  "cpp"
)
set(WITH_MULTITHREADING "no" CACHE STRING "${AVAILABLE_MULTITHREADING}")
set_property(CACHE WITH_MULTITHREADING PROPERTY STRINGS ${AVAILABLE_MULTITHREADING})

if(WITH_MULTITHREADING STREQUAL "omp")
  find_package(OpenMP REQUIRED)
  check_include_files(omp.h HAVE_OMP_H)
  target_link_libraries(PeanoInterface INTERFACE OpenMP::OpenMP_CXX)
  target_link_options(PeanoInterface INTERFACE ${OpenMP_CXX_FLAGS})
  target_compile_definitions(PeanoInterface INTERFACE SharedOMP)
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
  target_link_libraries(PeanoInterface INTERFACE TBB::tbb)
  target_compile_definitions(PeanoInterface INTERFACE SharedTBB)
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(PeanoInterface INTERFACE Threads::Threads)
  target_compile_definitions(PeanoInterface INTERFACE SharedCPP)
elseif(WITH_MULTITHREADING STREQUAL "sycl")
  check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)
  target_compile_options(PeanoInterface INTERFACE -fsycl)
  target_link_options(PeanoInterface INTERFACE -fsycl)
  target_compile_definitions(PeanoInterface INTERFACE SharedSYCL)
endif()

list(APPEND AVAILABLE_ACCELERATORS
  "no"
  "omp"
  "sycl"
  "cpp"
)
set(WITH_GPU "no" CACHE STRING "${AVAILABLE_ACCELERATORS}")
set_property(CACHE WITH_GPU PROPERTY STRINGS ${AVAILABLE_ACCELERATORS})

list(APPEND AVAILABLE_ACCELERATOR_ARCHITECTURES
  "none"
  "sm_70"
  "sm_72"
  "sm_75"
  "sm_80"
  "sm_86"
  "sm_89"
  "sm_90"
  "gfx906"
  "gfx908"
  "gfx90a"
  "gfx942"
  "spir64"
)
set(WITH_GPU_ARCH "none" CACHE STRING "${AVAILABLE_ACCELERATOR_ARCHITECTURES}")
set_property(CACHE WITH_GPU_ARCH PROPERTY STRINGS ${AVAILABLE_ACCELERATOR_ARCHITECTURES})

string(SUBSTRING "${WITH_GPU_ARCH}" 0 3 GPU_PREFIX)
if("${GPU_PREFIX}" STREQUAL "sm_")
  set(PEANO_NVIDIA_GPU TRUE)
elseif("${GPU_PREFIX}" STREQUAL "gfx")
  set(PEANO_AMD_GPU TRUE)
elseif("${GPU_PREFIX}" STREQUAL "spir")
  set(PEANO_INTEL_GPU TRUE)
endif()

if(NOT WITH_GPU STREQUAL "no" AND WITH_GPU_ARCH STREQUAL "none")
  message(FATAL_ERROR "Make sure to also set WITH_GPU_ARCH")
endif()

if(WITH_GPU STREQUAL "omp")
  if(WITH_MULTITHREADING STREQUAL "no")
    message(FATAL_ERROR "Make sure to also enable OpenMP multithreading support as this provides the necessary -fopenmp flag")
  endif()
  include(OpenMPTarget)
elseif(WITH_GPU STREQUAL "cpp")
  include(StdPar)
elseif(WITH_GPU STREQUAL "sycl")
  include(SYCL)
endif()

list(APPEND PEANO_DIMENSIONS
  "2"
  "3"
)

set(CMAKE_BUILD_TYPE "" CACHE INTERNAL "")
list(APPEND PEANO_BUILD_TYPES
  "Release"
  "Debug"
  "Trace"
  "Asserts"
  "Stats"
)

set(PEANO_CXX_FLAGS_RELEASE -O3 CACHE INTERNAL "")
set(PEANO_RELEASE_DEFINITIONS -DPeanoDebug=0 -DNDEBUG CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_DEBUG -g -O0 CACHE INTERNAL "")
set(PEANO_DEBUG_DEFINITIONS -DPeanoDebug=4 CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_STATS ${PEANO_CXX_FLAGS_RELEASE} CACHE INTERNAL "")
set(PEANO_STATS_DEFINITIONS -DPeanoDebug=0 -DTrackStatistics CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_ASSERTS ${PEANO_CXX_FLAGS_DEBUG} CACHE INTERNAL "")
set(PEANO_ASSERTS_DEFINITIONS -DPeanoDebug=2 CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_TRACE -g -O2 CACHE INTERNAL "")
set(PEANO_TRACE_DEFINITIONS -DPeanoDebug=1 -DNDEBUG CACHE INTERNAL "")

if(WITH_MPI)
  find_package(MPI REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE OMPI_SKIP_MPICXX) # To prevent compiler warnings
  target_compile_definitions(PeanoInterface INTERFACE Parallel)
  target_link_libraries(PeanoInterface INTERFACE MPI::MPI_CXX)
endif()

if(WITH_OTTER)
  find_package(Otter CONFIG)
  target_compile_definitions(PeanoInterface INTERFACE UseOtter)
  target_link_libraries(PeanoInterface INTERFACE Otter::Otter)
endif()

if(WITH_PETSC)
  find_package(PETSc REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE UsePETSc)
  target_link_libraries(PeanoInterface INTERFACE ${PETSc_LIBRARIES})
  target_include_directories(PeanoInterface INTERFACE ${PETSc_INCLUDE_DIRS})
endif()

if(WITH_TARGETDART)
  target_compile_definitions(PeanoInterface INTERFACE UseTargetDART)
endif()

if(WITH_LIBXSMM)
  find_package(LIBXSMM REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE UseLibXSMM)
  target_link_libraries(PeanoInterface INTERFACE LIBXSMM::LIBXSMM)
endif()

if(WITH_VTK)
  find_package(VTK REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE UseVTK)
  target_link_libraries(PeanoInterface INTERFACE VTK::CommonCore)
endif()

if(WITH_HDF5)
  find_package(HDF5 REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE UseHDF5)
  target_link_libraries(PeanoInterface INTERFACE HDF5::HDF5)
endif()

if(WITH_NETCDF)
  find_package(NetCDF REQUIRED)
  target_compile_definitions(PeanoInterface INTERFACE UseNetCDF)
  target_link_libraries(PeanoInterface INTERFACE ${NETCDF_LIBRARIES} ${NETCDF_LIBRARIES_CXX})
  target_include_directories(PeanoInterface INTERFACE ${NETCDF_INCLUDES} CACHE INTERNAL "")
endif()

if(ENABLE_LOADBALANCING)
  target_compile_definitions(PeanoInterface INTERFACE UseToolboxLoadbalancing)
endif()

if(ENABLE_BLOCKSTRUCTURED)
  target_compile_definitions(PeanoInterface INTERFACE UseToolboxBlockstructured)
endif()

if(ENABLE_PARTICLES)
  target_compile_definitions(PeanoInterface INTERFACE UseToolboxParticles)
endif()

if(ENABLE_MULTIPRECISION)
  target_compile_definitions(PeanoInterface INTERFACE UseToolboxMultiprecision)
endif()

if(ENABLE_FINITEELEMENTS)
  target_compile_definitions(PeanoInterface INTERFACE UseToolboxFiniteElements)
endif()

if(ENABLE_EXAHYPE)
  target_compile_definitions(PeanoInterface INTERFACE UseExaHyPE)
endif()

if(ENABLE_SWIFT)
  target_compile_definitions(PeanoInterface INTERFACE UseSWIFT)
endif()

target_include_directories(PeanoInterface INTERFACE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src
)

add_subdirectory(src)

export(EXPORT PeanoTargets
  FILE ${CMAKE_BINARY_DIR}/${PEANO_PROJECT_NAME}Targets.cmake
  NAMESPACE ${PEANO_PROJECT_NAME}::
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${PEANO_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${PEANO_VERSION_REVISION}
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
  ${CMAKE_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_BINARY_DIR}/${PEANO_PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/python
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md
  ${CMAKE_SOURCE_DIR}/CODE_OF_CONDUCT.md
  ${CMAKE_BINARY_DIR}/${PEANO_PROJECT_NAME}Config.cmake
  ${CMAKE_BINARY_DIR}/${PEANO_PROJECT_NAME}ConfigVersion.cmake
  ${CMAKE_BINARY_DIR}/${PEANO_PROJECT_NAME}Targets.cmake
  DESTINATION ${CMAKE_INSTALL_PREFIX}
)
