// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "exahype2/amr/tests/ChildDataStageTest.h"
#include "exahype2/amr/tests/InterpolationTest.h"
#include "exahype2/amr/tests/MultiresolutionDetailTest.h"
#include "exahype2/amr/tests/RestrictionTest.h"
#include "exahype2/dg/ader/tests/CauchyKovalewskiTest.h"
#include "exahype2/dg/ader/tests/CellErrorIntegralTest.h"
#include "exahype2/dg/ader/tests/KernelsOnDeviceTest.h"
#include "exahype2/dg/ader/tests/LinearPredictorTermsTest.h"
#include "exahype2/dg/ader/tests/PicardIterationTest.h"
#include "exahype2/dg/ader/tests/PointSourceContributionTest.h"
#include "exahype2/dg/ader/tests/PointSourcesTest.h"
#include "exahype2/dg/ader/tests/PredictorScratchTest.h"
#include "exahype2/dg/ader/tests/SpaceTimePredictorTest.h"
#include "exahype2/dg/ader/tests/SpaceTimeTermsTest.h"
#include "exahype2/dg/ader/tests/TimeAveragingTest.h"
#include "exahype2/dg/ader/tests/VolumeIntegralTest.h"
#include "exahype2/dg/rk/tests/KernelsOnDeviceTest.h"
#include "exahype2/dg/rk/tests/LinearCombinationTest.h"
#include "exahype2/dg/rk/tests/VolumeIntegralTest.h"
#include "exahype2/dg/tests/AMRMatricesTest.h"
#include "exahype2/dg/tests/BoundaryConditionsTest.h"
#include "exahype2/dg/tests/BoundaryExtrapolationTest.h"
#include "exahype2/dg/tests/DGTracerTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/EigenvalueStatisticsTest.h"
#include "exahype2/dg/tests/FaceIntegralTest.h"
#include "exahype2/dg/tests/GradientTest.h"
#include "exahype2/dg/tests/KernelsOnDeviceTest.h"
#include "exahype2/dg/tests/LimiterStatusTest.h"
#include "exahype2/dg/tests/LimiterTest.h"
#include "exahype2/dg/tests/LimitingSolverStateTest.h"
#include "exahype2/dg/tests/OperatorAlongAxisTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/dg/tests/SubcellProjectionTest.h"
#include "exahype2/fv/limiter/tests/SlopeLimiterTest.h"
#include "exahype2/fv/tests/ApplyBoundaryConditionsOnFaceTest.h"
#include "exahype2/fv/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/tests/CopyPatchTest.h"
#include "exahype2/fv/tests/EigenvalueStatisticsTest.h"
#include "exahype2/fv/tests/FiniteVolumeTest.h"
#include "exahype2/tests/CurviTest.h"
#include "exahype2/tests/EnumerationTest.h"
#include "exahype2/tests/NonNegativeReductionTest.h"
#include "exahype2/tests/NonNegativeStatisticsTest.h"
#include "exahype2/tests/PatchKernelsTest.h"
#include "exahype2/tests/RefinementControlServiceTest.h"
#include "exahype2/tests/RefinementControlTest.h"
#include "exahype2/tests/RefinementCriterionHelpersTest.h"
#include "exahype2/tests/TimeSteppingTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase* exahype2::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new exahype2::amr::tests::ChildDataStageTest());
  result->addTestCase(new exahype2::amr::tests::InterpolationTest());
  result->addTestCase(new exahype2::amr::tests::MultiresolutionDetailTest());
  result->addTestCase(new exahype2::amr::tests::RestrictionTest());

  result->addTestCase(new exahype2::dg::tests::AMRMatricesTest());
  result->addTestCase(new exahype2::dg::tests::BoundaryConditionsTest());
  result->addTestCase(new exahype2::dg::tests::BoundaryExtrapolationTest());
  result->addTestCase(new exahype2::dg::tests::DGTracerTest());
  result->addTestCase(new exahype2::dg::tests::DGUtilsTest());
  result->addTestCase(new exahype2::dg::tests::EigenvalueStatisticsTest());
  result->addTestCase(new exahype2::dg::tests::FaceIntegralTest());
  result->addTestCase(new exahype2::dg::tests::GradientTest());
  result->addTestCase(new exahype2::dg::tests::KernelsOnDeviceTest());
  result->addTestCase(new exahype2::dg::tests::LimiterStatusTest());
  result->addTestCase(new exahype2::dg::tests::LimiterTest());
  result->addTestCase(new exahype2::dg::tests::LimitingSolverStateTest());
  result->addTestCase(new exahype2::dg::tests::OperatorAlongAxisTest());
  result->addTestCase(new exahype2::dg::tests::RiemannTest());
  result->addTestCase(new exahype2::dg::tests::SubcellProjectionTest());

  result->addTestCase(new exahype2::dg::rk::tests::KernelsOnDeviceTest());
  result->addTestCase(new exahype2::dg::rk::tests::LinearCombinationTest());
  result->addTestCase(new exahype2::dg::rk::tests::VolumeIntegralTest());

  result->addTestCase(new exahype2::dg::ader::tests::CauchyKovalewskiTest());
  result->addTestCase(new exahype2::dg::ader::tests::CellErrorIntegralTest());
  result->addTestCase(new exahype2::dg::ader::tests::KernelsOnDeviceTest());
  result->addTestCase(new exahype2::dg::ader::tests::LinearPredictorTermsTest());
  result->addTestCase(new exahype2::dg::ader::tests::PicardIterationTest());
  result->addTestCase(new exahype2::dg::ader::tests::PointSourceContributionTest());
  result->addTestCase(new exahype2::dg::ader::tests::PointSourcesTest());
  result->addTestCase(new exahype2::dg::ader::tests::PredictorScratchTest());
  result->addTestCase(new exahype2::dg::ader::tests::SpaceTimePredictorTest());
  result->addTestCase(new exahype2::dg::ader::tests::SpaceTimeTermsTest());
  result->addTestCase(new exahype2::dg::ader::tests::TimeAveragingTest());
  result->addTestCase(new exahype2::dg::ader::tests::VolumeIntegralTest());

  result->addTestCase(new exahype2::fv::tests::CopyPatchTest());
  result->addTestCase(new exahype2::fv::tests::ApplySplit1DRiemannToPatchTest());
  result->addTestCase(new exahype2::fv::tests::EigenvalueStatisticsTest());
  result->addTestCase(new exahype2::fv::tests::FiniteVolumeTest());
  result->addTestCase(new exahype2::fv::tests::ApplyBoundaryConditionsOnFaceTest());
  result->addTestCase(new exahype2::fv::limiter::tests::SlopeLimiterTest());

  result->addTestCase(new exahype2::tests::CurviTest());
  result->addTestCase(new exahype2::tests::EnumerationTest());
  result->addTestCase(new exahype2::tests::NonNegativeReductionTest());
  result->addTestCase(new exahype2::tests::NonNegativeStatisticsTest());
  result->addTestCase(new exahype2::tests::PatchKernelsTest());
  result->addTestCase(new exahype2::tests::RefinementControlServiceTest());
  result->addTestCase(new exahype2::tests::RefinementControlTest());
  result->addTestCase(new exahype2::tests::RefinementCriterionHelpersTest());
  result->addTestCase(new exahype2::tests::TimeSteppingTest());

  return result;
}
