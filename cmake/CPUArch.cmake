# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

set(IS_GCC   FALSE)
set(IS_CLANG FALSE)
set(IS_INTEL FALSE)
set(IS_NVHPC FALSE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(IS_GCC TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "IntelLLVM|Intel")
  set(IS_INTEL TRUE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "NVHPC|PGI")
  set(IS_NVHPC TRUE)
endif()

set(WITH_CPU_ARCH "native" CACHE STRING "Instruction set architecture (e.g. native, skylake, zen3, armv8-a)")
set(WITH_CPU_TUNE "native" CACHE STRING "Tuning architecture (e.g. native, skylake, znver3)")

set(_DETECTED_FLAGS "")

if(IS_GCC OR IS_CLANG OR IS_INTEL)
  if(WITH_CPU_ARCH)
    check_cxx_compiler_flag("-march=${WITH_CPU_ARCH}" HAS_MARCH)
    if(HAS_MARCH)
      list(APPEND _DETECTED_FLAGS "-march=${WITH_CPU_ARCH}")
    endif()
  endif()

  if(WITH_CPU_TUNE)
    check_cxx_compiler_flag("-mtune=${WITH_CPU_TUNE}" HAS_MTUNE)
    if(HAS_MTUNE)
      list(APPEND _DETECTED_FLAGS "-mtune=${WITH_CPU_TUNE}")
    endif()
  endif()
elseif(IS_NVHPC)
  if(WITH_CPU_ARCH)
    check_cxx_compiler_flag("-tp=${WITH_CPU_ARCH}" HAS_TP_FLAG)
    if(HAS_TP_FLAG)
      list(APPEND _DETECTED_FLAGS "-tp=${WITH_CPU_ARCH}")
    endif()
  endif()
endif()

function(_check_macro_defined macro flags out_var)
  set(code
    "#ifndef ${macro}
     #error ${macro} not defined
     #endif
     int main(){return 0;}"
  )
  set(_OLD_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
  set(CMAKE_REQUIRED_FLAGS "${flags}")
  check_cxx_source_compiles("${code}" ${out_var})
  set(CMAKE_REQUIRED_FLAGS "${_OLD_REQUIRED_FLAGS}")
endfunction()

_check_macro_defined("__AVX512F__"          "${_DETECTED_FLAGS}" HAS_AVX512F)
_check_macro_defined("__AVX2__"             "${_DETECTED_FLAGS}" HAS_AVX2)
_check_macro_defined("__SSE2__"             "${_DETECTED_FLAGS}" HAS_SSE2)
_check_macro_defined("__ARM_NEON"           "${_DETECTED_FLAGS}" HAS_NEON)
_check_macro_defined("__ARM_FEATURE_SVE"    "${_DETECTED_FLAGS}" HAS_SVE)

set(WITH_SIMDLEN 2) # double lanes for 128-bit SIMD
set(WITH_ALIGNMENT 16)

if(HAS_AVX512F)
  # 512-bit vectors -> 8 doubles
  set(WITH_SIMDLEN 8)
  set(WITH_ALIGNMENT 64)
elseif(HAS_AVX2)
  # 256-bit vectors -> 4 doubles
  set(WITH_SIMDLEN 4)
  set(WITH_ALIGNMENT 32)
elseif(HAS_SVE)
  set(WITH_SIMDLEN 2)
  set(WITH_ALIGNMENT 64)
elseif(HAS_NEON)
  # AArch64 NEON: 128-bit -> 2 doubles
  set(WITH_SIMDLEN 2)
  set(WITH_ALIGNMENT 16)
elseif(HAS_SSE2)
  # 128-bit -> 2 doubles
  set(WITH_SIMDLEN 2)
  set(WITH_ALIGNMENT 16)
endif()

set(WITH_SIMDLEN   ${WITH_SIMDLEN} CACHE INTERNAL "Number of double-precision elements per SIMD vector (e.g., 4 for AVX2)")
set(WITH_ALIGNMENT ${WITH_ALIGNMENT} CACHE INTERNAL "Memory alignment in bytes required for vectorized loads/stores (e.g., 32 for AVX2)")
set(CPU_ARCH_FLAGS ${_DETECTED_FLAGS} CACHE INTERNAL "Detected CPU architecture compiler flags")

message(STATUS "Using CPU architecture flags: ${CPU_ARCH_FLAGS}")
message(STATUS "Using CPU features: ALIGNMENT=${WITH_ALIGNMENT}, SIMDLEN=${WITH_SIMDLEN}")

target_compile_definitions(PeanoOptions INTERFACE
  SIMDLEN=${WITH_SIMDLEN}
  ALIGNMENT=${WITH_ALIGNMENT}
)

target_compile_options(PeanoOptions INTERFACE
  ${CPU_ARCH_FLAGS}
  # -ffp-contract=off
)
