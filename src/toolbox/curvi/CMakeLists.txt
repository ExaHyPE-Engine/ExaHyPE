# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  Coordinate.cpp
  Index.cpp

  geometry/Block.cpp
  geometry/BlockBoundary.cpp
  geometry/Edge.cpp
  geometry/Face.cpp
  geometry/FaceBoundary.cpp

  input/YAMLInput.cpp

  interface/Interface.cpp

  kdTree/InnerNode.cpp
  kdTree/Leaf.cpp
  kdTree/Root.cpp

  netcdf/NetCDFWriter.cpp

  transformation/Transformation.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
  INSTALL_SUBDIR "toolbox"
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  add_peano_library(
    NAME ToolboxCurvi${mode}
    MODE ${mode}
    SOURCES ${SOURCES}
    HEADERS ${HEADERS}
    LINK_LIBRARIES Tarch${mode}
  )
endforeach()
