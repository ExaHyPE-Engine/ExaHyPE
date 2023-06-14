function(set_project_compiler_definitions project_name)
  set(MSVC_COMPILER_DEFINITIONS
      WIN32_LEAN_AND_MEAN
      _UNICODE
      UNICODE
  )

  set(CLANG_COMPILER_DEFINITIONS)

  set(GCC_COMPILER_DEFINITIONS)

  if(MSVC)
    set(COMPILER_DEFINITIONS ${MSVC_COMPILER_DEFINITIONS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    list(APPEND CLANG_COMPILER_DEFINITIONS -ftime-trace)
    set(COMPILER_DEFINITIONS ${CLANG_COMPILER_DEFINITIONS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(COMPILER_DEFINITIONS ${GCC_COMPILER_DEFINITIONS})
  else()
    message(AUTHOR_WARNING "No compiler definitions set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  target_compile_definitions(${project_name} INTERFACE ${COMPILER_DEFINITIONS})
endfunction()
