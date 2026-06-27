# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

find_package(OpenMP REQUIRED)
set(OMPT_FOUND "${OpenMP_FOUND}")

if(TARGET PeanoOptions)
  if(NOT DEFINED PEANO_WITH_GPU_ARCH OR "${PEANO_WITH_GPU_ARCH}" STREQUAL "none")
    message(FATAL_ERROR "PEANO_WITH_GPU_ARCH must be set to one of: sm_<cc>, gfx<arch>, spir64")
  endif()

  if(${CMAKE_CXX_COMPILER_ID} MATCHES ".*[Cc]lang")
    if(PEANO_WITH_GPU_VENDOR STREQUAL "nvidia")
      set(OFFLOADING_FLAGS -fopenmp-targets=nvptx64-nvidia-cuda --offload-arch=${PEANO_WITH_GPU_ARCH})
    elseif(PEANO_WITH_GPU_VENDOR STREQUAL "amd")
      set(OFFLOADING_FLAGS -fopenmp-targets=amdgcn-amd-amdhsa --offload-arch=${PEANO_WITH_GPU_ARCH})
    elseif(PEANO_WITH_GPU_VENDOR STREQUAL "intel")
      set(OFFLOADING_FLAGS -fopenmp-targets=spir64)
    endif()

  elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
    if(PEANO_WITH_GPU_VENDOR STREQUAL "nvidia")
      set(OFFLOADING_FLAGS -foffload=nvptx-none)
    elseif(PEANO_WITH_GPU_VENDOR STREQUAL "amd")
      set(OFFLOADING_FLAGS -foffload=amdgcn-amdhsa)
    endif()

  elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    if(PEANO_WITH_GPU_VENDOR STREQUAL "nvidia")
      string(REPLACE "sm_" "cc" NVHPC_ARCH "${PEANO_WITH_GPU_ARCH}")
      set(OFFLOADING_FLAGS -mp=gpu -gpu=${NVHPC_ARCH})
    endif()

  elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Intel|IntelLLVM")
    if(PEANO_WITH_GPU_VENDOR STREQUAL "intel")
      set(OFFLOADING_FLAGS -fopenmp-targets=spir64)
    endif()
  endif()

  if(OFFLOADING_FLAGS)
    target_link_libraries(PeanoOptions INTERFACE OpenMP::OpenMP_CXX)
    target_compile_options(PeanoOptions INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${OFFLOADING_FLAGS}>)
    target_link_options(PeanoOptions INTERFACE ${OFFLOADING_FLAGS} ${OpenMP_CXX_FLAGS})
    message(STATUS "Using OMPT offloading flags: ${OFFLOADING_FLAGS}")
    target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_OMPT PEANO_WITH_GPU_BACKEND)
  endif()
endif()
