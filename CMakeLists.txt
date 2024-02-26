cmake_minimum_required(VERSION 3.20.5)

set(META_PROJECT_NAME "Peano")
set(META_PROJECT_DESCRIPTION "Peano is an open source framework for solvers on dynamically adaptive Cartesian meshes")
set(META_AUTHOR_ORGANIZATION "Durham University and Technische Universitaet Muenchen")
set(META_AUTHOR_DOMAIN "www.peano-framework.org")

# This has to be done before the project is created and before any languages are added.
# Adding a subdirectory does not help here and needs to be explicitly given with the path(cmake/...),
# Because this file is needed by CMake itself and not by the project.
# We need to override specific compiler flags initialized by CMake, to
# match them exactly with the flags defined in Peano Automake build system.
# More details can be found in the cmake/CustomMakeRules.cmake file.
# For example, the Peano release optimisation flag is -O3 (and -DNDEBUG in definitions);
# but NVHPC defaults are -fast and -O3 in some systems, we override these with this.
set(CMAKE_USER_MAKE_RULES_OVERRIDE "cmake/CustomMakeRules.cmake")

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

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PEANO_DEFINITIONS "" CACHE INTERNAL "")
set(PEANO_FLAGS "" CACHE INTERNAL "")
set(PEANO_DEPENDS "" CACHE INTERNAL "")
set(PEANO_LINK_FLAGS "" CACHE INTERNAL "")
set(USE_COMPONENT_DEFINITIONS "" CACHE INTERNAL "")
set(PEANO_INCLUDES "" CACHE INTERNAL "")
set(PEANO_INCLUDES ${PEANO_INCLUDES} ${CMAKE_INSTALL_PREFIX}/include CACHE INTERNAL "")


set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

file(WRITE "${PROJECT_BINARY_DIR}/.gitignore" "*")
set(META_COMPILER_VERSION "${CMAKE_SYSTEM_NAME} ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER_VERSION})")
message(STATUS "Compiler is: ${META_COMPILER_VERSION}")

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

include(MessageUtils)
include(Clang-Format)
#include(Cppcheck)
#include(Coverage)
include(Sanitisers)
include(IWYU)
include(Clang-Tidy)
include(CCache)
include(Doxygen)
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
  #message(STATUS "IPO/LTO not supported: ${IPO_ERROR}")
  message(WARNING "IPO/LTO not supported")
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
  set(PEANO_INCLUDES ${PEANO_INCLUDESS} ${VT_INCLUDE_DIRS} CACHE INTERNAL "")
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
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} Shared${WITH_MULTITHREADING_UPPER} SharedMemoryParallelisation CACHE INTERNAL "")
endif()
if(WITH_MULTITHREADING STREQUAL "omp")
  find_package(OpenMP REQUIRED)
  check_include_files(omp.h HAVE_OMP_H)
  target_link_libraries(project_options INTERFACE OpenMP::OpenMP_C OpenMP::OpenMP_CXX)
  set(PEANO_DEPENDS ${PEANO_DEPENDS} OpenMP::OpenMP_C OpenMP::OpenMP_CXX CACHE INTERNAL "")
  set(PEANO_FLAGS ${PEANO_FLAGS} ${OpenMP_CXX_FLAGS} CACHE INTERNAL "")
  set(PEANO_LINK_FLAGS ${PEANO_LINK_FLAGS} ${OpenMP_EXE_LINKER_FLAGS} CACHE INTERNAL "")
elseif(WITH_MULTITHREADING STREQUAL "tbb")
  find_package(TBB REQUIRED)
  target_link_libraries(project_options INTERFACE TBB::tbb)
  set(PEANO_DEPENDS ${PEANO_DEPENDS} TBB::tbb CACHE INTERNAL "")
elseif(WITH_MULTITHREADING STREQUAL "cpp")
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
  target_link_libraries(project_options INTERFACE Threads::Threads)
  set(PEANO_DEPENDS ${PEANO_DEPENDS} Threads::Threads CACHE INTERNAL "")
elseif(WITH_MULTITHREADING STREQUAL "sycl")
  check_include_files(CL/sycl.hpp HAVE_CL_SYCL_HPP)
  target_compile_options(project_options INTERFACE -fsycl)
  target_link_options(project_options INTERFACE -fsycl)
  set(PEANO_FLAGS ${PEANO_FLAGS} -fsycl CACHE INTERNAL "")
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
  "sm_89"
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
  set(PEANO_FLAGS ${PEANO_FLAGS} -cuda CACHE INTERNAL "")
endif()

if("${WITH_GPU}" STREQUAL "cuda")
  set(ENABLE_CUDA_COMPILATION_OPTIONS ON)
  cmake_dependent_option(CUDA_SEPARABLE_COMPILATION "CUDA separable compilation" ON "ENABLE_CUDA_COMPILATION_OPTIONS" OFF)
  cmake_dependent_option(CUDA_RESOLVE_DEVICE_SYMBOLS "CUDA resolve device symbols" ON "ENABLE_CUDA_COMPILATION_OPTIONS" OFF)
  find_package(CUDAToolkit REQUIRED)
  target_link_libraries(project_options INTERFACE CUDA::toolkit)
  set(PEANO_DEPENDS ${PEANO_DEPENDS} CUDA::toolkit CACHE INTERNAL "")
else()
  set(ENABLE_CUDA_COMPILATION_OPTIONS OFF)
endif()

if(WITH_GPU)
  string(TOUPPER ${WITH_GPU} WITH_GPU_UPPER)
  target_compile_definitions(project_options INTERFACE GPUOffloading${WITH_GPU_UPPER})
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} GPUOffloading${WITH_GPU_UPPER} CACHE INTERNAL "")
  if(WITH_GPU STREQUAL "omp")
    find_package(OpenMPTarget REQUIRED)
    target_link_libraries(project_options INTERFACE OpenMP::OpenMPTarget)
    set(PEANO_FLAGS ${PEANO_FLAGS} ${OpenMP_Offloading_CXX_FLAGS} CACHE INTERNAL "")
    set(PEANO_LINK_FLAGS ${PEANO_LINK_FLAGS} ${OpenMP_Offloading_LINK_LIBRARIES} CACHE INTERNAL "")
    set(PEANO_DEPENDS ${PEANO_DEPENDS} OpenMP::OpenMPTarget CACHE INTERNAL "")
  elseif(WITH_GPU STREQUAL "cpp")
    if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
      string(SUBSTRING "${WITH_GPU_ARCH}" 3 5 GPU_ARCH_SUFFIX)
      target_link_libraries(project_options INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
      target_compile_options(project_options INTERFACE -mp=gpu -gpu=cc${GPU_ARCH_SUFFIX} -stdpar=gpu)
    endif()
  elseif(WITH_GPU STREQUAL "sycl")
    if(NVIDIA_GPU)
      target_compile_options(project_options INTERFACE -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
      target_link_options(project_options INTERFACE -fsycl -fsycl-targets=nvptx64-nvidia-cuda -Xsycl-target-backend=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
    elseif(AMD_GPU)
      target_compile_options(project_options INTERFACE -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
      target_link_options(project_options INTERFACE -fsycl -fsycl-targets=amdgcn-amd-amdhsa -Xsycl-target-backend=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
    endif()
  elseif(WITH_GPU STREQUAL "hip")
    message(WARNING "Currently the flags for HIP need to be passed manually")
  elseif(WITH_GPU STREQUAL "cuda")
    set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
    set(CMAKE_CUDA_STANDARD 17)
    enable_language(CUDA)
    option(CUDA_SEPARABLE_COMPILATION "CUDA separable compilation" ON)
    option(CUDA_RESOLVE_DEVICE_SYMBOLS "CUDA resolve device symbols" ON)
    find_package(CUDAToolkit REQUIRED)
    target_link_libraries(project_options INTERFACE CUDA::toolkit)
    string(SUBSTRING "${WITH_GPU_ARCH}" 3 5 GPU_ARCH_SUFFIX)
    set(CMAKE_CUDA_ARCHITECTURES ${GPU_ARCH_SUFFIX} CACHE INTERNAL "CUDA architecture" FORCE)
  endif()
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug;Release;Asserts;Trace;Stats" CACHE STRING "Choose the type of build (Debug, Release, Asserts, Trace, Stats)" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS " " "Debug" "Release" "Asserts" "Trace" "Stats" "Debug\;Release\;Asserts\;Trace\;Stats")
set(WITH_DIMENSIONS "2;3" CACHE STRING "Dimensions to build with ;-separated list of values")

if(PEANO_BUILD_TYPE)
  if(CMAKE_BUILD_TYPE)
    list(FIND CMAKE_BUILD_TYPE ${PEANO_BUILD_TYPE} index)
    if(index EQUAL -1)
      set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} ${PEANO_BUILD_TYPE} CACHE STRING "List of build types" FORCE)
    endif()
  else()
    set(CMAKE_BUILD_TYPE ${PEANO_BUILD_TYPE} CACHE STRING "List of build types" FORCE)
  endif()
endif()

if(PEANO_DIMENSIONS)
  foreach(DIMENSION IN LISTS PEANO_DIMENSIONS)
    list(FIND WITH_DIMENSIONS "${PEANO_DIMENSIONS}" index)
    if(index EQUAL -1)
      if (WITH_DIMENSIONS)
        set(WITH_DIMENSIONS "${WITH_DIMENSIONS};${DIMENSION}" CACHE STRING "Dimensions to build with ;-separated list of values" FORCE)
      else()
        set(WITH_DIMENSIONS "${DIMENSION}" CACHE STRING "Dimensions to build with ;-separated list of values" FORCE)
      endif()
    endif()
  endforeach()
endif()

# Note: linkers (ld, lld) ignore the flag -g when passed.
# Also see: https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_3.html
set(PEANO_CXX_FLAGS_RELEASE -O3 CACHE INTERNAL "")
set(PEANO_CUDA_FLAGS_RELEASE -O3 CACHE INTERNAL "")
set(PEANO_RELEASE_DEFINITIONS -DPeanoDebug=0 -DNDEBUG CACHE INTERNAL "")

#-O2 -fno-omit-frame-pointer
set(PEANO_CXX_FLAGS_DEBUG -g -O0 CACHE INTERNAL "")
set(PEANO_CUDA_FLAGS_DEBUG -g -O0 CACHE INTERNAL "")
set(PEANO_DEBUG_DEFINITIONS -DPeanoDebug=4 CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_STATS ${PEANO_CXX_FLAGS_RELEASE} CACHE INTERNAL "")
set(PEANO_CUDA_FLAGS_STATS ${PEANO_CUDA_FLAGS_RELEASE} CACHE INTERNAL "")
set(PEANO_STATS_DEFINITIONS -DPeanoDebug=0 -DTrackStatistics CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_ASSERTS ${PEANO_CXX_FLAGS_DEBUG} CACHE INTERNAL "")
set(PEANO_CUDA_FLAGS_ASSERTS ${PEANO_CUDA_FLAGS_DEBUG} CACHE INTERNAL "")
set(PEANO_ASSERTS_DEFINITIONS -DPeanoDebug=2 CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_TRACE -g -O2 CACHE INTERNAL "")
set(PEANO_CUDA_FLAGS_TRACE ${PEANO_CXX_FLAGS_TRACE} CACHE INTERNAL "")
set(PEANO_TRACE_DEFINITIONS -DPeanoDebug=1 -DNDEBUG CACHE INTERNAL "")

option(WITH_MPI "Build with Message Passing Interface (MPI) support" OFF)
option(WITH_TARGETDART "Build with targetDART support" OFF)
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
  set(PEANO_INCLUDES ${PEANO_INCLUDES} ${MPI_INCLUDE_PATH} CACHE INTERNAL "")
  target_compile_definitions(project_options INTERFACE OMPI_SKIP_MPICXX) # To prevent compiler warnings
  target_compile_definitions(project_options INTERFACE Parallel)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} OMPI_SKIP_MPICXX Parallel CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE MPI::MPI_CXX)
  set(PEANO_DEPENDS ${PEANO_DEPENDS} MPI::MPI_CXX CACHE INTERNAL "")
endif()

if(WITH_OTTER)
  find_package(Otter CONFIG)
  target_compile_definitions(project_options INTERFACE UseOtter)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseOtter CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE Otter::Otter)
endif()

if(WITH_PETSC)
  find_package(PETSc REQUIRED)
  target_compile_definitions(project_options INTERFACE UsePETSc)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UsePETSc CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE ${PETSc_LIBRARIES})
  target_include_directories(project_options INTERFACE ${PETSc_INCLUDE_DIRS})
  set(PEANO_INCLUDES ${PEANO_INCLUDESS} ${PETSc_INCLUDE_DIRS} CACHE INTERNAL "")
endif()

if(WITH_TARGETDART)
  target_compile_definitions(project_options INTERFACE UseTargetDART)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseTargetDART CACHE INTERNAL "")
endif()

if(WITH_LIBXSMM)
  find_package(LIBXSMM REQUIRED)
  target_compile_definitions(project_options INTERFACE UseLibXSMM)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseLibXSMM CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE LIBXSMM::LIBXSMM)
endif()

if(WITH_VTK)
  find_package(VTK REQUIRED)
  target_compile_definitions(project_options INTERFACE UseVTK)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseVTK CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE VTK::CommonCore)
endif()

if(WITH_HDF5)
  find_package(HDF5 REQUIRED)
  target_compile_definitions(project_options INTERFACE UseHDF5)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseHDF5 CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE HDF5::HDF5)
endif()

if(WITH_NETCDF)
  find_package(NetCDF REQUIRED)
  target_compile_definitions(project_options INTERFACE UseNetCDF)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseNetCDF CACHE INTERNAL "")
  target_link_libraries(project_options INTERFACE ${NETCDF_LIBRARIES} ${NETCDF_LIBRARIES_CXX})
  target_include_directories(project_options INTERFACE ${NETCDF_INCLUDES} CACHE INTERNAL "")
  set(PEANO_INCLUDES ${PEANO_INCLUDESS} ${NETCDF_INCLUDES})
endif()

if(ENABLE_EXAHYPE)
  #target_compile_definitions(project_options INTERFACE UseExaHyPE)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseExaHyPE CACHE INTERNAL "")
  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseExaHyPE CACHE INTERNAL "")
endif()

if(ENABLE_SWIFT)
  #target_compile_definitions(project_options INTERFACE UseSWIFT)
  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} UseSWIFT CACHE INTERNAL "")
  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseSWIFT CACHE INTERNAL "")
endif()

#if(ENABLE_FINITEELEMENTS)
#  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} ENABLE_FINITELEMENTS CACHE INTERNAL "")
#  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseExaHyPE CACHE INTERNAL "")
#endif()

#if(ENABLE_MULTIPRECISION)
#  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} ENABLE_MULTIPRECISION CACHE INTERNAL "")
#  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseToolboxMultiprecision CACHE INTERNAL "")
#endif()

#if(ENABLE_LOADBALANCING)
#  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} ENABLE_LOADBALANCING CACHE INTERNAL "")
#  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseToolboxLoadBalancing CACHE INTERNAL "")
#endif()

#if(ENABLE_PARTICLES)
#  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} ENABLE_PARTICLES CACHE INTERNAL "")
#  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseToolboxParticles CACHE INTERNAL "")
#endif()

#if(ENABLE_BLOCKSTRUCTURED)
#  set(PEANO_DEFINITIONS ${PEANO_DEFINITIONS} ENABLE_BLOCKSTRUCTURED CACHE INTERNAL "")
#  set(USE_COMPONENT_DEFINITIONS ${USE_COMPONENT_DEFINITIONS} -DUseToolboxBlocstructured CACHE INTERNAL "")
#endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${META_PROJECT_NAME}ConfigVersion.cmake
  VERSION ${META_VERSION_REVISION}
  COMPATIBILITY SameMajorVersion
)

function(create_symlinks source_dir dest_dir)
  file(MAKE_DIRECTORY "${dest_dir}")
  file(GLOB_RECURSE items LIST_DIRECTORIES TRUE RELATIVE "${source_dir}" "${source_dir}/*")
  foreach(item IN LISTS items)
    set(source_dir_item "${source_dir}/${item}")
    set(dest_dir_item "${dest_dir}/${item}")
    if(IS_DIRECTORY ${source_dir_item})
      file(MAKE_DIRECTORY "${dest_dir_item}")
    else()
      get_filename_component(dest_dir_item_path "${dest_dir_item}" DIRECTORY)
      file(RELATIVE_PATH relative_source_item "${dest_dir_item_path}" "${source_dir_item}")
      execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${relative_source_item}" "${dest_dir_item}")
    endif()
  endforeach()
endfunction()

list(APPEND DIRECTORIES_TO_LINK_OVER "python" "tutorials" "benchmarks" "applications" "documentation" "cmake")
foreach(directory IN LISTS DIRECTORIES_TO_LINK_OVER)
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/${directory}")
    create_symlinks("${CMAKE_SOURCE_DIR}/${directory}" "${CMAKE_BINARY_DIR}/${directory}")
  endif()
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/${directory}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
endforeach()

file(TOUCH ${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)
include(${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${CMAKE_BINARY_DIR}/GeneratedSubdirectory.cmake)

install(FILES
  ${CMAKE_SOURCE_DIR}/LICENSE
  ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md
  ${CMAKE_SOURCE_DIR}/CODE_OF_CONDUCT.md
  ${CMAKE_BINARY_DIR}/Config.cmake
  DESTINATION "${CMAKE_INSTALL_PREFIX}"
)

install(FILES
  ${CMAKE_SOURCE_DIR}/src/config.h
  DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
)

set(PYTHON_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/parse_cmake_cache.py")

add_custom_target(run_script ALL
     COMMAND echo "${PYTHON3_EXECUTABLE} ${PYTHON_SCRIPT} ${CMAKE_BINARY_DIR}/CMakeCache.txt"
     COMMAND ${PYTHON_EXECUTABLE} ${PYTHON_SCRIPT} "${CMAKE_BINARY_DIR}/CMakeCache.txt"
     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
     COMMENT "Running Python script after build")

add_subdirectory(src)

#file(GLOB_RECURSE UNIT_TEST_SOURCES CONFIGURE_DEPENDS "*/tests/*")
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
display_variable(WITH_TARGETDART)
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

display_subseparator("Options for Sanitisers")
display_variable(SANITISE_ADDRESS)
display_variable(SANITISE_UNDEFINED)
display_variable(SANITISE_THREAD)

display_subseparator("Options to Build")
display_variable(CMAKE_C_COMPILER)
display_variable(CMAKE_C_STANDARD)
display_variable(CMAKE_CXX_COMPILER)
display_variable(CMAKE_CXX_STANDARD)
display_variable(MPI_CXX_COMPILER)
display_variable(CMAKE_BUILD_TYPE)
display_variable(PEANO_CXX_FLAGS_RELEASE)
display_variable(PEANO_RELEASE_DEFINITIONS)
display_variable(PEANO_CXX_FLAGS_DEBUG)
display_variable(PEANO_DEBUG_DEFINITIONS)
display_variable(PEANO_CXX_FLAGS_STATS)
display_variable(PEANO_STATS_DEFINITIONS)
display_variable(PEANO_CXX_FLAGS_ASSERTS)
display_variable(PEANO_ASSERTS_DEFINITIONS)
display_variable(PEANO_CXX_FLAGS_TRACE)
display_variable(PEANO_TRACE_DEFINITIONS)
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
  display_variable(CMAKE_CUDA_HOST_COMPILER)
  display_variable(CMAKE_CUDA_ARCHITECTURES)
  display_variable(CUDA_SEPARABLE_COMPILATION)
  display_variable(CUDA_RESOLVE_DEVICE_SYMBOLS)
endif()

display_subseparator("Options to Install")
display_variable(CMAKE_INSTALL_FULL_BINDIR)
display_variable(CMAKE_INSTALL_FULL_LIBDIR)

display_subseparator("End of Summary")
