find_package(Doxygen)

if(DOXYGEN_FOUND)
  set(DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/documentation/Doxyfile)

  add_custom_target(documentation
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generate documentation with Doxygen"
    VERBATIM
  )
else()
  message(WARNING "Doxygen requested but executable not found")
endif()
