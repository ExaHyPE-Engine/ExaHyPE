# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

include_guard(GLOBAL)

function(peano_add_clang_tidy_targets)
  if(TARGET ClangTidy OR TARGET ClangTidyFix)
    message(FATAL_ERROR "peano_add_clang_tidy_targets() target names ClangTidy and ClangTidyFix must be unused")
  endif()

  find_program(PEANO_RUN_CLANG_TIDY NAMES run-clang-tidy run-clang-tidy.py)
  find_program(PEANO_CLANG_TIDY NAMES clang-tidy)
  find_program(PEANO_CLANG_APPLY_REPLACEMENTS NAMES clang-apply-replacements)

  if(DEFINED AVAILABLE_PROCESSOR_COUNT AND AVAILABLE_PROCESSOR_COUNT GREATER 0)
    set(_peano_clang_tidy_default_jobs "${AVAILABLE_PROCESSOR_COUNT}")
  else()
    set(_peano_clang_tidy_default_jobs 1)
  endif()

  set(PEANO_CLANG_TIDY_JOBS
    "${_peano_clang_tidy_default_jobs}"
    CACHE STRING "Number of parallel clang-tidy jobs"
  )
  set(PEANO_CLANG_TIDY_SOURCE_FILTER
    "^${CMAKE_SOURCE_DIR}/src/.*"
    CACHE STRING "run-clang-tidy source filter regular expression"
  )

  if(NOT PEANO_RUN_CLANG_TIDY)
    message(WARNING "run-clang-tidy not found; clang-tidy targets will not be available")
    return()
  endif()

  if(NOT PEANO_CLANG_TIDY)
    message(WARNING "clang-tidy not found; clang-tidy targets will not be available")
    return()
  endif()

  set(_peano_clang_tidy_common_args
    -p "${CMAKE_BINARY_DIR}"
    -quiet
    -clang-tidy-binary "${PEANO_CLANG_TIDY}"
    -j "${PEANO_CLANG_TIDY_JOBS}"
  )

  if(PEANO_CLANG_TIDY_SOURCE_FILTER)
    list(APPEND _peano_clang_tidy_common_args
      -source-filter "${PEANO_CLANG_TIDY_SOURCE_FILTER}"
    )
  endif()

  get_property(_peano_clang_tidy_sources GLOBAL PROPERTY PEANO_CLANG_TIDY_SOURCES)
  if(_peano_clang_tidy_sources)
    list(REMOVE_DUPLICATES _peano_clang_tidy_sources)
    list(SORT _peano_clang_tidy_sources)
    list(APPEND _peano_clang_tidy_common_args ${_peano_clang_tidy_sources})
  endif()

  add_custom_target(ClangTidy
    COMMAND
      "${PEANO_RUN_CLANG_TIDY}"
      ${_peano_clang_tidy_common_args}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM
    COMMENT "Running clang-tidy diagnostics"
  )

  set(_peano_clang_tidy_fix_args ${_peano_clang_tidy_common_args} -fix -format)
  if(PEANO_CLANG_APPLY_REPLACEMENTS)
    list(APPEND _peano_clang_tidy_fix_args
      -clang-apply-replacements-binary "${PEANO_CLANG_APPLY_REPLACEMENTS}"
    )
  endif()

  add_custom_target(ClangTidyFix
    COMMAND
      "${PEANO_RUN_CLANG_TIDY}"
      ${_peano_clang_tidy_fix_args}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    USES_TERMINAL
    VERBATIM
    COMMENT "Running clang-tidy and applying fixes"
  )
endfunction()
