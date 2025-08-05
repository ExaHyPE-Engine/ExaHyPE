# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

if(WITH_GPU_ARCH STREQUAL "none")
  message(FATAL_ERROR "No GPU architecture chosen. Make sure to also set WITH_GPU_ARCH")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  set(OFFLOADING_FLAGS -mp=gpu -stdpar=gpu -gpu=${WITH_GPU_ARCH})
endif()

if(OFFLOADING_FLAGS)
  target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OFFLOADING_FLAGS}>)
  target_link_options(PeanoOptions INTERFACE ${OFFLOADING_FLAGS})
  message(STATUS "Using StdPar offloading flags: ${OFFLOADING_FLAGS}")
endif()

target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_STDPAR WITH_GPU)
