# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_package(CUDAToolkit REQUIRED)
set(CUDA_FOUND "${CUDAToolkit_FOUND}")

if(TARGET PeanoOptions)
  target_link_libraries(PeanoOptions INTERFACE
    CUDA::toolkit
    CUDA::cudart
    CUDA::cublas
  )
  target_compile_options(PeanoOptions INTERFACE
    -fPIC
    $<$<COMPILE_LANGUAGE:CUDA>:-gencode arch=compute_${CMAKE_CUDA_ARCHITECTURES},code=sm_${CMAKE_CUDA_ARCHITECTURES}>
    $<$<COMPILE_LANGUAGE:CUDA>:-Wno-gnu-line-marker>
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-gnu-line-marker>
  )
  target_compile_features(PeanoOptions INTERFACE cuda_std_17)
  target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_CUDA PEANO_WITH_GPU_BACKEND)
endif()
