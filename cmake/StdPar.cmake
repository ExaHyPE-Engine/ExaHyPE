# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  target_compile_options(PeanoInterface INTERFACE $<$<COMPILE_LANGUAGE:CXX>:-mp=gpu -stdpar=gpu -gpu=${WITH_GPU_ARCH}>)
  target_link_options(PeanoInterface INTERFACE -mp=gpu -stdpar=gpu -gpu=${WITH_GPU_ARCH})
endif()

target_compile_definitions(PeanoInterface INTERFACE GPUOffloadingCPP)
