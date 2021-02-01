#include "UnitTests.h"


#include "peano4/grid/tests/SpacetreeTest.h"
#include "peano4/grid/tests/GridControlEventTest.h"
#include "peano4/parallel/tests/NodeTest.h"
#include "peano4/parallel/tests/PingPongTest.h"

#include "tarch/tests/TreeTestCaseCollection.h"


tarch::tests::TestCase* peano4::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection("");

  result->addTestCase( new peano4::grid::tests::SpacetreeTest() );
  result->addTestCase( new peano4::grid::tests::GridControlEventTest() );
  result->addTestCase( new peano4::parallel::tests::NodeTest() );
  result->addTestCase( new peano4::parallel::tests::PingPongTest() );

  return result;
}
