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

  checkpoint/CellDataLoader.cpp
  checkpoint/CellDataWriter.cpp
  checkpoint/CheckpointFile.cpp
  checkpoint/GridCheckpoint.cpp
  checkpoint/Lattice.cpp
  checkpoint/ParticleDataWriter.cpp
  checkpoint/tests/CellDataLoaderTest.cpp
  checkpoint/tests/CellDataWriterTest.cpp
  checkpoint/tests/CheckpointFileTest.cpp
  checkpoint/tests/GridCheckpointTest.cpp
  checkpoint/tests/LatticeTest.cpp
  checkpoint/tests/ParticleDataWriterTest.cpp

  communication/CommunicationManager.cpp
  communication/tests/CommunicationManagerTest.cpp

  data/ContainerRegistry.cpp
  data/RegisteredStackContainer.cpp
  data/tests/ContainerRegistryTest.cpp
  data/tests/RegisteredStackContainerTest.cpp

  datamanagement/CellMarker.cpp
  datamanagement/FaceMarker.cpp
  datamanagement/VertexMarker.cpp

  grid/AutomatonState.cpp
  grid/BoundaryDataAggregation.cpp
  grid/GridControlEvent.cpp
  grid/GridStatistics.cpp
  grid/GridTraversalEvent.cpp
  grid/GridTraversalObserver.cpp
  grid/GridVertex.cpp
  grid/LatticeIndexMap.cpp
  grid/PeanoCurve.cpp
  grid/StackContainer.cpp
  grid/StackTopology.cpp
  grid/tests/AutomatonStateTest.cpp
  grid/tests/BoundaryDataAggregationTest.cpp
  grid/tests/GridControlEventTest.cpp
  grid/tests/GridStatisticsTest.cpp
  grid/tests/GridTraversalEventTest.cpp
  grid/tests/GridVertexTest.cpp
  grid/tests/LatticeIndexMapTest.cpp
  grid/tests/StackTopologyTest.cpp

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

  topology/ChunkedTasks.cpp
  topology/TopologyManager.cpp
  topology/Tree.cpp
  topology/TreeTraversal.cpp
  topology/tests/ChunkedTasksTest.cpp
  topology/tests/TopologyManagerTest.cpp
  topology/tests/TreeTest.cpp
  topology/tests/TreeTraversalTest.cpp
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
