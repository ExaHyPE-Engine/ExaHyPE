# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

set(PEANO_RELEASE_DEFINITIONS -DPEANO_ASSERTIONS=0 -DPEANO_DEBUG=0 -DNDEBUG CACHE INTERNAL "" FORCE)
set(PEANO_ASSERTS_DEFINITIONS -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=0          CACHE INTERNAL "" FORCE)
set(PEANO_DEBUG_DEFINITIONS   -DPEANO_ASSERTIONS=1 -DPEANO_DEBUG=1          CACHE INTERNAL "" FORCE)

set(PEANO_C_FLAGS_RELEASE       -O3 -fomit-frame-pointer    CACHE INTERNAL "")
set(PEANO_C_FLAGS_ASSERTS       -g -O2                      CACHE INTERNAL "")
set(PEANO_C_FLAGS_DEBUG         -g -O0                      CACHE INTERNAL "")

set(PEANO_CXX_FLAGS_RELEASE     -O3 -fomit-frame-pointer    CACHE INTERNAL "")
set(PEANO_CXX_FLAGS_ASSERTS     -g -O2                      CACHE INTERNAL "")
set(PEANO_CXX_FLAGS_DEBUG       -g -O0                      CACHE INTERNAL "")

set(PEANO_Fortran_FLAGS_RELEASE -O3                         CACHE INTERNAL "")
set(PEANO_Fortran_FLAGS_ASSERTS -g -O2                      CACHE INTERNAL "")
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
    # -Wdouble-promotion
    -Werror
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
    -Werror
  )

  # CUDA routes host warnings through -Xcompiler because they do not reach nvcc or ptxas device
  # compilation. An NVHPC host requires PEANO_NVHPC_WARNINGS.
  set(PEANO_CUDA_WARNINGS "")
  foreach(_peano_warning ${PEANO_CXX_WARNINGS})
    list(APPEND PEANO_CUDA_WARNINGS "-Xcompiler=${_peano_warning}")
  endforeach()

  list(APPEND PEANO_CUDA_WARNINGS
    -Werror=all-warnings
    -Wreorder # Diagnose device member initialisation order.
    -Wdefault-stream-launch
    -Wext-lambda-captures-this
    #-Xptxas=--warn-on-spills # Diagnose device register spills.
    -Xcompiler=-Wno-overlength-strings
    -Xcompiler=-Wno-dtor-name
  )

  target_compile_options(PeanoOptions INTERFACE
    # NVHPC and PGI use a smaller compatible warning set.
    $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:${PEANO_NVHPC_WARNINGS}>
    $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:${PEANO_NVHPC_WARNINGS}>
    # Other C and C++ front-ends use the full warning set.
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<NOT:$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>>>:${PEANO_C_WARNINGS}>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>>>:${PEANO_CXX_WARNINGS}>
    # HIP accepts the warning set directly, while CUDA uses the routed set above.
    $<$<COMPILE_LANGUAGE:HIP>:${PEANO_CXX_WARNINGS}>
    $<$<COMPILE_LANGUAGE:CUDA>:${PEANO_CUDA_WARNINGS}>
    # NVHPC requires explicit IEEE-754 semantics in every supported language.
    $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:-Kieee>
    $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:-Kieee>
    $<$<COMPILE_LANG_AND_ID:Fortran,NVHPC,PGI>:-Kieee>
    # NVHPC's automatic inlining drops stores of the kernels that stage a narrower storage
    # precision through the compute precision.
    $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:-Mnoautoinline>
    $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:-Mnoautoinline>
    # IntelLLVM precise semantics preserve exact grid arithmetic and the symmetry of
    # interpolation and restriction. The second flag confines fused multiply-adds to
    # single expressions and supports FENV_ACCESS.
    $<$<COMPILE_LANG_AND_ID:C,IntelLLVM>:-ffp-model=precise>
    $<$<COMPILE_LANG_AND_ID:C,IntelLLVM>:-ffp-contract=on>
    $<$<COMPILE_LANG_AND_ID:CXX,IntelLLVM>:-ffp-model=precise>
    $<$<COMPILE_LANG_AND_ID:CXX,IntelLLVM>:-ffp-contract=on>
    # GCC fuses operations across statements by default, so two code paths of one kernel
    # round differently.
    $<$<COMPILE_LANG_AND_ID:C,GNU>:-ffp-contract=on>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-ffp-contract=on>
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
    # Frame pointers and line tables retain source locations in optimised sanitizer reports.
    set(PEANO_SANITIZER_FLAGS ${_peano_sanitize_flag} -g -fno-omit-frame-pointer)
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANGUAGE:C>:${PEANO_SANITIZER_FLAGS}>
      $<$<COMPILE_LANGUAGE:CXX>:${PEANO_SANITIZER_FLAGS}>
    )
    target_link_options(PeanoOptions INTERFACE ${_peano_sanitize_flag})
    # libstdc++ assertions detect library precondition violations outside sanitizer coverage.
    target_compile_definitions(PeanoOptions INTERFACE _GLIBCXX_ASSERTIONS)
  endif()

  if(PEANO_WITH_FPE)
    target_compile_definitions(PeanoOptions INTERFACE PEANO_WITH_FPE)
    # Frame pointers and debug information preserve fault locations in fatal reports.
    # Strict exception semantics keep a compiler from evaluating operations the source
    # does not reach, whose exceptions a trap would otherwise report.
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANG_AND_ID:C,GNU>:-ftrapping-math>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-ftrapping-math>
      $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,IntelLLVM>:-ffp-exception-behavior=strict>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang,IntelLLVM>:-ffp-exception-behavior=strict>
      $<$<COMPILE_LANG_AND_ID:C,NVHPC,PGI>:-Ktrap=fp>
      $<$<COMPILE_LANG_AND_ID:CXX,NVHPC,PGI>:-Ktrap=fp>
      $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,IntelLLVM>:-fno-omit-frame-pointer>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,IntelLLVM>:-fno-omit-frame-pointer>
      $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang,IntelLLVM>:-g>
      $<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang,AppleClang,IntelLLVM>:-g>
    )
  endif()

  target_compile_options(PeanoOptions INTERFACE
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-free-nonheap-object>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-array-bounds>
    $<$<COMPILE_LANG_AND_ID:CXX,GNU>:-Wno-null-dereference>
  )

  if(PEANO_WITH_GPU_ARCH MATCHES "^sm_")
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,IntelLLVM>:-Wno-unknown-cuda-version>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang,IntelLLVM>:-Wno-unknown-cuda-version>
    )
  endif()

  if(NOT PEANO_WITH_SANITIZER STREQUAL "none" OR PEANO_WITH_FPE)
    target_compile_options(PeanoOptions INTERFACE
      $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,IntelLLVM>:-Wno-pass-failed>
      $<$<COMPILE_LANG_AND_ID:CXX,Clang,AppleClang,IntelLLVM>:-Wno-pass-failed>
    )
  endif()
endfunction()
