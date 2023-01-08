include(FindPackageHandleStandardArgs)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

cmake_push_check_state()

if(NOT WITH_GPU_ARCH IN_LIST ACCELERATOR_ARCHITECTURES)
  message(FATAL_ERROR "GPU architechture must be one of: ${ACCELERATOR_ARCHITECTURES}")
endif()

if(NOT DEFINED WITH_GPU_ARCH OR WITH_GPU_ARCH STREQUAL " ")
  message(FATAL_ERROR "GPU architechture is not defined")
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  if(NVIDIA_GPU)
    find_package(CUDA REQUIRED)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=nvptx64-nvidia-cuda;-Xopenmp-target;-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lomptarget")
  elseif(AMD_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=amdgcn-amd-amdhsa;-Xopenmp-target;-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lomptarget")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  if(NVIDIA_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-foffload-options=nvptx-none=-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lgomp")
  elseif(AMD_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-foffload-options==amdgcn-amdhs=-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lgomp")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    string(REPLACE "sm_" "cc" OMP_DEVICE_ARCH "${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_CXX_FLAGS "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "IntelLLVM")
    set(OpenMP_Offloading_CXX_FLAGS "-fiopenmp;-fopenmp-targets=spir64")
    set(OpenMP_Offloading_LINK_LIBRARIES "-fiopenmp")
elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
endif()

set(CMAKE_REQUIRED_FLAGS "${OpenMP_CXX_FLAGS};${OpenMP_Offloading_CXX_FLAGS}")
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
set(CMAKE_REQUIRED_INCLUDES "${OpenMP_CXX_INCLUDE_DIRS}")
set(CMAKE_REQUIRED_LIBRARIES "${OpenMP_CXX_LIBRARIES};${OpenMP_Offloading_LINK_LIBRARIES}")
set(CMAKE_REQUIRED_LINK_OPTIONS "${OpenMP_CXX_LINK_OPTIONS}")
message(STATUS "${CMAKE_REQUIRED_FLAGS}")

check_cxx_source_runs("
  #include <omp.h>
  #define YES 0
  #define NO 1
  int main(int argc, char* argv[]) {
    int canOffload = NO;
    #pragma omp target map(tofrom: canOffload)
    {
      if (!omp_is_initial_device()) {
        canOffload = YES;
      }
    }
      return canOffload;
  }
  " OPENMP_OFFLOADING_RESULT
)

if(OPENMP_OFFLOADING_RESULT)
  add_library(OpenMP::OpenMP_CXX_with_offloading INTERFACE IMPORTED)
  set_property(TARGET OpenMP::OpenMP_CXX_with_offloading APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
    $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_Offloading_CXX_FLAGS}>)
    target_link_libraries(OpenMP::OpenMP_CXX_with_offloading
      INTERFACE
        ${OpenMP_CXX_LIBRARIES}
        ${OpenMP_Offloading_CXX_FLAGS}
        ${OpenMP_Offloading_LINK_LIBRARIES})
    endif()
else()
  message(FATAL_ERROR "OpenMP GPU offloading was demanded but it was not set."
    "Check your compiler whether it can perform OpenMP GPU offloading.")
endif()

find_package_handle_standard_args(OpenMP_Offloading REQUIRED_VARS OPENMP_OFFLOADING_RESULT)
mark_as_advanced(OpenMP_Offloading_CXX_FLAGS OpenMP_Offloading_LINK_LIBRARIES OPENMP_OFFLOADING_RESULT)
cmake_pop_check_state()
