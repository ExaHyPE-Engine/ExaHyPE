function(set_project_linker_flags project_name)
  set(MSVC_LINKER_FLAGS
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/LTCG>
  )

  set(CLANG_LINKER_FLAGS
    -flto
  )

  set(GCC_LINKER_FLAGS
      ${CLANG_LINKER_FLAGS}
  )

  if(MSVC)
    set(LINKER_FLAGS ${MSVC_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(LINKER_FLAGS ${CLANG_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(LINKER_FLAGS ${GCC_LINKER_FLAGS})
  else()
    message(AUTHOR_WARNING "No linker flags set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  target_link_options(${project_name} INTERFACE ${LINKER_FLAGS})
endfunction()
