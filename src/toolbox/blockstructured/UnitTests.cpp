#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "tests/IandRInMatricesTest.h"
#include "tests/IandRSecondOrderTest.h"
#include "tests/IandRThirdOrderTest.h"
#include "tests/InterpolationTest.h"

tarch::tests::TestCase* toolbox::blockstructured::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new toolbox::blockstructured::tests::InterpolationTest());
  result->addTestCase(new toolbox::blockstructured::tests::IandRInMatricesTest());
  result->addTestCase(new toolbox::blockstructured::tests::IandRSecondOrderTest());
  result->addTestCase(new toolbox::blockstructured::tests::IandRThirdOrderTest());

  return result;
}
