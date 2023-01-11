configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake.config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

add_subdirectory(tarch)
add_subdirectory(peano4)
add_subdirectory(toolbox)
add_subdirectory(convert)
if(ENABLE_EXAHYPE)
  add_subdirectory(exahype2)
endif()
if(ENABLE_SWIFT)
  add_subdirectory(swift2)
endif()
