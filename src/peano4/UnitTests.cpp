// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "peano4/checkpoint/tests/CellDataWriterTest.h"
#include "peano4/checkpoint/tests/LatticeTest.h"
#include "peano4/checkpoint/tests/ParticleDataWriterTest.h"
#include "peano4/communication/tests/CommunicationManagerTest.h"
#include "peano4/data/tests/ContainerRegistryTest.h"
#include "peano4/data/tests/RegisteredStackContainerTest.h"
#include "peano4/grid/tests/AutomatonStateTest.h"
#include "peano4/grid/tests/BoundaryDataAggregationTest.h"
#include "peano4/grid/tests/GridControlEventTest.h"
#include "peano4/grid/tests/GridStatisticsTest.h"
#include "peano4/grid/tests/GridTraversalEventTest.h"
#include "peano4/grid/tests/GridVertexTest.h"
#include "peano4/grid/tests/LatticeIndexMapTest.h"
#include "peano4/grid/tests/StackTopologyTest.h"
#include "peano4/stacks/tests/StackTest.h"
#include "peano4/tests/LoopTest.h"
#include "peano4/topology/tests/ChunkedTasksTest.h"
#include "peano4/topology/tests/TopologyManagerTest.h"
#include "peano4/topology/tests/TreeTest.h"
#include "peano4/topology/tests/TreeTraversalTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* peano4::getUnitTests() {
  auto* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new peano4::checkpoint::tests::CellDataWriterTest());
  result->addTestCase(new peano4::checkpoint::tests::LatticeTest());
  result->addTestCase(new peano4::checkpoint::tests::ParticleDataWriterTest());
  result->addTestCase(new peano4::grid::tests::AutomatonStateTest());
  result->addTestCase(new peano4::grid::tests::BoundaryDataAggregationTest());
  result->addTestCase(new peano4::grid::tests::GridControlEventTest());
  result->addTestCase(new peano4::grid::tests::GridStatisticsTest());
  result->addTestCase(new peano4::grid::tests::GridTraversalEventTest());
  result->addTestCase(new peano4::grid::tests::GridVertexTest());
  result->addTestCase(new peano4::grid::tests::LatticeIndexMapTest());
  result->addTestCase(new peano4::grid::tests::StackTopologyTest());

  result->addTestCase(new peano4::communication::tests::CommunicationManagerTest());
  result->addTestCase(new peano4::data::tests::ContainerRegistryTest());
  result->addTestCase(new peano4::data::tests::RegisteredStackContainerTest());

  result->addTestCase(new peano4::topology::tests::ChunkedTasksTest());
  result->addTestCase(new peano4::topology::tests::TopologyManagerTest());
  result->addTestCase(new peano4::topology::tests::TreeTest());
  result->addTestCase(new peano4::topology::tests::TreeTraversalTest());

  result->addTestCase(new peano4::tests::LoopTest());

  result->addTestCase(new peano4::stacks::tests::StackTest());

  return result;
}
