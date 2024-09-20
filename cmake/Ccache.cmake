option(USE_CCACHE "Use Ccache" ON)

if(USE_CCACHE)
  find_program(CCACHE ccache)

  if(CCACHE)
    # Set up wrapper scripts
    set(C_LAUNCHER   "${CCACHE}")
    set(CXX_LAUNCHER "${CCACHE}")
    configure_file(cmake/launch-c.in launch-c)
    configure_file(cmake/launch-cxx.in launch-cxx)
    #execute_process(COMMAND chmod a+rx
    #  "${CMAKE_BINARY_DIR}/launch-c"
    #  "${CMAKE_BINARY_DIR}/launch-cxx"
    #)
    file(CHMOD "${CMAKE_BINARY_DIR}/launch-c" PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE)
    file(CHMOD "${CMAKE_BINARY_DIR}/launch-cxx" PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE)

    if(CMAKE_GENERATOR STREQUAL "Xcode")
      # Set Xcode project attributes to route compilation and linking through our scripts
      set(CMAKE_XCODE_ATTRIBUTE_CC         "${CMAKE_BINARY_DIR}/launch-c")
      set(CMAKE_XCODE_ATTRIBUTE_CXX        "${CMAKE_BINARY_DIR}/launch-cxx")
      set(CMAKE_XCODE_ATTRIBUTE_LD         "${CMAKE_BINARY_DIR}/launch-c")
      set(CMAKE_XCODE_ATTRIBUTE_LDPLUSPLUS "${CMAKE_BINARY_DIR}/launch-cxx")
    else()
      # Support Unix Makefiles and Ninja
      set(CMAKE_C_COMPILER_LAUNCHER   "${CMAKE_BINARY_DIR}/launch-c")
      set(CMAKE_CXX_COMPILER_LAUNCHER "${CMAKE_BINARY_DIR}/launch-cxx")
    endif()

    message(STATUS "Found Ccache: ${CCACHE}")
  else()
    message(WARNING "Ccache requested but executable not found")
  endif()
endif()
