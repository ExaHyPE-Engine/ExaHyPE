#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"

#include "exahype2/aderdg/tests/ADERDGTest.h"
#include "fv/rusanov/tests/ApplySplit1DRiemannToPatchTest.h"
#include "fv/rusanov/tests/CopyPatchTest.h"


tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection( "exahype2" );

  result->addTestCase( new exahype2::fv::rusanov::tests::CopyPatchTest() );
  result->addTestCase( new exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest() );
  result->addTestCase( new exahype2::aderdg::tests::ADERDGTest() );

  return result;
}
