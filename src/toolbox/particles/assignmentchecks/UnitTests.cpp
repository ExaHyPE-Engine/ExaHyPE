#include "UnitTests.h"


#include "tarch/tests/TreeTestCaseCollection.h"


// #include "toolbox/particles/assignmentchecks/tests/MultiscaleTransitionsTest.h"
#include "toolbox/particles/assignmentchecks/tests/TestHelpers.h"



tarch::tests::TestCase* toolbox::particles::assignmentchecks::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection( "particles-assignmentchecks" );

  // result->addTestCase( new toolbox::particles::assignmentchecks::tests::MultiscaleTransitionsTest() );
  result->addTestCase( new toolbox::particles::assignmentchecks::tests::TestHelpers() );

  return result;
}
