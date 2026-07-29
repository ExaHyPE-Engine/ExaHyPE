// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "grid/tests/AutomatonStateTest.h"
#include "grid/tests/BoundaryDataAggregationTest.h"
#include "grid/tests/CellTypeTest.h"
#include "grid/tests/FaceTypeTest.h"
#include "grid/tests/GridControlEventTest.h"
#include "grid/tests/GridStatisticsTest.h"
#include "grid/tests/GridTraversalEventGeneratorTest.h"
#include "grid/tests/GridVertexTest.h"
#include "grid/tests/PeriodicTopologyTest.h"
#include "grid/tests/SpacetreeHelperTest.h"
#include "grid/tests/SpacetreeTest.h"
#include "grid/tests/StackContainerTest.h"
#include "grid/tests/StackTopologyTest.h"
#include "grid/tests/TreeIdSetTest.h"
#include "grid/tests/VertexLookupTableTest.h"
#include "grid/tests/VertexTypeTest.h"
#include "parallel/tests/ExchangeChannelsTest.h"
#include "parallel/tests/PingPongTest.h"
#include "parallel/tests/RunControlTest.h"
#include "parallel/tests/SpacetreeSetTest.h"
#include "parallel/tests/TreeIdMappingTest.h"
#include "stacks/tests/StackTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"
#include "tests/LoopTest.h"

tarch::tests::TestCase* peano4::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new grid::tests::AutomatonStateTest());
  result->addTestCase(new grid::tests::BoundaryDataAggregationTest());
  result->addTestCase(new grid::tests::CellTypeTest());
  result->addTestCase(new grid::tests::FaceTypeTest());
  result->addTestCase(new grid::tests::GridControlEventTest());
  result->addTestCase(new grid::tests::GridTraversalEventGeneratorTest());
  result->addTestCase(new grid::tests::GridStatisticsTest());
  result->addTestCase(new grid::tests::TreeIdSetTest());
  result->addTestCase(new grid::tests::GridVertexTest());
  result->addTestCase(new grid::tests::PeriodicTopologyTest());
  result->addTestCase(new grid::tests::SpacetreeHelperTest());
  result->addTestCase(new grid::tests::SpacetreeTest());
  result->addTestCase(new grid::tests::StackContainerTest());
  result->addTestCase(new grid::tests::StackTopologyTest());
  result->addTestCase(new grid::tests::VertexLookupTableTest());
  result->addTestCase(new grid::tests::VertexTypeTest());

  result->addTestCase(new parallel::tests::ExchangeChannelsTest());
  result->addTestCase(new parallel::tests::TreeIdMappingTest());
  result->addTestCase(new parallel::tests::RunControlTest());
  result->addTestCase(new parallel::tests::SpacetreeSetTest());
  result->addTestCase(new parallel::tests::PingPongTest());

  result->addTestCase(new tests::LoopTest());

  result->addTestCase(new stacks::tests::StackTest());

  return result;
}
