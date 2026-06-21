# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

option(PEANO_WITH_MULTITHREADING "Build with multithreading (OpenMP) support" ON)
option(PEANO_WITH_MPI "Build with Message Passing Interface (MPI) support" ON)
option(PEANO_WITH_FPE "Build with floating-point exception traps" OFF)
option(PEANO_WITH_HDF5 "Build with HDF5 support" ON)
option(PEANO_WITH_NETCDF "Build with NetCDF support" ON)
option(PEANO_WITH_EASI "Build with easi support" OFF)
option(PEANO_WITH_ZLIB "Build with ZLIB compression support" ON)
option(PEANO_WITH_LIBXSMM "Build with LIBXSMM support" OFF)
option(PEANO_WITH_CURVI "Build with Curvi toolbox support" OFF)

set(SUPPORTED_ACCELERATOR_ARCHITECTURES
  none
  sm_70 sm_72 sm_75 sm_80 sm_86 sm_87 sm_89 sm_90 sm_100 sm_103 sm_120 sm_121
  gfx906 gfx908 gfx90a gfx942 gfx950
  spir64
  CACHE STRING "Editable list of supported accelerator architectures"
)
mark_as_advanced(SUPPORTED_ACCELERATOR_ARCHITECTURES)

set(PEANO_WITH_GPU_ARCH "none" CACHE STRING "${SUPPORTED_ACCELERATOR_ARCHITECTURES}")
set_property(CACHE PEANO_WITH_GPU_ARCH PROPERTY STRINGS ${SUPPORTED_ACCELERATOR_ARCHITECTURES})

set(PEANO_WITH_GPU_BACKEND "none" CACHE STRING "GPU backend: none, native, kokkos, acpp, ompt")
set_property(CACHE PEANO_WITH_GPU_BACKEND PROPERTY STRINGS none native kokkos acpp ompt)

set(SUPPORTED_DIMENSIONS
  2 3
  CACHE STRING "Editable list of supported dimensions"
)
mark_as_advanced(SUPPORTED_DIMENSIONS)

set(CMAKE_BUILD_TYPE "" CACHE INTERNAL "")
set(SUPPORTED_BUILD_TYPES
  Release Debug Trace Asserts
  CACHE STRING "Editable list of supported build types"
)
mark_as_advanced(SUPPORTED_BUILD_TYPES)

set(PEANO_CXX_FLAGS_RELEASE -O3 -fomit-frame-pointer CACHE INTERNAL "")
set(PEANO_RELEASE_DEFINITIONS -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=0 -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "" FORCE)

set(PEANO_CXX_FLAGS_ASSERTS -g -O3 CACHE INTERNAL "")
set(PEANO_ASSERTS_DEFINITIONS -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=0 CACHE INTERNAL "" FORCE)

set(PEANO_CXX_FLAGS_TRACE -g -O3 -fomit-frame-pointer CACHE INTERNAL "")
set(PEANO_TRACE_DEFINITIONS -DPEANO_TRACE=1 -DPEANO_ASSERTIONS=0 -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "" FORCE)

set(PEANO_CXX_FLAGS_DEBUG -g -O0 CACHE INTERNAL "")
set(PEANO_DEBUG_DEFINITIONS -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=1 CACHE INTERNAL "" FORCE)

set(PEANO_EXTRA_CXX_FLAGS   "" CACHE STRING "Extra C++ flags (semicolon-separated)")
set(PEANO_EXTRA_DEFINITIONS "" CACHE STRING "Extra preprocessor definitions (semicolon-separated)")
set(PEANO_EXTRA_LINK_FLAGS  "" CACHE STRING "Extra linker flags (semicolon-separated)")

set(MAX_SPACETREES_PER_RANK "1024" CACHE STRING "Maximum number of spacetrees per MPI rank")
mark_as_advanced(MAX_SPACETREES_PER_RANK)

function(peano_create_options_target)
  add_library(PeanoOptions INTERFACE)
  target_compile_features(PeanoOptions INTERFACE
    c_std_${CMAKE_C_STANDARD}
    cxx_std_${CMAKE_CXX_STANDARD}
  )
  install(TARGETS PeanoOptions EXPORT ${PROJECT_NAME}Targets)

  if(${CMAKE_CXX_COMPILER_ID} MATCHES "NVHPC|PGI")
    target_compile_options(PeanoOptions INTERFACE
      -W
      -Wall
      -Wextra
      -Kieee
    )
  else()
    target_compile_options(PeanoOptions INTERFACE
      -Wall
      -Wextra
      -Wpedantic
      -Wformat=2
      -Wcast-align
      -Wcast-qual
      -Wreturn-type
      -Wdouble-promotion
      -Wnull-dereference
      -Woverloaded-virtual
      -Wnon-virtual-dtor
      -Wno-unused-parameter
      -Wunused-but-set-variable
    )
  endif()

  target_compile_options(PeanoOptions INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:${PEANO_EXTRA_CXX_FLAGS}>
  )
  target_compile_definitions(PeanoOptions INTERFACE
    ${PEANO_EXTRA_DEFINITIONS}
    MAX_SPACETREES_PER_RANK=${MAX_SPACETREES_PER_RANK}
  )
  target_link_options(PeanoOptions INTERFACE
    ${PEANO_EXTRA_LINK_FLAGS}
  )

  if(PEANO_WITH_FPE)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_FPE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
      # Intel's LLVM front-end rejects FENV_ACCESS without precise FP semantics.
      target_compile_options(PeanoOptions INTERFACE -ffp-model=precise)
    endif()
  endif()

  target_include_directories(PeanoOptions INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
endfunction()

macro(peano_resolve_gpu_backend)
  if(PEANO_WITH_GPU_ARCH MATCHES "^spir")
    set(PEANO_WITH_GPU_VENDOR "intel")
  elseif(PEANO_WITH_GPU_ARCH MATCHES "^sm_")
    set(PEANO_WITH_GPU_VENDOR "nvidia")
  elseif(PEANO_WITH_GPU_ARCH MATCHES "^gfx")
    set(PEANO_WITH_GPU_VENDOR "amd")
  endif()

  if(PEANO_WITH_GPU_BACKEND STREQUAL "none")
    set(_peano_gpu_backend "no")
  elseif(PEANO_WITH_GPU_BACKEND STREQUAL "native")
    if(PEANO_WITH_GPU_VENDOR STREQUAL "nvidia")
      set(_peano_gpu_backend "cuda")
    elseif(PEANO_WITH_GPU_VENDOR STREQUAL "amd")
      set(_peano_gpu_backend "hip")
    elseif(PEANO_WITH_GPU_VENDOR STREQUAL "intel")
      set(_peano_gpu_backend "sycl")
    else()
      message(FATAL_ERROR "PEANO_WITH_GPU_BACKEND=native requires PEANO_WITH_GPU_ARCH to be set (sm_*, gfx* or spir64).")
    endif()
  elseif(PEANO_WITH_GPU_BACKEND STREQUAL "kokkos")
    set(_peano_gpu_backend "kokkos")
  elseif(PEANO_WITH_GPU_BACKEND STREQUAL "acpp")
    set(_peano_gpu_backend "sycl")
  elseif(PEANO_WITH_GPU_BACKEND STREQUAL "ompt")
    set(_peano_gpu_backend "ompt")
  else()
    message(FATAL_ERROR "Invalid PEANO_WITH_GPU_BACKEND='${PEANO_WITH_GPU_BACKEND}'. Expected none, native, kokkos, acpp or ompt.")
  endif()

  if(NOT _peano_gpu_backend STREQUAL "no")
    if(NOT PEANO_WITH_GPU_ARCH OR PEANO_WITH_GPU_ARCH STREQUAL "none")
      message(FATAL_ERROR "PEANO_WITH_GPU_BACKEND='${PEANO_WITH_GPU_BACKEND}' requires PEANO_WITH_GPU_ARCH to be set (sm_*, gfx* or spir64).")
    endif()
  elseif(PEANO_WITH_GPU_ARCH AND NOT PEANO_WITH_GPU_ARCH STREQUAL "none")
    message(WARNING "PEANO_WITH_GPU_ARCH='${PEANO_WITH_GPU_ARCH}' is set but PEANO_WITH_GPU_BACKEND is 'none'; the GPU architecture is ignored.")
  endif()

  if(PEANO_WITH_GPU_BACKEND STREQUAL "acpp" OR PEANO_WITH_GPU_BACKEND STREQUAL "kokkos")
    unset(CMAKE_C_COMPILER_LAUNCHER)
    unset(CMAKE_CXX_COMPILER_LAUNCHER)
  endif()
  if(PEANO_WITH_GPU_BACKEND STREQUAL "kokkos")
    set(IPO_SUPPORTED FALSE)
    message(STATUS "Disabling IPO/LTO for Kokkos compatability")
  endif()
endmacro()

function(peano_configure_curvi)
  if(PEANO_WITH_CURVI)
    if(NOT PEANO_WITH_NETCDF OR NOT PEANO_WITH_EASI)
      message(WARNING "PEANO_WITH_CURVI requires NetCDF and easi (PEANO_WITH_NETCDF and PEANO_WITH_EASI), which are not both available; disabling the Curvi toolbox.")
      set(PEANO_WITH_CURVI OFF CACHE BOOL "Build with Curvi toolbox support" FORCE)
    else()
      target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_CURVI)
    endif()
  endif()
endfunction()
