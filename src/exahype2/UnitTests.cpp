#include "UnitTests.h"

#include "tarch/tests/TreeTestCaseCollection.h"

#include "exahype2/fv/tests/CopyPatchTest.h"
#include "exahype2/fv/tests/applySplit1DRiemannToPatch_Overlap1AoSTest.h"

#include "exahype2/aderdg/tests/ADERDGTest.h"


tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection( "exahype2" );

  result->addTestCase( new exahype2::fv::tests::CopyPatchTest() );
  result->addTestCase( new exahype2::fv::tests::applySplit1DRiemannToPatch_Overlap1AoSTest() );
  result->addTestCase( new exahype2::aderdg::tests::ADERDGTest() );

  return result;
}
