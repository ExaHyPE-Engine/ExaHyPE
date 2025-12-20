# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${HEADER_EXTENSIONS_TO_GLOB})

set(SOURCES
  peano4.cpp
  UnitTests.cpp

  datamanagement/CellMarker.cpp
  datamanagement/FaceMarker.cpp
  datamanagement/VertexMarker.cpp

  grid/AutomatonState.cpp
  grid/CellType.cpp
  grid/FaceType.cpp
  grid/GridControlEvent.cpp
  grid/GridStatistics.cpp
  grid/GridTraversalEvent.cpp
  grid/GridTraversalEventGenerator.cpp
  grid/GridTraversalObserver.cpp
  grid/GridVertex.cpp
  grid/LoadStoreComputeFlag.cpp
  grid/PeanoCurve.cpp
  grid/Spacetree.cpp
  grid/SpacetreeHelper.cpp
  grid/SpacetreeState.cpp
  grid/SplitInstruction.cpp
  grid/VertexLookupTable.cpp
  grid/VertexType.cpp
  grid/tests/AutomatonStateTest.cpp
  grid/tests/GridControlEventTest.cpp
  grid/tests/GridStatisticsTest.cpp
  grid/tests/GridTraversalEventGeneratorTest.cpp
  grid/tests/GridVertexTest.cpp
  grid/tests/SpacetreeTest.cpp

  parallel/parallel.cpp
  parallel/Node.cpp
  parallel/SpacetreeSet.cpp
  parallel/StartTraversalMessage.cpp
  parallel/TreeEntry.cpp
  parallel/TreeManagementMessage.cpp
  parallel/tests/NodeTest.cpp
  parallel/tests/PingPongTest.cpp

  stacks/IOMode.cpp
  stacks/SendReceiveContext.cpp
  stacks/Stack.cpp
  stacks/StackWithCustomSerialisation.cpp
  stacks/StackWithSerialisation.cpp
  stacks/TreeToStacks.cpp
  stacks/tests/StackTest.cpp
  stacks/tests/TestDataOnHeap.cpp
  stacks/tests/TestDataOnHeapWithContainer.cpp
  stacks/tests/TestDataOnStack.cpp
  stacks/tests/TreeToStacksTest.cpp

  utils/Linearised.cpp
  utils/Loop.cpp
  utils/tests/LinearisedTest.cpp
)

install_peano_headers(
  PATTERNS ${HEADER_EXTENSIONS_TO_GLOB}
)

foreach(mode IN LISTS SUPPORTED_BUILD_TYPES)
  foreach(dim IN LISTS SUPPORTED_DIMENSIONS)
    add_peano_library(
      NAME Peano4${mode}${dim}d
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Tarch${mode}
    )
  endforeach()
endforeach()
