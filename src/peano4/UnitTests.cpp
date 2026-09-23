// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "peano4/communication/tests/CommunicationManagerTest.h"
#include "peano4/data/tests/ContainerRegistryTest.h"
#include "peano4/data/tests/MemoryUsageTest.h"
#include "peano4/data/tests/PooledRecordTest.h"
#include "peano4/data/tests/RegisteredStackContainerTest.h"
#include "peano4/dataview/tests/CellMarkerTest.h"
#include "peano4/dataview/tests/EnumeratorTest.h"
#include "peano4/dataview/tests/FaceMarkerTest.h"
#include "peano4/dataview/tests/MetaDataRecordTest.h"
#include "peano4/dataview/tests/VertexMarkerTest.h"
#include "peano4/grid/tests/AutomatonStateTest.h"
#include "peano4/grid/tests/BoundaryDataAggregationTest.h"
#include "peano4/grid/tests/GridControlEventTest.h"
#include "peano4/grid/tests/GridStatisticsTest.h"
#include "peano4/grid/tests/GridTraversalEventTest.h"
#include "peano4/grid/tests/GridVertexTest.h"
#include "peano4/grid/tests/LatticeIndexMapTest.h"
#include "peano4/grid/tests/SendReceiveContextTest.h"
#include "peano4/grid/tests/StackContainerTest.h"
#include "peano4/grid/tests/StackExchangeTest.h"
#include "peano4/grid/tests/StackTest.h"
#include "peano4/grid/tests/StackTopologyTest.h"
#include "peano4/particles/assignmentchecks/tests/TestHelpers.h"
#include "peano4/particles/tests/MultiscaleTransitionsTest.h"
#include "peano4/particles/tests/ParticlesTest.h"
#include "peano4/particles/tests/TestHelpers.h"
#include "peano4/particles/tests/TrajectoryDatabaseTest.h"
#include "peano4/sfc/tests/GrammarTest.h"
#include "peano4/sfc/tests/SpaceFillingCurveTest.h"
#include "peano4/tests/LoopTest.h"
#include "peano4/topology/tests/ChunkedTasksTest.h"
#include "peano4/topology/tests/TopologyManagerTest.h"
#include "peano4/topology/tests/TreeTest.h"
#include "peano4/topology/tests/TreeTraversalTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* peano4::getUnitTests() {
  auto* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new peano4::grid::tests::AutomatonStateTest());
  result->addTestCase(new peano4::grid::tests::BoundaryDataAggregationTest());
  result->addTestCase(new peano4::grid::tests::GridControlEventTest());
  result->addTestCase(new peano4::grid::tests::GridStatisticsTest());
  result->addTestCase(new peano4::grid::tests::GridTraversalEventTest());
  result->addTestCase(new peano4::grid::tests::GridVertexTest());
  result->addTestCase(new peano4::grid::tests::LatticeIndexMapTest());
  result->addTestCase(new peano4::grid::tests::SendReceiveContextTest());
  result->addTestCase(new peano4::grid::tests::StackContainerTest());
  result->addTestCase(new peano4::grid::tests::StackExchangeTest());
  result->addTestCase(new peano4::grid::tests::StackTest());
  result->addTestCase(new peano4::grid::tests::StackTopologyTest());

  result->addTestCase(new peano4::dataview::tests::CellMarkerTest());
  result->addTestCase(new peano4::dataview::tests::EnumeratorTest());
  result->addTestCase(new peano4::dataview::tests::FaceMarkerTest());
  result->addTestCase(new peano4::dataview::tests::MetaDataRecordTest());
  result->addTestCase(new peano4::dataview::tests::VertexMarkerTest());

  result->addTestCase(new peano4::communication::tests::CommunicationManagerTest());
  result->addTestCase(new peano4::data::tests::ContainerRegistryTest());
  result->addTestCase(new peano4::data::tests::MemoryUsageTest());
  result->addTestCase(new peano4::data::tests::PooledRecordTest());
  result->addTestCase(new peano4::data::tests::RegisteredStackContainerTest());

  result->addTestCase(new peano4::particles::tests::MultiscaleTransitionsTest());
  result->addTestCase(new peano4::particles::tests::ParticlesTest());
  result->addTestCase(new peano4::particles::tests::TestHelpers());
  result->addTestCase(new peano4::particles::tests::TrajectoryDatabaseTest());
  result->addTestCase(new peano4::particles::assignmentchecks::tests::TestHelpers());

  result->addTestCase(new peano4::sfc::tests::GrammarTest());
  result->addTestCase(new peano4::sfc::tests::SpaceFillingCurveTest());

  result->addTestCase(new peano4::topology::tests::ChunkedTasksTest());
  result->addTestCase(new peano4::topology::tests::TopologyManagerTest());
  result->addTestCase(new peano4::topology::tests::TreeTest());
  result->addTestCase(new peano4::topology::tests::TreeTraversalTest());

  result->addTestCase(new peano4::tests::LoopTest());

  return result;
}
