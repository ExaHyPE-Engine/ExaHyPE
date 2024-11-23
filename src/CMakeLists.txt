# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/config.h")

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  DESTINATION "${CMAKE_INSTALL_PREFIX}/include"
)

set(SOURCE_EXTENSIONS_TO_GLOB
  "*.c"
  "*.cxx"
  "*.c++"
  "*.cpp"
  "*.cu"
)
set(HEADER_EXTENSIONS_TO_GLOB
  "*.h"
  "*.hxx"
  "*.h++"
  "*.hpp"
  "*.cpph"
  "*.cuh"
  "*.inc"
  "*.inl"
)

add_subdirectory(tarch)
add_subdirectory(peano4)
add_subdirectory(toolbox)

if(WITH_PETSC)
  add_subdirectory(petsc)
endif()

if(ENABLE_EXAHYPE)
  add_subdirectory(exahype2)
endif()

if(ENABLE_MGHYPE)
  add_subdirectory(mghype)
endif()

if(ENABLE_SWIFT)
  add_subdirectory(swift2)
endif()

add_subdirectory(unittests)
