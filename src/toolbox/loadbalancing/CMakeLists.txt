# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Blacklist.cpp
  Configuration.cpp
  LoadBalancer.cpp
  State.cpp
  Statistics.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
  INSTALL_SUBDIR "toolbox"
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    add_peano_library(
      NAME ToolboxLoadBalancing${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Peano4${mode}${dim}d
    )
  endforeach()
endforeach()
