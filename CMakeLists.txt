cmake_minimum_required(VERSION 3.20.5)

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
set(META_PROJECT_DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes")
set(META_AUTHOR_ORGANIZATION "Durham University and Technische Universitaet Muenchen")
set(META_AUTHOR_DOMAIN "www.peano-framework.org")
set(META_VERSION_REVISION "${GIT_COMMIT_HASH_SHORT}")
set(META_GIT_BRANCH "${GIT_BRANCH}")
set(META_GIT_HASH "${GIT_COMMIT_HASH}")
set(META_GIT_BUGREPORT "https://gitlab.lrz.de/hpcsoftware/Peano/-/issues")
set(META_GIT_URL "https://gitlab.lrz.de/hpcsoftware/Peano")
message(STATUS "On Git Branch: ${GIT_BRANCH} (${GIT_COMMIT_HASH})")

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

set(LANGUAGES C CXX)
project(
  ${META_PROJECT_NAME}
  DESCRIPTION ${META_PROJECT_DESCRIPTION}
  HOMEPAGE_URL ${META_AUTHOR_DOMAIN}
  LANGUAGES ${LANGUAGES})
file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")
set(META_COMPILER_VERSION "${CMAKE_SYSTEM_NAME} ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "Compiler is: ${META_COMPILER_VERSION}")

include(GNUInstallDirs)
set(DEFAULT_CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

add_subdirectory(cmake)
include(MessageUtils)
include(Clang-Format)
#include(Clang-Tidy)
#include(Cppcheck)
#include(Link-What-You-Use)
#include(Coverage)
include(TestBigEndian)
test_big_endian(BIG_ENDIAN)
include(ProcessorCount)
ProcessorCount(AVAILABLE_PROCESSOR_COUNT)
find_package(Slurm)
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
  message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
endif()

add_library(project_warnings INTERFACE)
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_${CMAKE_CXX_STANDARD})
include(CompilerDefinitions)
set_project_compiler_definitions(project_options)
include(CompilerFlags)
set_project_compiler_flags(project_warnings)
target_compile_options(project_warnings INTERFACE "-w")
include(LinkerFlags)
set_project_linker_flags(project_options)

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

list(APPEND TOOLCHAINS
  " "
  "intel"
  "nvidia"
  "scorep"
  "chrome"
  "gnu"
)
set(WITH_TOOLCHAIN "" CACHE STRING "${TOOLCHAINS}")
set_property(CACHE WITH_TOOLCHAIN PROPERTY STRINGS ${TOOLCHAINS})

if(WITH_TOOLCHAIN STREQUAL "scorep")
  find_library(PAPI libpapi.a papi)
  find_library(CUBE4W libcube4w.a cube4w)
  target_link_libraries(project_options INTERFACE ${PAPI})
  target_link_libraries(project_options INTERFACE ${CUBE4W})
  target_link_directories(project_options INTERFACE ${LD_LIBRARY_PATH})
  target_link_libraries(project_options INTERFACE ${SCOREP_LIBRARIES})
  target_compile_options(project_options INTERFACE ${SCOREP_COMPILE_FLAGS})
  target_link_options(project_options INTERFACE ${SCOREP_LINK_OPTION})
  add_compile_definitions(UseScoreP)
  add_compile_definitions(UseLogService=ScorePLogger)
elseif(WITH_TOOLCHAIN STREQUAL "itac")
  target_link_libraries(project_options INTERFACE VT::VT)
  target_include_directories(project_options SYSTEM INTERFACE ${VT_INCLUDE_DIRS})
  target_link_directories(project_options INTERFACE ${VT_LIBRARIES_DIR})
  add_compile_definitions(UseITAC)
  add_compile_definitions(UseLogService=ITACLogger)
elseif(WITH_TOOLCHAIN STREQUAL "nvidia")
  check_include_files(nvToolsExt.h HAVE_NVTOOLSEXT_H)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(project_options INTERFACE CUDA::nvToolsExt)
  add_compile_definitions(UseNVTX)
  add_compile_definitions(UseLogService=NVTXLogger)
elseif(NOT WITH_TOOLCHAIN STREQUAL "")
  message(FATAL_ERROR "CMake build system currently does not support the logger ${WITH_TOOLCHAIN}")
endif()

list(APPEND MULTICORES_BACKENDS
  " "
  "cpp"
  "omp"
  "tbb"
  "sycl"
)
set(WITH_MULTITHREADING "" CACHE STRING "${MULTICORES_BACKENDS}")
set_property(CACHE WITH_MULTITHREADING PROPERTY STRINGS ${MULTICORES_BACKENDS})

if(WITH_MULTITHREADING)
  string(TOUPPER ${WITH_MULTITHREADING} WITH_MULTITHREADING_UPPER)
  target_compile_definitions(project_options INTERFACE Shared${WITH_MULTITHREADING_UPPER})
  target_compile_definitions(project_options INTERFACE SharedMemoryParallelisation)
  endif()
if(WITH_MULTITHREADING STREQUAL "omp")
  find_package(OpenMP REQUIRED)
  check_include_files(omp.h HAVE_OMP_H)

  # Workaround for Intel oneAPI. TODO: Fix this:
  # "PLEASE append the compiler options "-save-temps -v", rebuild the application to to get the full command which is failing"
  if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    target_compile_options(project_options INTERFACE -fopenmp)
    target_link_libraries(project_options INTERFACE -fopenmp)
  else()
    target_link_libraries(project_options INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
  endif()
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
  target_link_libraries(project_options INTERFACE TBB::tbb)
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(project_options INTERFACE Threads::Threads)
elseif(WITH_MULTITHREADING STREQUAL "sycl")
  check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)
endif()

list(APPEND ACCELERATOR_BACKENDS
  " "
  "cpp"
  "omp"
  "sycl"
  "hip"
  "cuda"
)
set(WITH_GPU "" CACHE STRING "${ACCELERATOR_BACKENDS}")
set_property(CACHE WITH_GPU PROPERTY STRINGS ${ACCELERATOR_BACKENDS})

list(APPEND ACCELERATOR_ARCHITECTURES
  " "
  "sm_70"
  "sm_72"
  "sm_75"
  "sm_80"
  "sm_86"
  "sm_90"
  "gfx906"
  "gfx908"
  "gfx90a"
  "spir64"
)
set(WITH_GPU_ARCH "" CACHE STRING "GPU architecture sm_XX or gfxXXX")
set_property(CACHE WITH_GPU_ARCH PROPERTY STRINGS ${ACCELERATOR_ARCHITECTURES})

string(SUBSTRING "${WITH_GPU_ARCH}" 0 3 GPU_PREFIX)
if("${GPU_PREFIX}" STREQUAL "sm_")
  set(NVIDIA_GPU TRUE)
elseif("${GPU_PREFIX}" STREQUAL "gfx")
  set(AMD_GPU TRUE)
endif()

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "IntelLLVM" AND ("${WITH_GPU}" STREQUAL "sycl" OR "${WITH_MULTITHREADING}" STREQUAL "sycl"))
  find_package(IntelSYCL REQUIRED)
  #target_link_libraries(project_options INTERFACE IntelSYCL)
endif()

if(WITH_CUDA)
  target_compile_options(project_options INTERFACE -cuda)
  target_link_options(project_options INTERFACE -cuda)
endif()

if("${WITH_GPU}" STREQUAL "cuda")
  set(ENABLE_CUDA_COMPILATION_OPTIONS ON)
  cmake_dependent_option(CUDA_SEPARABLE_COMPILATION "Cuda Separable Compilation" ON "ENABLE_CUDA_COMPILATION_OPTIONS" OFF)
  cmake_dependent_option(CUDA_RESOLVE_DEVICE_SYMBOLS "Cuda Resolve Device Symbols" ON "ENABLE_CUDA_COMPILATION_OPTIONS" OFF)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(project_options INTERFACE CUDA::toolkit)
else()
  set(ENABLE_CUDA_COMPILATION_OPTIONS OFF)
endif()

if(WITH_GPU)
  string(TOUPPER ${WITH_GPU} WITH_GPU_UPPER)
  target_compile_definitions(project_options INTERFACE GPUOffloading${WITH_GPU_UPPER})
  if(WITH_GPU STREQUAL "omp")
    find_package(OpenMPWithOffloading REQUIRED)
    target_link_libraries(project_options INTERFACE OpenMP::OpenMP_CXX_with_offloading)
  elseif(WITH_GPU STREQUAL "cpp")
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
      string(SUBSTRING "${WITH_GPU_ARCH}" 3 5 GPU_ARCH_SUFFIX)
      target_link_libraries(project_options INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
      target_compile_options(project_options INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
    endif()
  elseif(WITH_GPU STREQUAL "sycl")
    set(WITH_MULTITHREADING "sycl" CACHE STRING "GPU sycl needs tbb multithreading")
  elseif(WITH_GPU STREQUAL "hip")
  elseif(WITH_GPU STREQUAL "cuda")
    set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
    set(CMAKE_CUDA_STANDARD 17)
    enable_language(CUDA)
    option(CUDA_SEPARABLE_COMPILATION "CUDA Separable Compilation" ON)
    option(CUDA_RESOLVE_DEVICE_SYMBOLS "CUDA Resolve Device Symbols" ON)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(project_options INTERFACE CUDA::toolkit)
    string(SUBSTRING "${WITH_GPU_ARCH}" 3 5 GPU_ARCH_SUFFIX)
    set(CMAKE_CUDA_ARCHITECTURES ${GPU_ARCH_SUFFIX} CACHE INTERNAL "CUDA Architecture" FORCE)
  endif()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release" "Asserts" "Trace" "Stats" CACHE STRING "Enabled build types" FORCE)
if(NOT CMAKE_BUILD_TYPE OR NOT ${CMAKE_BUILD_TYPE} IN_LIST CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "" CACHE STRING "${CMAKE_CONFIGURATION_TYPES}" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES})
  list(APPEND BUILD_MODES ${CMAKE_CONFIGURATION_TYPES})
else()
  list(APPEND BUILD_MODES ${CMAKE_BUILD_TYPE})
endif()

# Update core libraries' build modes and dimensions if it is not set to build all and if the build type of an application is different.
if(CMAKE_BUILD_TYPE AND PEANO_APP_BUILD_TYPE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "${PEANO_APP_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE ${PEANO_APP_BUILD_TYPE} CACHE STRING "${CMAKE_CONFIGURATION_TYPES}" FORCE)
  list(APPEND BUILD_MODES ${PEANO_APP_BUILD_TYPE})
endif()

# CMAKE_CXX_FLAGS_* are appeneded to the targets (build modes) automatically in most of the cases,
# CMAKE_CXX_FLAGS_* are eppeneded to every target, regardles of build mode. e.g., Release, Debug
# But an uncommon name such as _TRACE is not always appeneded (CMAKE_CXX_FLAGS_TRACE)
# Therefore we will use PEANO_CXX_FLAGS_* variables instead and set CMAKE_CXX_FLAG_* to ""
# Furthermore, CMAKE_CXX_FLAGS is the variable when user defined flags to be used in the environment 
# variable CXXFLAGS="..." and added to every target, which we should not touch here,
# This allows for the CMake build system to use exactly the same flags as the Automake build system 
# for Peano regardless of the default configuration, unless user provides flags.
# Release and Debug are also used by Peano, MinSizeRel and RelWithDebInfo are not used by Peano
# It needs to be done similarly for definitions and LDFLAGS
foreach(${CMAKE_BUILD_TYPE} IN_LIST CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE} "")
  set(${CMAKE_BUILD_TYPE}_DEFINITIONS "")
  set(${CMAKE_BUILD_TYPE}_LDFLAGS "")
endforeach()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  set(PEANO_CXX_FLAGS_RELEASE -O4)
  set(CMAEK_CUDA_FLAGS_RELEASE -O4)
else()
  set(PEANO_CXX_FLAGS_RELEASE -O3)
  set(PEANO_CUDA_FLAGS_RELEASE -O3)
endif()
set(PEANO_RELEASE_DEFINITIONS -DPeanoDebug=0 -DNDEBUG)
set(PEANO_RELEASE_LDFLAGS -g)

#-O2 -fno-omit-frame-pointer
set(PEANO_CXX_FLAGS_DEBUG -g -O0)
set(PEANO_CUDA_FLAGS_DEBUG -g -O0)
set(PEANO_DEBUG_DEFINITIONS -DPeanoDebug=4)
set(PEANO_DEBUG_LDFLAGS -g)

set(PEANO_CXX_FLAGS_STATS ${PEANO_CXX_FLAGS_RELEASE})
set(PEANO_CUDA_FLAGS_STATS ${PEANO_CUDA_FLAGS_RELEASE})
set(PEANO_STATS_DEFINITIONS -DPeanoDebug=0 -DTrackStatistics)
set(PEANO_STATS_LDFLAGS -g)

set(PEANO_CXX_FLAGS_ASSERTS ${PEANO_CXX_FLAGS_DEBUG})
set(PEANO_CUDA_FLAGS_ASSERTS ${PEANO_CUDA_FLAGS_DEBUG})
set(PEANO_ASSERTS_DEFINITIONS -DPeanoDebug=2)
set(PEANO_ASSERTS_LDFLAGS -g)

set(PEANO_CXX_FLAGS_TRACE -g -O2)
set(PEANO_CUDA_FLAGS_TRACE ${PEANO_CXX_FLAGS_TRACE})
set(PEANO_TRACE_DEFINITIONS -DPeanoDebug=1 -DNDEBUG)
set(PEANO_TRACE_LDFLAGS -g)

set(WITH_DIMENSIONS "2;3" CACHE STRING "Build with number of (space) dimensions")

if(NOT "${WITH_DIMENSIONS}" STREQUAL "2;3" AND PEANO_APP_DIMENSION AND NOT "${WITH_DIMENSIONS}" STREQUAL "${PEANO_APP_DIMENSION}")
  set(WITH_DIMENSIONS ${PEANO_APP_DIMENSION} CACHE STRING "Dimensions 2;3" FORCE)
endif()

option(WITH_MPI "Build with Message Passing Interface (MPI) support" OFF)
option(WITH_OTTER "Build with Otter support" OFF)
option(WITH_PETSC "Build with PETSc support" OFF)
option(WITH_LIBXSMM "Build with LIBXSMM support" OFF)
option(WITH_VTK "Build with VTK support" OFF)
option(WITH_HDF5 "Build with HDF5 support" OFF)
option(WITH_NETCDF "Build with NetCDF support" OFF)
option(ENABLE_EXAHYPE "Enable ExaHyPE2" OFF)
option(ENABLE_SWIFT "Enable Swift2" OFF)
option(ENABLE_FINITEELEMENTS "Enable the finite elements toolbox" OFF)
option(ENABLE_MULTIPRECISION "Enable the multi-precision toolbox" OFF)
option(ENABLE_LOADBALANCING "Enable the loadbalancing toolbox" OFF)
option(ENABLE_PARTICLES "Enable the particles toolbox" OFF)
option(ENABLE_BLOCKSTRUCTURED "Enable the blockstructured toolbox" OFF)

if(WITH_MPI)
  find_package(MPI REQUIRED)
  message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
  include_directories(SYSTEM ${MPI_INCLUDE_PATH})
  target_compile_definitions(project_options INTERFACE OMPI_SKIP_MPICXX) # To prevent compiler warnings
  target_compile_definitions(project_options INTERFACE Parallel)
  target_link_libraries(project_options INTERFACE MPI::MPI_CXX)
endif()

if(WITH_OTTER)
  find_package(Otter CONFIG)
  target_compile_definitions(project_options INTERFACE UseOtter)
  target_link_libraries(project_options INTERFACE Otter::Otter)
endif()

if(WITH_PETSC)
  find_package(PETSc REQUIRED)
  target_compile_definitions(project_options INTERFACE UsePETSc)
  target_link_libraries(project_options INTERFACE ${PETSc_LIBRARIES})
  target_include_directories(project_options INTERFACE ${PETSc_INCLUDE_DIRS})
endif()

if(WITH_LIBXSMM)
  find_package(LIBXSMM REQUIRED)
  target_compile_definitions(project_options INTERFACE UseLibXSMM)
  target_link_libraries(project_options INTERFACE ${LIBXSMM_LIBRARIES}) #target_link_system_libraries
  target_include_directories(project_options INTERFACE ${LIBXSMM_INCLUDE_DIRS}) #target_include_system_directories
endif()

if(WITH_VTK)
  find_package(VTK REQUIRED)
  target_compile_definitions(project_options INTERFACE UseVTK)
  target_link_libraries(project_options INTERFACE VTK::CommonCore)
endif()

if(WITH_HDF5)
  find_package(HDF5 REQUIRED)
  target_compile_definitions(project_options INTERFACE UseHDF5)
  target_link_libraries(project_options INTERFACE HDF5::HDF5)
endif()

if(WITH_NETCDF)
  find_package(NetCDF REQUIRED)
  target_compile_definitions(project_options INTERFACE UseNetCDF)
  target_link_libraries(project_options INTERFACE ${NETCDF_LIBRARIES} ${NETCDF_LIBRARIES_CXX})
  target_include_directories(project_options INTERFACE ${NETCDF_INCLUDES})
endif()

if(ENABLE_EXAHYPE)
  #target_compile_definitions(project_options INTERFACE UseExaHyPE)
  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseExaHyPE)
endif()

if(ENABLE_SWIFT)
  #target_compile_definitions(project_options INTERFACE UseSWIFT)
  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseSWIFT)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${META_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${META_VERSION_REVISION}
  COMPATIBILITY SameMajorVersion
)

list(APPEND DRIECTORIES_TO_LINK_OVER "python" "tutorials" "benchmarks" "applications")
foreach(item IN LISTS DRIECTORIES_TO_LINK_OVER)
  if (NOT EXISTS "${CMAKE_BINARY_DIR}/${item}")
    execute_process(COMMAND cp -rsfn "${CMAKE_SOURCE_DIR}/${item}" "${CMAKE_BINARY_DIR}/${item}")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/${item}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
  endif()
endforeach()

file(TOUCH ${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)
include(${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)

if (NOT EXISTS "${CMAKE_BINARY_DIR}/documentation")
  execute_process(COMMAND cp -rsfn "${CMAKE_SOURCE_DIR}/documentation" "${CMAKE_BINARY_DIR}/documentation")
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/documentation/Doxyfile)

    add_custom_target(documentation
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif(DOXYGEN_FOUND)

add_subdirectory(src)

#file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "*/UnitTests/*")
#foreach(SOURCE_FILE ${UNIT_TEST_SOURCES})
#    get_filename_component(TEST_NAME ${SOURCE_FILE} NAME_WE)
#    add_executable(${TEST_NAME} ${SOURCE_FILE})
#    target_link_libraries(${TEST_NAME} PRIVATE TArch)
#    set_target_properties(${TEST_NAME} PROPERTIES
#        DEBUG_POSTFIX "d"
#        SOVERSION ${TARCH_VERSION_MAJOR}
#        VERSION ${TARCH_VERSION}
#        CXX_STANDARD ${CMAKE_CXX_STANDARD}
#    )
#    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
#endforeach()

#if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    add_coverage(
#        NAME Coverage
#        EXECUTABLE ctest -V
#        EXCLUDE "/usr/*"
#        GENHTML_ARGS -s --legend
#    )
#    add_coverage_cobertura(
#        NAME CoverageCobertura
#        EXECUTABLE ctest -V
#    )
#    add_coverage_gcovr(
#        NAME CoverageGcovr
#        EXECUTABLE ctest -V
#    )
#elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#    add_coverage(
#        TARGET TestRunnerTest
#        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}"
#        CONTINUE_AFTER_CPP_EXCEPTION
#        REPORT_FOR_PROJECT
#        REPORT_FOR_TARGET
#    )
#endif()

display_separator("Project Configuration Summary")
display_variable(AVAILABLE_PROCESSOR_COUNT)
display_variable(WITH_TOOLCHAIN)
display_variable(WITH_MULTITHREADING)
display_variable(WITH_GPU)
display_variable(WITH_GPU_ARCH)
display_variable(WITH_DIMENSIONS)
display_variable(WITH_MPI)
display_variable(WITH_OTTER)
if(DEFINED Otter_DIR)
  display_variable(Otter_DIR)
endif()
display_variable(WITH_PETSC)
display_variable(WITH_LIBXSMM)
display_variable(WITH_VTK)
display_variable(WITH_HDF5)
display_variable(WITH_NETCDF)
display_variable(ENABLE_EXAHYPE)
display_variable(ENABLE_FINITEELEMENTS)
display_variable(ENABLE_MULTIPRECISION)
display_variable(ENABLE_LOADBALANCING)
display_variable(ENABLE_PARTICLES)
display_variable(ENABLE_BLOCKSTRUCTURED)

display_subseparator("Options to Build")
display_variable(CMAKE_C_COMPILER)
display_variable(CMAKE_C_STANDARD)
display_variable(CMAKE_CXX_COMPILER)
display_variable(CMAKE_CXX_STANDARD)
display_variable(CMAKE_BUILD_TYPE)
display_variable(CMAKE_CONFIGURATION_TYPES)
display_variable(CMAKE_CXX_FLAGS)
display_variable(PEANO_CXX_FLAGS_RELEASE)
display_variable(PEANO_RELEASE_DEFINITIONS)
display_variable(PEANO_RELEASE_LDFLAGS)
display_variable(PEANO_CXX_FLAGS_DEBUG)
display_variable(PEANO_DEBUG_DEFINITIONS)
display_variable(PEANO_DEBUG_LDFLAGS)
display_variable(PEANO_CXX_FLAGS_STATS)
display_variable(PEANO_STATS_DEFINITIONS)
display_variable(PEANO_STATS_LDFLAGS)
display_variable(PEANO_CXX_FLAGS_ASSERTS)
display_variable(PEANO_ASSERTS_DEFINITIONS)
display_variable(PEANO_ASSERTS_LDFLAGS)
display_variable(PEANO_CXX_FLAGS_TRACE)
display_variable(PEANO_TRACE_DEFINITIONS)
display_variable(PEANO_TRACE_LDFLAGS)
display_variable(CMAKE_CXX_FLAGS_MINSIZEREL)
display_variable(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
display_variable(CMAKE_VERBOSE_MAKEFILE)
display_variable(BUILD_SHARED_LIBS)
display_variable(CMAKE_CXX_FLAGS_INIT)

if(WITH_GPU STREQUAL "cuda")
  display_variable(CMAKE_CUDA_FLAGS)
  display_variable(PEANO_CUDA_FLAGS_RELEASE)
  display_variable(PEANO_CUDA_FLAGS_DEBUG)
  display_variable(PEANO_CUDA_FLAGS_STATS)
  display_variable(PEANO_CUDA_FLAGS_ASSERTS)
  display_variable(PEANO_CUDA_FLAGS_TRACE)
  display_variable(PEANO_CUDA_FLAGS_MINSIZEREL)
  display_variable(PEANO_CUDA_FLAGS_RELWITHDEBINFO)
  display_variable(CMAKE_CUDA_HOST_COMPILER)
  display_variable(CMAKE_CUDA_ARCHITECTURES)
  display_variable(CUDA_NVCC_FLAGS)
  display_variable(CUDA_NVCC_FLAGS_RELEASE)
  display_variable(CUDA_SEPARABLE_COMPILATION)
  display_variable(CUDA_RESOLVE_DEVICE_SYMBOLS)
endif()

display_subseparator("Options to Install")
display_variable(CMAKE_INSTALL_FULL_BINDIR)
display_variable(CMAKE_INSTALL_FULL_LIBDIR)

display_subseparator("End of Summary")
