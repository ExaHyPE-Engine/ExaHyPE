// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "exahype2/dg/rusanov/tests/RiemannTest.h"
#include "exahype2/dg/tests/CellIntegralTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/EigenvalueStatisticsTest.h"
#include "exahype2/dg/tests/LimiterStatusTest.h"
#include "exahype2/dg/tests/LimiterTest.h"
#include "exahype2/dg/tests/LimitingSolverStateTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/dg/tests/SubcellProjectionTest.h"
#include "exahype2/fv/limiter/tests/SlopeLimiterTest.h"
#include "exahype2/fv/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/tests/CopyPatchTest.h"
#include "exahype2/fv/tests/EigenvalueStatisticsTest.h"
#include "exahype2/fv/tests/FiniteVolumeTest.h"
#include "exahype2/fv/tests/HandleBoundaryTest.h"
#include "exahype2/tests/NonNegativeReductionTest.h"
#include "exahype2/tests/NonNegativeStatisticsTest.h"
#include "exahype2/tests/RefinementControlServiceTest.h"
#include "exahype2/tests/RefinementControlTest.h"
#include "exahype2/tests/RefinementCriterionHelpersTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* exahype2::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new exahype2::dg::tests::CellIntegralTest());
  result->addTestCase(new exahype2::dg::tests::DGUtilsTest());
  result->addTestCase(new exahype2::dg::tests::EigenvalueStatisticsTest());
  result->addTestCase(new exahype2::dg::tests::LimiterStatusTest());
  result->addTestCase(new exahype2::dg::tests::LimiterTest());
  result->addTestCase(new exahype2::dg::tests::LimitingSolverStateTest());
  result->addTestCase(new exahype2::dg::tests::RiemannTest());
  result->addTestCase(new exahype2::dg::tests::SubcellProjectionTest());
  result->addTestCase(new exahype2::dg::rusanov::tests::RiemannTest());

  result->addTestCase(new exahype2::fv::tests::CopyPatchTest());
  result->addTestCase(new exahype2::fv::tests::ApplySplit1DRiemannToPatchTest());
  result->addTestCase(new exahype2::fv::tests::EigenvalueStatisticsTest());
  result->addTestCase(new exahype2::fv::tests::FiniteVolumeTest());
  result->addTestCase(new exahype2::fv::tests::HandleBoundaryTest());
  result->addTestCase(new exahype2::fv::limiter::tests::SlopeLimiterTest());

  result->addTestCase(new exahype2::tests::NonNegativeReductionTest());
  result->addTestCase(new exahype2::tests::NonNegativeStatisticsTest());
  result->addTestCase(new exahype2::tests::RefinementControlServiceTest());
  result->addTestCase(new exahype2::tests::RefinementControlTest());
  result->addTestCase(new exahype2::tests::RefinementCriterionHelpersTest());

  return result;
}
