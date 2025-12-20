# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

string(REGEX MATCH "[0-9]+$" CUDA_ARCH "${WITH_GPU_ARCH}")
message(STATUS "Using CUDA_ARCH=${CUDA_ARCH}")

set(CMAKE_CUDA_HOST_COMPILER "${CMAKE_CXX_COMPILER}")
enable_language(CUDA)

set(CMAKE_CUDA_ARCHITECTURES "${CUDA_ARCH}" CACHE STRING "" FORCE)

find_package(CUDAToolkit REQUIRED)
set(GPU_FIND_DEPENDENCY "find_dependency(CUDAToolkit)" CACHE INTERNAL "")

target_link_libraries(PeanoOptions INTERFACE
  CUDA::toolkit
  CUDA::cudart
  CUDA::cublas
)

target_compile_options(PeanoOptions INTERFACE
  -fPIC
  $<$<COMPILE_LANGUAGE:CUDA>:-gencode arch=compute_${CUDA_ARCH},code=sm_${CUDA_ARCH}>
  $<$<COMPILE_LANGUAGE:CUDA>:-Wno-gnu-line-marker>
  $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-gnu-line-marker>
)

target_compile_features(PeanoOptions INTERFACE cuda_std_17)

target_compile_definitions(PeanoOptions INTERFACE
   WITH_GPU_CUDA
   WITH_GPU
)

if(CMAKE_CUDA_COMPILER)
  set(WITH_CUDA ON  CACHE INTERNAL "CUDA support is enabled")
else()
  set(WITH_CUDA OFF CACHE INTERNAL "CUDA support is disabled")
endif()

function(add_cuda_to_target target)
  set_target_properties(${target} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CUDA_SEPARABLE_COMPILATION OFF
    CUDA_RESOLVE_DEVICE_SYMBOLS OFF
    CUDA_ARCHITECTURES "${CUDA_ARCH}"
  )
endfunction()
