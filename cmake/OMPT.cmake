# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

if(NOT DEFINED WITH_GPU_ARCH OR "${WITH_GPU_ARCH}" STREQUAL "none")
  message(FATAL_ERROR "WITH_GPU_ARCH must be set to one of: sm_<cc>, gfx<arch>, spir64")
endif()

if(${CMAKE_CXX_COMPILER_ID} MATCHES ".*Clang")
  if(WITH_GPU_VENDOR STREQUAL "nvidia")
    set(OFFLOADING_FLAGS -fopenmp-targets=nvptx64-nvidia-cuda --offload-arch=${WITH_GPU_ARCH})
  elseif(WITH_GPU_VENDOR STREQUAL "amd")
    set(OFFLOADING_FLAGS -fopenmp-targets=amdgcn-amd-amdhsa --offload-arch=${WITH_GPU_ARCH})
  elseif(WITH_GPU_VENDOR STREQUAL "intel")
    set(OFFLOADING_FLAGS -fopenmp-targets=spir64)
  endif()

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  if(WITH_GPU_VENDOR STREQUAL "nvidia")
    set(OFFLOADING_FLAGS -foffload=nvptx-none)
  elseif(WITH_GPU_VENDOR STREQUAL "amd")
    set(OFFLOADING_FLAGS -foffload=amdgcn-amdhs)
  endif()

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
  if(WITH_GPU_VENDOR STREQUAL "nvidia")
    string(REPLACE "sm_" "cc" NVHPC_ARCH "${WITH_GPU_ARCH}")
    set(OFFLOADING_FLAGS -mp=gpu -gpu=${NVHPC_ARCH})
  endif()

elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel|IntelLLVM")
  if(WITH_GPU_VENDOR STREQUAL "intel")
    set(OFFLOADING_FLAGS -fopenmp-targets=spir64)
  endif()
endif()

if(OFFLOADING_FLAGS)
  find_package(OpenMP REQUIRED)
  check_include_files(omp.h HAVE_OMP_H)

  target_link_libraries(PeanoOptions INTERFACE OpenMP::OpenMP_CXX)
  target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OFFLOADING_FLAGS}>)
  target_link_options(PeanoOptions INTERFACE ${OFFLOADING_FLAGS} ${OpenMP_CXX_FLAGS})

  set(ONLY_IF_GPU_FIND_DEPENDENCY "find_dependency(OpenMP)")
  message(STATUS "Using OMPT offloading flags: ${OFFLOADING_FLAGS}")

  target_compile_definitions(PeanoOptions INTERFACE WITH_GPU_OMPT WITH_GPU)
endif()
