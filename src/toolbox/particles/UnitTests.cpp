#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"


#include "toolbox/blockstructured/tests/InterpolationTest.h"



tarch::tests::TestCase* toolbox::blockstructured::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection("blockstructured");

  result->addTestCase( new toolbox::blockstructured::tests::InterpolationTest() );

  return result;
}
