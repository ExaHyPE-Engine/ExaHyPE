# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

if(WITH_GPU_ARCH STREQUAL "none")
  message(FATAL_ERROR "No GPU architecture chosen. Make sure to also set WITH_GPU_ARCH")
endif()

string(REGEX MATCH "[0-9]+$" CUDA_ARCH "${WITH_GPU_ARCH}")
message(STATUS "Using CUDA_ARCH=${CUDA_ARCH}")

set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
set(CMAKE_CUDA_ARCHITECTURES ${CUDA_ARCH})

enable_language(CUDA)

set_target_properties(PeanoOptions PROPERTIES CUDA_ARCHITECTURES ${CUDA_ARCH})
set_target_properties(PeanoOptions PROPERTIES CUDA_SEPARABLE_COMPILATION OFF)
set_target_properties(PeanoOptions PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS OFF)
set_target_properties(PeanoOptions PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(CUDAToolkit REQUIRED)
set(ONLY_IF_GPU_FIND_DEPENDENCY "find_dependency(CUDAToolkit)")

target_link_libraries(PeanoOptions INTERFACE CUDA::toolkit CUDA::cudart CUDA::cublas)
target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:-gencode arch=compute_${CUDA_ARCH},code=sm_${CUDA_ARCH}>)
target_compile_features(PeanoOptions INTERFACE cuda_std_17)
target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_CUDA WITH_GPU)
