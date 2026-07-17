# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

macro(_peano_resolve_gpu_backend out_backend)
  if(DEFINED _peano_gpu_backend)
    set(${out_backend} "${_peano_gpu_backend}")
  elseif(DEFINED PEANO_WITH_GPU_BACKEND)
    set(${out_backend} "${PEANO_WITH_GPU_BACKEND}")
  else()
    set(${out_backend} "no")
  endif()
endmacro()

macro(_peano_resolve_gpu_arch out_arch)
  if(DEFINED PEANO_WITH_GPU_ARCH)
    set(${out_arch} "${PEANO_WITH_GPU_ARCH}")
  else()
    set(${out_arch} "none")
  endif()
endmacro()

macro(peano_enable_cuda_language)
  _peano_resolve_gpu_arch(_peano_cuda_gpu_arch)
  string(REGEX MATCH "[0-9]+$" _peano_cuda_arch "${_peano_cuda_gpu_arch}")
  message(STATUS "Using CUDA_ARCH=${_peano_cuda_arch}")

  set(CMAKE_CUDA_HOST_COMPILER "${CMAKE_CXX_COMPILER}")
  enable_language(CUDA)
  set(CMAKE_CUDA_ARCHITECTURES "${_peano_cuda_arch}" CACHE STRING "" FORCE)
endmacro()

macro(peano_enable_hip_language)
  _peano_resolve_gpu_arch(_peano_hip_arch)
  message(STATUS "Using HIP_ARCH=${_peano_hip_arch}")

  set(HIP_PLATFORM "amd" CACHE STRING "" FORCE)
  set(CMAKE_HIP_PLATFORM "amd" CACHE STRING "" FORCE)
  set(CMAKE_HIP_HOST_COMPILER "${CMAKE_CXX_COMPILER}")
  enable_language(HIP)
  set(CMAKE_HIP_ARCHITECTURES "${_peano_hip_arch}" CACHE STRING "" FORCE)
endmacro()

macro(peano_enable_gpu_language)
  _peano_resolve_gpu_backend(_peano_gpu_backend)

  # ${ARGN} is the explicit list of GPU kernel sources emitted by the code generator (GENERATED_GPU_SOURCES).
  set(_peano_gpu_sources ${ARGN})

  if(_peano_gpu_sources AND _peano_gpu_backend STREQUAL "cuda")
    peano_enable_cuda_language()
    foreach(_peano_src ${_peano_gpu_sources})
      message(STATUS "Setting CUDA language for file: ${_peano_src}")
      set_source_files_properties(${_peano_src} PROPERTIES LANGUAGE CUDA)
    endforeach()
  elseif(_peano_gpu_sources AND _peano_gpu_backend STREQUAL "hip")
    peano_enable_hip_language()
    foreach(_peano_src ${_peano_gpu_sources})
      message(STATUS "Setting HIP language for file: ${_peano_src}")
      set_source_files_properties(${_peano_src} PROPERTIES LANGUAGE HIP)
    endforeach()
  endif()
endmacro()
