# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(PEANO_RELEASE_DEFINITIONS -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=0 -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "" FORCE)
set(PEANO_ASSERTS_DEFINITIONS -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=0          CACHE INTERNAL "" FORCE)
set(PEANO_TRACE_DEFINITIONS   -DPEANO_TRACE=1 -DPEANO_ASSERTIONS=0 -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "" FORCE)
set(PEANO_DEBUG_DEFINITIONS   -DPEANO_TRACE=0 -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=1          CACHE INTERNAL "" FORCE)

set(PEANO_C_FLAGS_RELEASE       -O3 -fomit-frame-pointer    CACHE INTERNAL "")
set(PEANO_C_FLAGS_ASSERTS       -g -O3                      CACHE INTERNAL "")
set(PEANO_C_FLAGS_TRACE         -g -O3 -fomit-frame-pointer CACHE INTERNAL "")
set(PEANO_C_FLAGS_DEBUG         -g -O0                      CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_RELEASE     -O3 -fomit-frame-pointer    CACHE INTERNAL "")
set(PEANO_CXX_FLAGS_ASSERTS     -g -O3                      CACHE INTERNAL "")
set(PEANO_CXX_FLAGS_TRACE       -g -O3 -fomit-frame-pointer CACHE INTERNAL "")
set(PEANO_CXX_FLAGS_DEBUG       -g -O0                      CACHE INTERNAL "")

set(PEANO_Fortran_FLAGS_RELEASE -O3                         CACHE INTERNAL "")
set(PEANO_Fortran_FLAGS_ASSERTS -g -O3                      CACHE INTERNAL "")
set(PEANO_Fortran_FLAGS_TRACE   -g -O3                      CACHE INTERNAL "")
set(PEANO_Fortran_FLAGS_DEBUG   -g -O0                      CACHE INTERNAL "")

set(PEANO_EXTRA_C_FLAGS       "" CACHE STRING "Extra C flags (semicolon-separated)")
set(PEANO_EXTRA_CXX_FLAGS     "" CACHE STRING "Extra C++ flags (semicolon-separated)")
set(PEANO_EXTRA_Fortran_FLAGS "" CACHE STRING "Extra Fortran flags (semicolon-separated)")
set(PEANO_EXTRA_CUDA_FLAGS    "" CACHE STRING "Extra CUDA flags (semicolon-separated)")
set(PEANO_EXTRA_HIP_FLAGS     "" CACHE STRING "Extra HIP flags (semicolon-separated)")
set(PEANO_EXTRA_DEFINITIONS   "" CACHE STRING "Extra preprocessor definitions (semicolon-separated)")
set(PEANO_EXTRA_LINK_FLAGS    "" CACHE STRING "Extra linker flags (semicolon-separated)")

function(peano_apply_flags)
  set(PEANO_C_WARNINGS
    -W
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wreturn-type
    -Wnull-dereference
    -Wno-unused-parameter
    -Wunused-but-set-variable
    -Wformat=2
    -Wcast-qual
    -Wdouble-promotion
  )
  set(PEANO_CXX_WARNINGS
    ${PEANO_C_WARNINGS}
    -Woverloaded-virtual
    -Wnon-virtual-dtor
  )
  set(PEANO_NVHPC_WARNINGS
    -W
    -Wall
    -Wextra
  )

  # nvcc is split: -Xcompiler forwards the host warnings to its host compiler (assumed
  # GCC/Clang; an NVHPC host would want PEANO_NVHPC_WARNINGS), but that reaches only the host
  # half of a .cu. The device (kernel) code is compiled by nvcc's own front-end and ptxas,
  # which -Xcompiler does not touch, so it needs nvcc's native device warnings and ptxas
  # spill warnings.
  set(PEANO_CUDA_WARNINGS "")
  foreach(_peano_warning ${PEANO_CXX_WARNINGS})
    list(APPEND PEANO_CUDA_WARNINGS "-Xcompiler=${_peano_warning}")
  endforeach()

  list(APPEND PEANO_CUDA_WARNINGS
    -Wreorder                          # nvcc-native device diagnostics
    -Wdefault-stream-launch
    -Wext-lambda-captures-this
    -Xptxas=--warn-on-spills           # ptxas: warn on register spills
  )

  target_compile_options(PeanoOptions INTERFACE
    # NVHPC / PGI front-ends.
    $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:${PEANO_NVHPC_WARNINGS}>
    $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:${PEANO_NVHPC_WARNINGS}>
    # Every other (GCC / Clang / Intel) C or C++ front-end.
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<NOT:$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>>>:${PEANO_C_WARNINGS}>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>>>:${PEANO_CXX_WARNINGS}>
    # GPU host+device code: HIP (clang) directly, CUDA via nvcc's -Xcompiler + native diagnostics.
    $<$<COMPILE_LANGUAGE:HIP>:${PEANO_CXX_WARNINGS}>
    $<$<COMPILE_LANGUAGE:CUDA>:${PEANO_CUDA_WARNINGS}>
    # NVHPC needs IEEE-754 arithmetic; applied to whichever language it compiles, Fortran included.
    $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:-Kieee>
    $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:-Kieee>
    $<$<COMPILE_LANG_AND_ID:Fortran,NVHPC,PGI>:-Kieee>
    # Intel's LLVM front-end defaults to value-unsafe fast maths, which breaks the exact
    # grid-event arithmetic and the bitwise symmetry of the interpolation and restriction
    # kernels; precise restores value-safe IEEE semantics, and the explicit contraction
    # flag pins fused multiply-adds to single statements whatever the model maps it to.
    # FENV_ACCESS also requires precise semantics.
    $<$<COMPILE_LANG_AND_ID:C,IntelLLVM>:-ffp-model=precise>
    $<$<COMPILE_LANG_AND_ID:C,IntelLLVM>:-ffp-contract=on>
    $<$<COMPILE_LANG_AND_ID:CXX,IntelLLVM>:-ffp-model=precise>
    $<$<COMPILE_LANG_AND_ID:CXX,IntelLLVM>:-ffp-contract=on>
  )

  target_compile_options(PeanoOptions INTERFACE
    $<$<COMPILE_LANGUAGE:C>:${PEANO_EXTRA_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${PEANO_EXTRA_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:Fortran>:${PEANO_EXTRA_Fortran_FLAGS}>
    $<$<COMPILE_LANGUAGE:CUDA>:${PEANO_EXTRA_CUDA_FLAGS}>
    $<$<COMPILE_LANGUAGE:HIP>:${PEANO_EXTRA_HIP_FLAGS}>
  )
  target_compile_definitions(PeanoOptions INTERFACE
    ${PEANO_EXTRA_DEFINITIONS}
  )
  target_link_options(PeanoOptions INTERFACE
    ${PEANO_EXTRA_LINK_FLAGS}
  )

  if(NOT PEANO_WITH_SANITIZER STREQUAL "none")
    if(PEANO_WITH_SANITIZER STREQUAL "address")
      set(_peano_sanitize_flag -fsanitize=address)
    elseif(PEANO_WITH_SANITIZER STREQUAL "undefined")
      set(_peano_sanitize_flag -fsanitize=undefined)
    elseif(PEANO_WITH_SANITIZER STREQUAL "address+undefined")
      set(_peano_sanitize_flag -fsanitize=address,undefined)
    elseif(PEANO_WITH_SANITIZER STREQUAL "thread")
      set(_peano_sanitize_flag -fsanitize=thread)
    else()
      message(FATAL_ERROR "Invalid PEANO_WITH_SANITIZER='${PEANO_WITH_SANITIZER}'. Expected none, address, undefined, address+undefined or thread.")
    endif()
    # Frame pointers and line tables keep the sanitizer stack traces readable in
    # the optimised build types, which otherwise drop both.
    set(PEANO_SANITIZER_FLAGS ${_peano_sanitize_flag} -g -fno-omit-frame-pointer)
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANGUAGE:C>:${PEANO_SANITIZER_FLAGS}>
      $<$<COMPILE_LANGUAGE:CXX>:${PEANO_SANITIZER_FLAGS}>
    )
    target_link_options(PeanoOptions INTERFACE ${_peano_sanitize_flag})
    # libstdc++ bounds and precondition checks catch container misuse the
    # sanitizer cannot see.
    target_compile_definitions(PeanoOptions INTERFACE _GLIBCXX_ASSERTIONS)
  endif()

  if(PEANO_WITH_FPE)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_FPE)
  endif()
endfunction()
