# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

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

if(ENABLE_EXAHYPE)
  add_subdirectory(exahype2)
endif()

add_subdirectory(unittests)
