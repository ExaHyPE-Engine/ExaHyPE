#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"




tarch::tests::TestCase* petsc::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection("petsc");

  //result->addTestCase( new toolbox::blockstructured::tests::InterpolationTest() );

  return result;
}
