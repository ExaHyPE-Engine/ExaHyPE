include(FindPackageHandleStandardArgs)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

cmake_push_check_state()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
  if(NVIDIA_GPU)
    find_package(CUDA REQUIRED)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=nvptx64-nvidia-cuda;-Xopenmp-target=nvptx64-nvidia-cuda;-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lomptarget")
  elseif(AMD_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-fopenmp-targets=amdgcn-amd-amdhsa;-Xopenmp-target=amdgcn-amd-amdhsa;-march=${WITH_GPU_ARCH}")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lomptarget")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  if(NVIDIA_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-foffload=nvptx-none;-march=native")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lgomp")
  elseif(AMD_GPU)
    set(OpenMP_Offloading_CXX_FLAGS "-fopenmp;-foffload=amdgcn-amdhs;-march=native")
    set(OpenMP_Offloading_LINK_LIBRARIES "-lgomp")
  endif()
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  string(REPLACE "sm_" "cc" OMP_DEVICE_ARCH "${WITH_GPU_ARCH}")
  set(OpenMP_Offloading_CXX_FLAGS "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}") # -cuda
  set(OpenMP_Offloading_LINK_LIBRARIES "-mp=gpu;-gpu=${OMP_DEVICE_ARCH}") # -cuda
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

add_library(OpenMP::OpenMPTarget INTERFACE IMPORTED)
set_property(TARGET OpenMP::OpenMPTarget APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
  $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_Offloading_CXX_FLAGS}>)
target_link_libraries(OpenMP::OpenMPTarget
  INTERFACE
    ${OpenMP_CXX_LIBRARIES}
    ${OpenMP_Offloading_CXX_FLAGS}
    ${OpenMP_Offloading_LINK_LIBRARIES})

#find_package_handle_standard_args(OpenMTarget)
mark_as_advanced(OpenMP_Offloading_CXX_FLAGS OpenMP_Offloading_LINK_LIBRARIES)
cmake_pop_check_state()
