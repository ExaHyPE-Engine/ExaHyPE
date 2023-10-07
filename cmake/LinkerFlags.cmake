function(set_project_linker_flags project_name)
  set(MSVC_LINKER_FLAGS
    $<$<CONFIG:Release>:/OPT:REF>
    $<$<CONFIG:Release>:/LTCG>
  )

  set(LLVM_LINKER_FLAGS
    #-flto
  )

  set(GNU_LINKER_FLAGS
    ${LLVM_LINKER_FLAGS}
  )

  set(NVHPC_LINKER_FLAGS)

  if(MSVC)
    set(LINKER_FLAGS ${MSVC_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(LINKER_FLAGS ${LLVM_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(LINKER_FLAGS ${GNU_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
    set(LINKER_FLAGS ${LLVM_LINKER_FLAGS})
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    set(LINKER_FLAGS ${NVHPC_LINKER_FLAGS})
  else()
    message(AUTHOR_WARNING "No linker flags set for '${CMAKE_CXX_COMPILER_ID}' compiler.")
  endif()

  target_link_options(${project_name} INTERFACE ${LINKER_FLAGS})
endfunction()
