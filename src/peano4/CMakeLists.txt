# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS ${HEADER_EXTENSIONS_TO_GLOB})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/Version.h
  @ONLY
)

set(SOURCES
  Init.cpp
  Loop.cpp
  UnitTests.cpp

  datamanagement/CellMarker.cpp
  datamanagement/FaceMarker.cpp
  datamanagement/VertexMarker.cpp

  grid/AutomatonState.cpp
  grid/BoundaryDataAggregation.cpp
  grid/CellType.cpp
  grid/FaceType.cpp
  grid/GridControlEvent.cpp
  grid/GridStatistics.cpp
  grid/GridTraversalEvent.cpp
  grid/GridTraversalEventGenerator.cpp
  grid/GridTraversalObserver.cpp
  grid/GridVertex.cpp
  grid/PeanoCurve.cpp
  grid/PeriodicTopology.cpp
  grid/Spacetree.cpp
  grid/SpacetreeHelper.cpp
  grid/SpacetreeState.cpp
  grid/SplitInstruction.cpp
  grid/StackContainer.cpp
  grid/StackTopology.cpp
  grid/VertexLookupTable.cpp
  grid/VertexType.cpp
  grid/tests/AutomatonStateTest.cpp
  grid/tests/BoundaryDataAggregationTest.cpp
  grid/tests/CellTypeTest.cpp
  grid/tests/FaceTypeTest.cpp
  grid/tests/GridControlEventTest.cpp
  grid/tests/GridStatisticsTest.cpp
  grid/tests/TreeIdSetTest.cpp
  grid/tests/GridTraversalEventGeneratorTest.cpp
  grid/tests/GridVertexTest.cpp
  grid/tests/PeriodicTopologyTest.cpp
  grid/tests/SpacetreeHelperTest.cpp
  grid/tests/SpacetreeTest.cpp
  grid/tests/StackContainerTest.cpp
  grid/tests/StackTopologyTest.cpp
  grid/tests/VertexLookupTableTest.cpp
  grid/tests/VertexTypeTest.cpp

  parallel/ExchangeChannels.cpp
  parallel/RunControl.cpp
  parallel/SpacetreeSet.cpp
  parallel/TreeIdMapping.cpp
  parallel/tests/ExchangeChannelsTest.cpp
  parallel/tests/PingPongTest.cpp
  parallel/tests/RunControlTest.cpp
  parallel/tests/SpacetreeSetTest.cpp
  parallel/tests/TreeIdMappingTest.cpp

  stacks/IOMode.cpp
  stacks/SendReceiveContext.cpp
  stacks/Stack.cpp
  stacks/StackWithCustomSerialisation.cpp
  stacks/StackWithSerialisation.cpp
  stacks/tests/StackTest.cpp
  stacks/tests/TestDataOnHeap.cpp
  stacks/tests/TestDataOnHeapWithContainer.cpp
  stacks/tests/TestDataOnStack.cpp

  tests/LoopTest.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/Version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/peano4
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    peano_add_target(
      TYPE STATIC_LIBRARY
      NAME Peano4${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Tarch${mode}
    )
  endforeach()
endforeach()
