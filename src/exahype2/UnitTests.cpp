// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "exahype2/dg/rusanov/tests/RiemannTest.h"
#include "exahype2/dg/tests/CellIntegralTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/fv/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/tests/CopyPatchTest.h"
#include "exahype2/tests/RefinementControlServiceTest.h"
#include "exahype2/tests/RefinementControlTest.h"
#include "exahype2/tests/RefinementCriterionHelpersTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* exahype2::getUnitTests() {
  tarch::tests::TreeTestCaseCollection* result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new dg::tests::CellIntegralTest());
  result->addTestCase(new dg::tests::DGUtilsTest());
  result->addTestCase(new dg::tests::RiemannTest());
  result->addTestCase(new dg::rusanov::tests::RiemannTest());

  result->addTestCase(new fv::tests::CopyPatchTest());
  result->addTestCase(new fv::tests::ApplySplit1DRiemannToPatchTest());

  result->addTestCase(new tests::RefinementControlServiceTest());
  result->addTestCase(new tests::RefinementControlTest());
  result->addTestCase(new tests::RefinementCriterionHelpersTest());

  return result;
}
