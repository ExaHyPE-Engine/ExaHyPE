// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "grid/tests/AutomatonStateTest.h"
#include "grid/tests/CellTypeTest.h"
#include "grid/tests/FaceTypeTest.h"
#include "grid/tests/GridControlEventTest.h"
#include "grid/tests/GridStatisticsTest.h"
#include "grid/tests/GridTraversalEventGeneratorTest.h"
#include "grid/tests/GridVertexTest.h"
#include "grid/tests/SpacetreeHelperTest.h"
#include "grid/tests/SpacetreeTest.h"
#include "grid/tests/TreeIdSetTest.h"
#include "grid/tests/VertexLookupTableTest.h"
#include "grid/tests/VertexTypeTest.h"
#include "parallel/tests/NodeTest.h"
#ifdef WITH_MPI
#include "parallel/tests/PingPongTest.h"
#endif
#include "stacks/tests/StackTest.h"
#include "stacks/tests/TreeToStacksTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"
#include "utils/tests/LinearisedTest.h"

tarch::tests::TestCase* peano4::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new grid::tests::AutomatonStateTest());
  result->addTestCase(new grid::tests::CellTypeTest());
  result->addTestCase(new grid::tests::FaceTypeTest());
  result->addTestCase(new grid::tests::GridControlEventTest());
  result->addTestCase(new grid::tests::GridTraversalEventGeneratorTest());
  result->addTestCase(new grid::tests::GridStatisticsTest());
  result->addTestCase(new grid::tests::TreeIdSetTest());
  result->addTestCase(new grid::tests::GridVertexTest());
  result->addTestCase(new grid::tests::SpacetreeHelperTest());
  result->addTestCase(new grid::tests::SpacetreeTest());
  result->addTestCase(new grid::tests::VertexLookupTableTest());
  result->addTestCase(new grid::tests::VertexTypeTest());

  result->addTestCase(new parallel::tests::NodeTest());
#ifdef WITH_MPI
  result->addTestCase(new parallel::tests::PingPongTest());
#endif

  result->addTestCase(new utils::tests::LinearisedTest());

  result->addTestCase(new stacks::tests::StackTest());
  result->addTestCase(new stacks::tests::TreeToStacksTest());

  return result;
}
