#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"


#include "toolbox/blockstructured/tests/InterpolationTest.h"
#include "toolbox/blockstructured/tests/IandRInMatricesTest.h"
#include "toolbox/blockstructured/tests/IandRSecondOrderTest.h"


tarch::tests::TestCase* toolbox::blockstructured::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection("blockstructured");

  result->addTestCase( new toolbox::blockstructured::tests::InterpolationTest() );
  result->addTestCase( new toolbox::blockstructured::tests::IandRInMatricesTest() );
  result->addTestCase( new toolbox::blockstructured::tests::IandRSecondOrderTest() );

  return result;
}
