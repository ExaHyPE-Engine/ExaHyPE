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
  UnitTests.cpp

  communication/CommunicationManager.cpp
  communication/tests/CommunicationManagerTest.cpp

  data/ContainerRegistry.cpp
  data/tests/ContainerRegistryTest.cpp
  data/tests/PooledRecordTest.cpp
  data/tests/RegisteredStackContainerTest.cpp

  dataview/CellMarker.cpp
  dataview/FaceMarker.cpp
  dataview/VertexMarker.cpp
  dataview/tests/CellMarkerTest.cpp
  dataview/tests/EnumeratorTest.cpp
  dataview/tests/FaceMarkerTest.cpp
  dataview/tests/MetaDataRecordTest.cpp
  dataview/tests/VertexMarkerTest.cpp

  grid/AutomatonState.cpp
  grid/GridControlEvent.cpp
  grid/GridStatistics.cpp
  grid/GridTraversalEvent.cpp
  grid/GridVertex.cpp
  grid/LatticeIndexMap.cpp
  grid/PeanoCurve.cpp
  grid/StackTopology.cpp
  grid/tests/AutomatonStateTest.cpp
  grid/tests/BoundaryDataAggregationTest.cpp
  grid/tests/GridControlEventTest.cpp
  grid/tests/GridStatisticsTest.cpp
  grid/tests/GridTraversalEventTest.cpp
  grid/tests/GridVertexTest.cpp
  grid/tests/LatticeIndexMapTest.cpp
  grid/tests/SelfSerialisingRecord.cpp
  grid/tests/SendReceiveContextTest.cpp
  grid/tests/StackContainerTest.cpp
  grid/tests/StackExchangeTest.cpp
  grid/tests/StackTest.cpp
  grid/tests/StackTopologyTest.cpp

  particles/FileReader.cpp
  particles/MultiscaleTransitions.cpp
  particles/ParticleFactory.cpp
  particles/TrajectoryDatabase.cpp
  particles/internal.cpp
  particles/particles.cpp
  particles/assignmentchecks/Database.cpp
  particles/assignmentchecks/Event.cpp
  particles/assignmentchecks/MeshSweepData.cpp
  particles/assignmentchecks/ParticleIdentifier.cpp
  particles/assignmentchecks/TracingAPI.cpp
  particles/assignmentchecks/tests/TestHelpers.cpp
  particles/assignmentchecks/tests/TestParticle.cpp
  particles/potentials/Springs.cpp
  particles/tests/MultiscaleTransitionsTest.cpp
  particles/tests/ParticlesTest.cpp
  particles/tests/TestHelpers.cpp
  particles/tests/TestParticle.cpp
  particles/tests/TrajectoryDatabaseTest.cpp

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
      NAME Peano${mode}${dim}D
      MODE ${mode}
      DIMENSION ${dim}
      SOURCES ${SOURCES}
      HEADERS ${HEADERS}
      LINK_LIBRARIES Tarch${mode}
    )
  endforeach()
endforeach()
