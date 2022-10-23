#include "UnitTests.h"

#include "dg/tests/CellIntegralTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#include "exahype2/aderdg/tests/ADERDGTest.h"
#include "exahype2/dg/tests/CellIntegralTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/dg/rusanov/tests/RiemannTest.h"
#include "exahype2/fv/tests/InterpolationRestrictionTest.h"
#include "exahype2/fv/rusanov/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/rusanov/tests/CopyPatchTest.h"


tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase( new exahype2::dg::tests::CellIntegralTest() );
  result->addTestCase( new exahype2::dg::tests::DGUtilsTest() );
  result->addTestCase( new exahype2::dg::tests::RiemannTest() );
  result->addTestCase( new exahype2::dg::rusanov::tests::RiemannTest() );
  result->addTestCase( new exahype2::fv::tests::InterpolationRestrictionTest() );
  result->addTestCase( new exahype2::fv::rusanov::tests::CopyPatchTest() );
  result->addTestCase( new exahype2::fv::rusanov::tests::ApplySplit1DRiemannToPatchTest() );
  result->addTestCase( new exahype2::aderdg::tests::ADERDGTest() );

  return result;
}
