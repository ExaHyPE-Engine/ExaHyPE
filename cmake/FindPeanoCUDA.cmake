# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_package(CUDAToolkit REQUIRED)
set(PeanoCUDA_FOUND "${CUDAToolkit_FOUND}")

if(TARGET PeanoOptions)
  target_link_libraries(PeanoOptions INTERFACE
    CUDA::toolkit
    CUDA::cudart
    CUDA::cublas
  )
  target_compile_options(PeanoOptions INTERFACE
    -fPIC
    $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wno-gnu-line-marker>
    $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
  )
  target_compile_features(PeanoOptions INTERFACE cuda_std_20)
  target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_CUDA PEANO_WITH_GPU_BACKEND)
endif()
