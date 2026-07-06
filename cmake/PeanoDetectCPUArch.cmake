# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(PEANO_WITH_CPU_ARCH "native" CACHE STRING "Instruction set architecture (e.g. native, skylake, zen3, armv8-a)")
set(PEANO_WITH_CPU_TUNE "native" CACHE STRING "Tuning architecture (e.g. native, skylake, znver3)")

# Helper: does the compiler predefine <macro> under <flags>? Result -> <out_var>
# (cached by check_cxx_source_compiles()).
function(_peano_check_macro_defined macro flags out_var)
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

function(peano_detect_cpu_architecture)
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

  set(_DETECTED_FLAGS "")
  if(IS_GCC OR IS_CLANG OR IS_INTEL)
    if(PEANO_WITH_CPU_ARCH)
      check_cxx_compiler_flag("-march=${PEANO_WITH_CPU_ARCH}" HAS_MARCH)
      if(HAS_MARCH)
        list(APPEND _DETECTED_FLAGS "-march=${PEANO_WITH_CPU_ARCH}")
      endif()
    endif()
    if(PEANO_WITH_CPU_TUNE)
      check_cxx_compiler_flag("-mtune=${PEANO_WITH_CPU_TUNE}" HAS_MTUNE)
      if(HAS_MTUNE)
        list(APPEND _DETECTED_FLAGS "-mtune=${PEANO_WITH_CPU_TUNE}")
      endif()
    endif()
  elseif(IS_NVHPC)
    if(PEANO_WITH_CPU_ARCH)
      check_cxx_compiler_flag("-tp=${PEANO_WITH_CPU_ARCH}" HAS_TP_FLAG)
      if(HAS_TP_FLAG)
        list(APPEND _DETECTED_FLAGS "-tp=${PEANO_WITH_CPU_ARCH}")
      endif()
    endif()
  endif()

  _peano_check_macro_defined("__AVX512F__"       "${_DETECTED_FLAGS}" HAS_AVX512F)
  _peano_check_macro_defined("__AVX2__"          "${_DETECTED_FLAGS}" HAS_AVX2)
  _peano_check_macro_defined("__SSE2__"          "${_DETECTED_FLAGS}" HAS_SSE2)
  _peano_check_macro_defined("__ARM_NEON"        "${_DETECTED_FLAGS}" HAS_NEON)
  _peano_check_macro_defined("__ARM_FEATURE_SVE" "${_DETECTED_FLAGS}" HAS_SVE)

  set(_simdlen 2)    # double lanes for 128-bit SIMD
  set(_alignment 16)
  if(HAS_AVX512F)
    # 512-bit vectors -> 8 doubles
    set(_simdlen 8)
    set(_alignment 64)
  elseif(HAS_AVX2)
    # 256-bit vectors -> 4 doubles
    set(_simdlen 4)
    set(_alignment 32)
  elseif(HAS_SVE)
    set(_simdlen 2)
    set(_alignment 64)
  elseif(HAS_NEON)
    # AArch64 NEON: 128-bit -> 2 doubles
    set(_simdlen 2)
    set(_alignment 16)
  elseif(HAS_SSE2)
    # 128-bit -> 2 doubles
    set(_simdlen 2)
    set(_alignment 16)
  endif()

  set(PEANO_WITH_SIMDLEN   ${_simdlen}   CACHE INTERNAL "Number of double-precision elements per SIMD vector (e.g., 4 for AVX2)")
  set(PEANO_WITH_ALIGNMENT ${_alignment} CACHE INTERNAL "Memory alignment in bytes required for vectorized loads/stores (e.g., 32 for AVX2)")
  set(CPU_ARCH_FLAGS ${_DETECTED_FLAGS}  CACHE INTERNAL "Detected CPU architecture compiler flags")

  message(STATUS "Detected CPU architecture flags: ${CPU_ARCH_FLAGS}")
  message(STATUS "Detected CPU features: ALIGNMENT=${PEANO_WITH_ALIGNMENT}, SIMDLEN=${PEANO_WITH_SIMDLEN}")

  target_compile_definitions(PeanoOptions INTERFACE
    PEANO_WITH_SIMDLEN=${PEANO_WITH_SIMDLEN}
    PEANO_WITH_ALIGNMENT=${PEANO_WITH_ALIGNMENT}
  )
  # CPU_ARCH_FLAGS (e.g. -march=native -mtune=native, or -tp=native for NVHPC) are host-compiler
  # flags, and they are only validated against the C++ compiler (check_cxx_compiler_flag above).
  # Apply them to C, C++ and HIP directly, and forward them to nvcc via -Xcompiler. Fortran is
  # excluded.
  set(_peano_cuda_arch_flags "")
  foreach(_peano_flag ${CPU_ARCH_FLAGS})
    list(APPEND _peano_cuda_arch_flags "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=${_peano_flag}>")
  endforeach()
  target_compile_options(PeanoOptions INTERFACE
    $<$<AND:$<NOT:$<COMPILE_LANGUAGE:CUDA>>,$<NOT:$<COMPILE_LANGUAGE:Fortran>>>:${CPU_ARCH_FLAGS}>
    ${_peano_cuda_arch_flags}
  )
endfunction()
