// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UnitTests.h"

#include "exahype2/dg/ader/tests/CorrectorTest.h"
#include "exahype2/dg/ader/tests/LinearPredictorTest.h"
#include "exahype2/dg/ader/tests/NonlinearPredictorTest.h"
#include "exahype2/dg/ader/tests/PredictorTest.h"
#include "exahype2/dg/ader/tests/SolverTest.h"
#include "exahype2/dg/limiting/tests/DetectorTest.h"
#include "exahype2/dg/limiting/tests/LimiterStatusTest.h"
#include "exahype2/dg/limiting/tests/SubcellLimiterTest.h"
#include "exahype2/dg/limiting/tests/SubcellSchemeTest.h"
#include "exahype2/dg/rk/tests/SolverTest.h"
#include "exahype2/dg/rk/tests/StagesTest.h"
#include "exahype2/dg/tests/BasisTest.h"
#include "exahype2/dg/tests/KernelsTest.h"
#include "exahype2/dg/tests/TracingTest.h"
#include "exahype2/fv/limiter/tests/SlopeLimiterTest.h"
#include "exahype2/fv/tests/FiniteVolumeTest.h"
#include "exahype2/fv/tests/KernelsTest.h"
#include "exahype2/fv/tests/SolverTest.h"
#include "exahype2/fv/tests/TracingTest.h"
#include "exahype2/particles/tests/ForcesTest.h"
#include "exahype2/particles/tests/ParticleErrorMeasurementTest.h"
#include "exahype2/particles/tests/ParticleSetTest.h"
#include "exahype2/particles/tests/TrajectoryDatabaseTest.h"
#include "exahype2/particles/tests/TrajectoryRecordingTest.h"
#include "exahype2/plotter/tests/FacePlotterTest.h"
#include "exahype2/plotter/tests/PatchPlotterTest.h"
#include "exahype2/runner/tests/ConsoleTest.h"
#include "exahype2/runner/tests/RunnerTest.h"
#include "exahype2/runner/tests/RuntimeParametersTest.h"
#include "exahype2/runner/tests/StatisticsTest.h"
#include "exahype2/terms/tests/TermsTest.h"
#include "exahype2/tests/CurviTest.h"
#include "exahype2/tests/ErrorMeasurementTest.h"
#include "exahype2/tests/LevelJumpsTest.h"
#include "exahype2/tests/MultiresolutionDetailTest.h"
#include "exahype2/tests/RefinementRequestsTest.h"
#include "exahype2/tests/RefinementVotesTest.h"
#include "exahype2/tests/SolverRepositoryTest.h"
#include "exahype2/tests/TimeSteppingTest.h"
#include "tarch/tests/TreeTestCaseCollection.h"

tarch::tests::TestCase*
exahype2::getUnitTests() {
  auto* const result = new tarch::tests::TreeTestCaseCollection();

  result->addTestCase(new exahype2::tests::CurviTest());
  result->addTestCase(new exahype2::tests::ErrorMeasurementTest());
  result->addTestCase(new exahype2::tests::LevelJumpsTest());
  result->addTestCase(new exahype2::tests::MultiresolutionDetailTest());
  result->addTestCase(new exahype2::tests::RefinementRequestsTest());
  result->addTestCase(new exahype2::tests::RefinementVotesTest());
  result->addTestCase(new exahype2::tests::SolverRepositoryTest());
  result->addTestCase(new exahype2::tests::TimeSteppingTest());

  result->addTestCase(new exahype2::fv::tests::FiniteVolumeTest());
  result->addTestCase(new exahype2::fv::tests::KernelsTest());
  result->addTestCase(new exahype2::fv::tests::SolverTest());
  result->addTestCase(new exahype2::fv::tests::TracingTest());
  result->addTestCase(new exahype2::fv::limiter::tests::SlopeLimiterTest());

  result->addTestCase(new exahype2::dg::tests::BasisTest());
  result->addTestCase(new exahype2::dg::tests::KernelsTest());
  result->addTestCase(new exahype2::dg::tests::TracingTest());

  result->addTestCase(new exahype2::dg::limiting::tests::DetectorTest());
  result->addTestCase(new exahype2::dg::limiting::tests::LimiterStatusTest());
  result->addTestCase(new exahype2::dg::limiting::tests::SubcellLimiterTest());
  result->addTestCase(new exahype2::dg::limiting::tests::SubcellSchemeTest());

  result->addTestCase(new exahype2::dg::ader::tests::CorrectorTest());
  result->addTestCase(new exahype2::dg::ader::tests::LinearPredictorTest());
  result->addTestCase(new exahype2::dg::ader::tests::NonlinearPredictorTest());
  result->addTestCase(new exahype2::dg::ader::tests::PredictorTest());
  result->addTestCase(new exahype2::dg::ader::tests::SolverTest());

  result->addTestCase(new exahype2::dg::rk::tests::SolverTest());
  result->addTestCase(new exahype2::dg::rk::tests::StagesTest());

  result->addTestCase(new exahype2::particles::tests::ForcesTest());
  result->addTestCase(new exahype2::particles::tests::ParticleErrorMeasurementTest());
  result->addTestCase(new exahype2::particles::tests::ParticleSetTest());
  result->addTestCase(new exahype2::particles::tests::TrajectoryDatabaseTest());
  result->addTestCase(new exahype2::particles::tests::TrajectoryRecordingTest());

  result->addTestCase(new exahype2::plotter::tests::FacePlotterTest());
  result->addTestCase(new exahype2::plotter::tests::PatchPlotterTest());

  result->addTestCase(new exahype2::runner::tests::ConsoleTest());
  result->addTestCase(new exahype2::runner::tests::RunnerTest());
  result->addTestCase(new exahype2::runner::tests::RuntimeParametersTest());
  result->addTestCase(new exahype2::runner::tests::StatisticsTest());

  result->addTestCase(new exahype2::terms::tests::TermsTest());

  return result;
}
