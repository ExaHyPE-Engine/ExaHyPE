# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
include(CheckCXXSourceCompiles)

# Helper: check if a macro is defined after current flags (so we detect *effective* ISA)
function(_check_macro_defined macro out_var)
  set(code
    "#ifndef ${macro}
     #error ${macro} not defined
     #endif
     int main(){return 0;}")
  check_cxx_source_compiles("${code}" ${out_var})
endfunction()

# Probe common ISAs (order matters: test highest first)
_check_macro_defined("__AVX512F__"   HAS_AVX512F)
_check_macro_defined("__AVX2__"      HAS_AVX2)
_check_macro_defined("__SSE2__"      HAS_SSE2)
_check_macro_defined("__ARM_NEON"    HAS_NEON)
_check_macro_defined("__ARM_FEATURE_SVE" HAS_SVE) # GCC/Clang AArch64 SVE

set(ALIGNMENT 16)
set(SIMDLEN 2) # double lanes for 128-bit SIMD

if(HAS_AVX512F)
  # 512-bit vectors -> 8 doubles
  set(ALIGNMENT 64)
  set(SIMDLEN 8)
elseif(HAS_AVX2)
  # 256-bit vectors -> 4 doubles
  set(ALIGNMENT 32)
  set(SIMDLEN 4)
elseif(HAS_SVE)
  set(ALIGNMENT 64)
  set(SIMDLEN 2)
elseif(HAS_NEON)
  # AArch64 NEON: 128-bit -> 2 doubles
  set(ALIGNMENT 16)
  set(SIMDLEN 2)
elseif(HAS_SSE2)
  # 128-bit -> 2 doubles
  set(ALIGNMENT 16)
  set(SIMDLEN 2)
endif()

# Expose as cache variables so users can override in cmake -D...
set(ALIGNMENT ${ALIGNMENT} CACHE STRING "Byte alignment hint for SIMD loads/stores")
set(SIMDLEN ${SIMDLEN} CACHE STRING "OpenMP SIMD length")
message(STATUS "Using ALIGNMENT=${ALIGNMENT}, SIMDLEN=${SIMDLEN}")

target_compile_definitions(PeanoOptions INTERFACE ALIGNMENT=${ALIGNMENT} SIMDLEN=${SIMDLEN})
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-march=native)
  #target_compile_options(PeanoOptions INTERFACE -march=native)
elseif(MSVC)
  add_compile_options(/arch:AVX2)
  #target_compile_options(PeanoOptions INTERFACE /arch:AVX2)
endif()
