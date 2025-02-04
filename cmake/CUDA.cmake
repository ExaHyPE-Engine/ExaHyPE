# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

set(CMAKE_CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})
set(CMAKE_CUDA_STANDARD 17 CACHE STRING "")
set(CMAKE_CUDA_ARCHITECTURES ${GPU_ARCH_SUFFIX} CACHE INTERNAL "CUDA architecture" FORCE)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler")

set_property(TARGET PeanoOptions PROPERTY CUDA_SEPARABLE_COMPILATION OFF)
set_property(TARGET PeanoOptions PROPERTY CUDA_RESOLVE_DEVICE_SYMBOLS OFF)

enable_language(CUDA)

find_package(CUDAToolkit REQUIRED)
find_library(CUDART_LIBRARY cudart ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})
find_library(CUBLASLT_LIBRARY cublasLt ${CMAKE_CUDA_IMPLICIT_LINK_DIRECTORIES})

target_link_libraries(PeanoOptions INTERFACE CUDA::toolkit ${CUDART_LIBRARY} ${CUBLASLT_LIBRARY})
target_compile_definitions(PeanoOptions INTERFACE GPUOffloadingCUDA)
target_compile_options(PeanoOptions INTERFACE$<$<COMPILE_LANGUAGE:CUDA>:-gencode arch=compute_${GPU_ARCH_SUFFIX},code=sm_${GPU_ARCH_SUFFIX}>)
