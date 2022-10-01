#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"


tarch::tests::TestCase* swift2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

/*
  result->addTestCase( new exahype2::dg::tests::DGTest() );
  result->addTestCase( new exahype2::fv::rusanov::tests::CopyPatchTest() );
  result->addTestCase( new exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest() );
  result->addTestCase( new exahype2::aderdg::tests::ADERDGTest() );
*/

  return result;
}
