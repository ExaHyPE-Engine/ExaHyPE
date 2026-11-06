# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/Version.h
  @ONLY
)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/sfc/ActiveCurve.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/sfc/ActiveCurve.h
  @ONLY
)

set(SOURCES
  Init.cpp
  RefinementRequest.cpp
  Spacetree.cpp

  communication/CommunicationManager.cpp

  grid/AutomatonState.cpp
  grid/GridVertex.cpp
  grid/LatticeIndexMap.cpp

  topology/ChunkedTasks.cpp
  topology/Tree.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/Version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/peano4
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/sfc/ActiveCurve.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/peano4/sfc
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME Peano${mode}${dim}D
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Tarch${mode}
    )
  endforeach()
endforeach()
