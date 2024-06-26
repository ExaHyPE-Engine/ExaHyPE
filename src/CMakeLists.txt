configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake.config.h.in
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h
)

set(SOURCE_EXTENSIONS_TO_GLOB "*.c" "*.cxx" "*.c++" "*.cpp" "*.cu" "*.csnippet")
set(HEADER_EXTENSIONS_TO_GLOB "*.h" "*.hxx" "*.h++" "*.hpp" "*.cuh" "*.cpph" "*.inc" "*.inl" "*.hsnippet")

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
