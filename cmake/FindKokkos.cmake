# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

set(KOKKOS_VERSION "5.0.1" CACHE STRING "")

function(_kokkos_build_user_args out_var)
  if(NOT DEFINED PEANO_WITH_GPU_ARCH OR "${PEANO_WITH_GPU_ARCH}" STREQUAL "none")
    message(FATAL_ERROR "PEANO_WITH_GPU_ARCH must be set to one of: sm_<cc>, gfx<arch>, spir64")
  endif()

  set(_args
    "-DKokkos_ENABLE_SERIAL=ON"
    "-DKokkos_ENABLE_OPENMP=OFF"
    "-DKokkos_ARCH_NATIVE=ON"
  )

  string(TOLOWER "${PEANO_WITH_GPU_ARCH}" _w)

  if(_w MATCHES "^sm_([0-9]+)$")
    list(APPEND _args "-DKokkos_ENABLE_CUDA=ON" "-DKokkos_ENABLE_CUDA_RELOCATABLE_DEVICE_CODE=OFF")
    set(_sm "${CMAKE_MATCH_1}")
    set(_CUDA_MAP
      60  PASCAL60
      61  PASCAL61
      70  VOLTA70
      72  VOLTA72
      75  TURING75
      80  AMPERE80
      86  AMPERE86
      87  AMPERE87
      89  ADA89
      90  HOPPER90
      100 BLACKWELL100
      120 BLACKWELL120
    )
    list(FIND _CUDA_MAP "${_sm}" _idx)
    if(_idx GREATER -1)
      math(EXPR _val_idx "${_idx} + 1")
      list(GET _CUDA_MAP ${_val_idx} _kokkos_arch)
      list(APPEND _args "-DKokkos_ARCH_${_kokkos_arch}=ON")
    else()
      message(WARNING "PEANO_WITH_GPU_ARCH='${PEANO_WITH_GPU_ARCH}' has no Kokkos NVIDIA arch option in Kokkos ${KOKKOS_VERSION}; enabling CUDA without a specific Kokkos_ARCH_* flag")
    endif()

  elseif(_w MATCHES "^gfx[0-9a-z]+$")
    list(APPEND _args "-DKokkos_ENABLE_HIP=ON")
    # A Kokkos release that does not know the gfx target fails its own configuration.
    string(TOUPPER "${_w}" _gfx_upper)
    list(APPEND _args "-DKokkos_ARCH_AMD_${_gfx_upper}=ON")

  elseif(_w STREQUAL "spir64")
    list(APPEND _args "-DKokkos_ENABLE_SYCL=ON")
  else()
    message(FATAL_ERROR "Invalid PEANO_WITH_GPU_ARCH='${PEANO_WITH_GPU_ARCH}'. Expected sm_<cc>, gfx<arch>, or spir64.")
  endif()

  set(${out_var} "${_args}" PARENT_SCOPE)
endfunction()

function(_kokkos_build_parallel_args out_var)
  if(DEFINED AVAILABLE_PROCESSOR_COUNT AND AVAILABLE_PROCESSOR_COUNT GREATER 0)
    set(_build_jobs "${AVAILABLE_PROCESSOR_COUNT}")
  else()
    include(ProcessorCount)
    ProcessorCount(_build_jobs)
  endif()

  if(DEFINED _build_jobs AND _build_jobs GREATER 0)
    set(${out_var} --parallel ${_build_jobs} PARENT_SCOPE)
  else()
    set(${out_var} "" PARENT_SCOPE)
  endif()
endfunction()

# An application uses the Kokkos package of the core build.
if(DEFINED PEANO_KOKKOS_DIR AND NOT PEANO_KOKKOS_DIR STREQUAL "")
  set(Kokkos_DIR "${PEANO_KOKKOS_DIR}" CACHE PATH "Kokkos package config dir (from Peano core build)" FORCE)
endif()

# The CONFIG mode bypasses this module, so the call does not recurse.
find_package(Kokkos CONFIG QUIET)

if(NOT Kokkos_FOUND)
  message(STATUS "Kokkos ${KOKKOS_VERSION} not found; fetching sources...")
  include(FetchContent)
  set(FETCHCONTENT_QUIET ON)

  _kokkos_build_user_args(Kokkos_USER_CMAKE_ARGS)
  _kokkos_build_parallel_args(Kokkos_BUILD_PARALLEL_ARGS)

  FetchContent_GetProperties(Kokkos)
  if(NOT Kokkos_POPULATED)
    FetchContent_Populate(Kokkos
      URL https://github.com/kokkos/kokkos/archive/refs/tags/${KOKKOS_VERSION}.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
  endif()

  set(kokkos_BINARY_DIR ${CMAKE_BINARY_DIR}/kokkos-build)
  set(kokkos_INSTALL_DIR ${CMAKE_BINARY_DIR}/kokkos-install)
  file(MAKE_DIRECTORY ${kokkos_BINARY_DIR})
  file(MAKE_DIRECTORY ${kokkos_INSTALL_DIR})

  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -G ${CMAKE_GENERATOR}
      -S ${kokkos_SOURCE_DIR}
      -B ${kokkos_BINARY_DIR}
      -DCMAKE_INSTALL_PREFIX=${kokkos_INSTALL_DIR}
      -DCMAKE_BUILD_TYPE=Release
      -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      ${Kokkos_USER_CMAKE_ARGS}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    # ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "CMake step for Kokkos failed: ${result}")
  endif()

  execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${kokkos_BINARY_DIR} --config Release --target install ${Kokkos_BUILD_PARALLEL_ARGS}
    RESULT_VARIABLE result
    OUTPUT_QUIET
    # ERROR_QUIET
  )
  if(result)
    message(FATAL_ERROR "Build step for Kokkos failed: ${result}")
  endif()

  message(STATUS "Kokkos was successfully installed at ${kokkos_INSTALL_DIR}")
  find_package(Kokkos CONFIG REQUIRED HINTS ${kokkos_INSTALL_DIR})
endif()

message(STATUS "Found Kokkos: ${Kokkos_DIR}")

if(TARGET PeanoOptions)
  target_link_libraries(PeanoOptions INTERFACE Kokkos::kokkos)
  target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_GPU_BACKEND_KOKKOS PEANO_WITH_GPU_BACKEND)
  # The nvcc_wrapper script passes -Werror to the host compiler only.
  if(Kokkos_CXX_COMPILER_ID STREQUAL "NVIDIA")
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANGUAGE:CXX>:--expt-relaxed-constexpr>
      $<$<COMPILE_LANGUAGE:CXX>:-Werror=all-warnings>
    )
  endif()
endif()
