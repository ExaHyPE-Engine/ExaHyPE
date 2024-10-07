# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
cmake_minimum_required(VERSION 3.21)

set(META_PROJECT_NAME "Peano")
set(META_PROJECT_DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes")
set(META_AUTHOR_ORGANIZATION "Durham University and Technische Universitaet Muenchen")
set(META_AUTHOR_DOMAIN "www.peano-framework.org")

project(${META_PROJECT_NAME}
  DESCRIPTION ${META_PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${META_AUTHOR_DOMAIN}
  LANGUAGES C CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GetGitRevisionDescription)
get_git_head_revision(GIT_BRANCH GIT_COMMIT_HASH)
get_git_commit_message(GIT_COMMIT_MESSAGE)
get_git_commit_timestamp(GIT_COMMIT_TIMESTAMP)

set(META_VERSION_REVISION "${GIT_COMMIT_HASH}")
set(META_GIT_BRANCH "${GIT_BRANCH}")
set(META_GIT_HASH "${GIT_COMMIT_HASH}")
set(META_GIT_COMMIT_MESSAGE "${GIT_COMMIT_MESSAGE}")
set(META_GIT_URL "https://gitlab.lrz.de/hpcsoftware/Peano")
message(STATUS "Revision: ${GIT_BRANCH} -m \"${GIT_COMMIT_MESSAGE}\" (${GIT_COMMIT_HASH}) ${GIT_COMMIT_TIMESTAMP}")

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

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
execute_process(COMMAND ${Python3_EXECUTABLE} -c "import jinja2" RESULT_VARIABLE JINJA2_FOUND)
# We need Jinja2 for code generation
if(NOT JINJA2_FOUND EQUAL 0)
  message(WARNING "Jinja2 package (needed for code generation) not found. Please install Jinja2 e.g.: 'pip3 install jinja2'")
else()
  message(STATUS "Found Jinja2")
endif()

include(Ccache)
include(TestBigEndian)
test_big_endian(BIG_ENDIAN)
include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)
find_package(Threads REQUIRED)
include(CTest)
enable_testing()
include(CMakeDependentOption)
# https://stackoverflow.com/questions/31355692/how-do-i-enable-link-time-optimization-lto-with-cmake
include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
if(IPO_SUPPORTED)
  message(STATUS "IPO/LTO enabled")
  set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION 1)
else()
  #message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
  message(WARNING "IPO/LTO not supported")
endif()

add_library(ProjectOptions INTERFACE)
target_compile_features(ProjectOptions INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

if(${CMAKE_CXX_COMPILER_ID} MATCHES ".*Clang|GNU|Intel|IntelLLVM")
  target_compile_options(ProjectOptions INTERFACE
    -w
    -W
    -Wall
    -Wextra
    -pedantic
    -Wpedantic
    -Wshadow
    -Wunused
    -Wno-attributes
    -Wno-unknown-attributes
    -Wnon-virtual-dtor
    -Wreturn-type
    -Wno-system-headers
    #-flto # https://stackoverflow.com/questions/31688069/requirements-to-use-flto
    #-ffat-lto-objects # https://stackoverflow.com/questions/13799452/what-is-the-difference-in-gcc-between-lto-and-fat-lto-objects
    #-fno-elide-constructors # Disables RVO. For testing purposes only, should be disabled in production code. https://shaharmike.com/cpp/rvo/
    $<$<CONFIG:Debug>:-O0;-ggdb;-fno-elide-constructors>
  )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  target_compile_options(ProjectOptions INTERFACE
    -W
    -Wall
    --diag_suppress=unrecognized_attribute
  )
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
  target_compile_options(ProjectOptions INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:/MP> # Use as many cores as available for compilation. https://blog.kitware.com/cmake-building-with-all-your-cores/
    $<$<COMPILE_LANGUAGE:CXX>:/W4> #/Wall # Baseline reasonable warnings.
    $<$<COMPILE_LANGUAGE:CXX>:/permissive-> # Standards conformance mode for MSVC compiler. https://docs.microsoft.com/de-de/cpp/build/reference/permissive-standards-conformance?view=vs-2019
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Oi>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/O2>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/Ot>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GL>>
    $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Release>:/GF>>
  )
  target_link_options(ProjectOptions INTERFACE
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/LTCG>
  )
endif()

include(CheckIncludeFiles)
check_include_files(dlfcn.h HAVE_DLFCN_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(stdio.h HAVE_STDIO_H)
check_include_files(stdlib.h HAVE_STDLIB_H)
check_include_files(strings.h HAVE_STRINGS_H)
check_include_files(string.h HAVE_STRING_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(unistd.h HAVE_UNISTD_H)

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
  target_link_libraries(ProjectOptions INTERFACE ${PAPI} ${CUBE4W} ${SCOREP_LIBRARIES})
  target_link_options(ProjectOptions INTERFACE ${SCOREP_LINK_OPTION})
  target_compile_options(ProjectOptions INTERFACE ${SCOREP_COMPILE_FLAGS})
  target_compile_definitions(ProjectOptions INTERFACE UseScoreP UseLogService=ScorePLogger)
elseif(WITH_TOOLCHAIN STREQUAL "itac")
  target_link_libraries(ProjectOptions INTERFACE VT::VT)
  target_include_directories(ProjectOptions SYSTEM INTERFACE ${VT_INCLUDE_DIRS})
  target_link_directories(ProjectOptions INTERFACE ${VT_LIBRARIES_DIR})
  target_compile_definitions(ProjectOptions INTERFACE UseITAC UseLogService=ITACLogger)
elseif(WITH_TOOLCHAIN STREQUAL "nvidia")
  check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(ProjectOptions INTERFACE CUDA::nvToolsExt)
  target_compile_definitions(ProjectOptions INTERFACE UseNVTX UseLogService=NVTXLogger)
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
  target_link_libraries(ProjectOptions INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
  target_compile_definitions(ProjectOptions INTERFACE SharedOMP)
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
  target_link_libraries(ProjectOptions INTERFACE TBB::tbb)
  target_compile_definitions(ProjectOptions INTERFACE SharedTBB)
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(ProjectOptions INTERFACE Threads::Threads)
  target_compile_definitions(ProjectOptions INTERFACE SharedCPP)
elseif(WITH_MULTITHREADING STREQUAL "sycl")
  check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)
  target_compile_options(ProjectOptions INTERFACE -fsycl)
  target_link_options(ProjectOptions INTERFACE -fsycl)
  target_compile_definitions(ProjectOptions INTERFACE SharedSYCL)
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

if(WITH_GPU STREQUAL "omp")
  find_package(OpenMPTarget REQUIRED)
  target_link_libraries(ProjectOptions INTERFACE OpenMP::OpenMPTarget)
  target_compile_definitions(ProjectOptions INTERFACE GPUOffloadingOMP)
elseif(WITH_GPU STREQUAL "cpp")
  if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    string(SUBSTRING "${WITH_GPU_ARCH}" 3 5 GPU_ARCH_SUFFIX)
    target_link_options(ProjectOptions INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
    target_compile_options(ProjectOptions INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
    target_compile_definitions(ProjectOptions INTERFACE GPUOffloadingCPP)
  endif()
elseif(WITH_GPU STREQUAL "sycl")
  target_compile_definitions(ProjectOptions INTERFACE GPUOffloadingSYCL)
  if(PEANO_NVIDIA_GPU)
    target_compile_options(ProjectOptions INTERFACE -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
    target_link_options(ProjectOptions INTERFACE -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
  elseif(PEANO_AMD_GPU)
    target_compile_options(ProjectOptions INTERFACE -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
    target_link_options(ProjectOptions INTERFACE -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
  elseif(PEANO_INTEL_GPU)
    find_package(IntelSYCL REQUIRED)
    target_link_libraries(ProjectOptions INTERFACE IntelSYCL)
  endif()
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

if(WITH_MPI)
  find_package(MPI REQUIRED)
  message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
  target_compile_definitions(ProjectOptions INTERFACE OMPI_SKIP_MPICXX) # To prevent compiler warnings
  target_compile_definitions(ProjectOptions INTERFACE Parallel)
  # target_link_libraries(ProjectOptions INTERFACE MPI::MPI_CXX)
  target_compile_options(ProjectOptions INTERFACE ${MPI_CXX_FLAGS})
  target_link_libraries(ProjectOptions INTERFACE ${MPI_CXX_LIBRARIES})
  target_compile_definitions(ProjectOptions INTERFACE ${MPI_CXX_COMPILE_DEFINITIONS})
  target_include_directories(ProjectOptions INTERFACE ${MPI_INCLUDE_PATH} ${MPI_CXX_INCLUDE_DIRS})
  target_link_options(ProjectOptions INTERFACE ${MPI_CXX_LINK_FLAGS})
endif()

if(WITH_OTTER)
  find_package(Otter CONFIG)
  target_compile_definitions(ProjectOptions INTERFACE UseOtter)
  target_link_libraries(ProjectOptions INTERFACE Otter::Otter)
endif()

if(WITH_PETSC)
  find_package(PETSc REQUIRED)
  target_compile_definitions(ProjectOptions INTERFACE UsePETSc)
  target_link_libraries(ProjectOptions INTERFACE ${PETSc_LIBRARIES})
  target_include_directories(ProjectOptions INTERFACE ${PETSc_INCLUDE_DIRS})
endif()

if(WITH_TARGETDART)
  target_compile_definitions(ProjectOptions INTERFACE UseTargetDART)
endif()

if(WITH_LIBXSMM)
  find_package(LIBXSMM REQUIRED)
  target_compile_definitions(ProjectOptions INTERFACE UseLibXSMM)
  target_link_libraries(ProjectOptions INTERFACE LIBXSMM::LIBXSMM)
endif()

if(WITH_VTK)
  find_package(VTK REQUIRED)
  target_compile_definitions(ProjectOptions INTERFACE UseVTK)
  target_link_libraries(ProjectOptions INTERFACE VTK::CommonCore)
endif()

if(WITH_HDF5)
  find_package(HDF5 REQUIRED)
  target_compile_definitions(ProjectOptions INTERFACE UseHDF5)
  target_link_libraries(ProjectOptions INTERFACE HDF5::HDF5)
endif()

if(WITH_NETCDF)
  find_package(NetCDF REQUIRED)
  target_compile_definitions(ProjectOptions INTERFACE UseNetCDF)
  target_link_libraries(ProjectOptions INTERFACE ${NETCDF_LIBRARIES} ${NETCDF_LIBRARIES_CXX})
  target_include_directories(ProjectOptions INTERFACE ${NETCDF_INCLUDES} CACHE INTERNAL "")
endif()

if(ENABLE_LOADBALANCING)
  target_compile_definitions(ProjectOptions INTERFACE UseToolboxLoadbalancing)
endif()

if(ENABLE_BLOCKSTRUCTURED)
  target_compile_definitions(ProjectOptions INTERFACE UseToolboxBlockstructured)
endif()

if(ENABLE_PARTICLES)
  target_compile_definitions(ProjectOptions INTERFACE UseToolboxParticles)
endif()

if(ENABLE_MULTIPRECISION)
  target_compile_definitions(ProjectOptions INTERFACE UseToolboxMultiprecision)
endif()

if(ENABLE_FINITEELEMENTS)
  target_compile_definitions(ProjectOptions INTERFACE UseToolboxFiniteElements)
endif()

if(ENABLE_EXAHYPE)
  target_compile_definitions(ProjectOptions INTERFACE UseExaHyPE)
endif()

if(ENABLE_SWIFT)
  target_compile_definitions(ProjectOptions INTERFACE UseSWIFT)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${META_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${META_VERSION_REVISION}
  COMPATIBILITY SameMajorVersion
)

install(DIRECTORY
  ${CMAKE_SOURCE_DIR}/cmake
  ${CMAKE_SOURCE_DIR}/python
  DESTINATION "${CMAKE_INSTALL_PREFIX}")

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md
  ${CMAKE_SOURCE_DIR}/CODE_OF_CONDUCT.md
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

target_include_directories(ProjectOptions INTERFACE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_BINARY_DIR}/src
)

add_subdirectory(src)
