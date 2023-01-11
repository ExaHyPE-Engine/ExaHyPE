configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake.config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

add_subdirectory(tarch)
add_subdirectory(peano4)
#add_subdirectory(toolbox)
#add_subdirectory(exahype2)
#add_subdirectory(convert)
#add_subdirectory(swift2)
