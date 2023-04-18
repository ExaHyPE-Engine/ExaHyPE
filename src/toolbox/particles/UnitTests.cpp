#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"


#include "toolbox/particles/tests/MultiscaleTransitionsTest.h"



tarch::tests::TestCase* toolbox::particles::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection( "particles" );

  result->addTestCase( new toolbox::particles::tests::MultiscaleTransitionsTest() );

  return result;
}
