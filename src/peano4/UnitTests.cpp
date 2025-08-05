// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "grid/tests/GridControlEventTest.h"
#include "grid/tests/GridTraversalEventGeneratorTest.h"
#include "grid/tests/SpacetreeTest.h"
#include "parallel/tests/NodeTest.h"
#ifdef WITH_MPI
#include "parallel/tests/PingPongTest.h"
#endif
#include "tarch/tests/TreeTestCaseCollection.h"
#include "utils/tests/LinearisedTest.h"

tarch::tests::TestCase* peano4::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new grid::tests::SpacetreeTest());
  result->addTestCase(new grid::tests::GridControlEventTest());
  result->addTestCase(new grid::tests::GridTraversalEventGeneratorTest());

  result->addTestCase(new parallel::tests::NodeTest());
#ifdef WITH_MPI
  result->addTestCase(new parallel::tests::PingPongTest());
#endif

  result->addTestCase(new utils::tests::LinearisedTest());

  return result;
}
