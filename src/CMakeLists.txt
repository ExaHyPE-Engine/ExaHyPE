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
add_subdirectory(exahype2)
add_subdirectory(unittests)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    add_library(PeanoRuntime${mode}${dim}d INTERFACE)
    target_link_libraries(PeanoRuntime${mode}${dim}d INTERFACE
      ExaHyPE2${mode}${dim}d
    )
    if(PEANO_WITH_CURVI)
      target_link_libraries(PeanoRuntime${mode}${dim}d INTERFACE
        ToolboxCurvi${mode}
      )
    endif()
    install(TARGETS PeanoRuntime${mode}${dim}d
      EXPORT ${PROJECT_NAME}Targets
    )
  endforeach()
endforeach()
