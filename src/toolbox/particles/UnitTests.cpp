#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"


#include "toolbox/particles/tests/MultiscaleTransitionsTest.h"
#include "toolbox/particles/tests/TestHelpers.h"



tarch::tests::TestCase* toolbox::particles::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection( "particles" );

  result->addTestCase( new toolbox::particles::tests::MultiscaleTransitionsTest() );
  result->addTestCase( new toolbox::particles::tests::TestHelpers() );

  return result;
}
