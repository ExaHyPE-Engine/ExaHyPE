#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"
#include "toolbox/blockstructured/tests/IandRInMatricesTest.h"
#include "toolbox/blockstructured/tests/IandRSecondOrderTest.h"
#include "toolbox/blockstructured/tests/IandRThirdOrderTest.h"
#include "toolbox/blockstructured/tests/IandRVolumetricTest.h"
#include "toolbox/blockstructured/tests/InterpolationTest.h"


tarch::tests::TestCase* toolbox::blockstructured::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::
    TreeTestCaseCollection("blockstructured");

  result->addTestCase(new toolbox::blockstructured::tests::InterpolationTest());
  result->addTestCase(new toolbox::blockstructured::tests::IandRInMatricesTest()
  );
  result->addTestCase(new toolbox::blockstructured::tests::IandRSecondOrderTest(
  ));
  result->addTestCase(new toolbox::blockstructured::tests::IandRThirdOrderTest()
  );
  result->addTestCase(new toolbox::blockstructured::tests::IandRVolumetricTest()
  );

  return result;
}
