configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake.config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

set(FILES_TO_GLOB "*.c" "*.cpp" "*.h" "*.hpp" "*.cuh" "*.cu" "*.cpph" "*.cxx")

add_subdirectory(tarch)
add_subdirectory(peano4)
add_subdirectory(convert)
add_subdirectory(toolbox)

if(ENABLE_EXAHYPE)
  add_subdirectory(exahype2)
endif()

if(ENABLE_SWIFT)
  add_subdirectory(swift2)
endif()

if(WITH_PETSC)
  add_subdirectory(petsc)
endif()

add_subdirectory(unittests)
