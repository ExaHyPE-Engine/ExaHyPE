// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverStatisticsTest.h"

#include "runner/SolverStatistics.h"

namespace {
  struct PlainSolver final {
    [[nodiscard]] double getMinTimeStepSize() const { return 0.25; }
    [[nodiscard]] double getMinCellSize() const { return 0.5; }
    [[nodiscard]] double getMaxCellSize() const { return 1.5; }
  };

  struct LimitedSolver final {
    [[nodiscard]] double getMinTimeStepSize() const { return 0.125; }
    [[nodiscard]] double getMinCellSize() const { return 1.0; }
    [[nodiscard]] double getMaxCellSize() const { return 3.0; }
    [[nodiscard]] double getMaxEigenvalue() const { return 7.0; }
    [[nodiscard]] long   getNumberOfTroubledCells() const { return troubledCells; }
    long                 troubledCells{0};
  };

  void appendNothing([[maybe_unused]] const std::vector<runner::SolverStatistic>& statistics) {}
} // namespace

runner::tests::SolverStatisticsTest::SolverStatisticsTest() :
  TestCase("runner::tests::SolverStatisticsTest") {}

void runner::tests::SolverStatisticsTest::run() {
  testMethod(testValueRangeAndCountCarryTheirKind);
  testMethod(testASolverWithoutEigenvalueReportsStepAndMeshWidth);
  testMethod(testASolverWithEigenvalueAndTroubledCellsAppendsBoth);
  testMethod(testAdditionalStatisticsFollowTheMeshWidth);
  testMethod(testParticleStatisticsCountTheParticles);
}

void runner::tests::SolverStatisticsTest::testValueRangeAndCountCarryTheirKind() {
  const auto value = runner::makeValueStatistic("dt", 2.0);
  validateEquals(value.label, std::string("dt"));
  validateTrue(value.kind == runner::SolverStatistic::Kind::Value);
  validateNumericalEquals(value.minimum, 2.0);
  validateNumericalEquals(value.maximum, 2.0);

  const auto range = runner::makeRangeStatistic("h", 1.0, 4.0);
  validateTrue(range.kind == runner::SolverStatistic::Kind::Value);
  validateNumericalEquals(range.minimum, 1.0);
  validateNumericalEquals(range.maximum, 4.0);

  const auto count = runner::makeCountStatistic("Troubled", 12L);
  validateTrue(count.kind == runner::SolverStatistic::Kind::Count);
  validateEquals(count.count, 12L);
}

void runner::tests::SolverStatisticsTest::testASolverWithoutEigenvalueReportsStepAndMeshWidth() {
  const PlainSolver solver;
  const auto        statistics = runner::makeSolverStatistics("Plain", solver, appendNothing);
  validateEquals(statistics.first, std::string("Plain"));
  validateEquals(statistics.second.size(), 2U);
  validateEquals(statistics.second[0].label, std::string("dt"));
  validateNumericalEquals(statistics.second[0].minimum, 0.25);
  validateEquals(statistics.second[1].label, std::string("h"));
  validateNumericalEquals(statistics.second[1].minimum, 0.5);
  validateNumericalEquals(statistics.second[1].maximum, 1.5);
}

void runner::tests::SolverStatisticsTest::testASolverWithEigenvalueAndTroubledCellsAppendsBoth() {
  LimitedSolver solver;
  const auto    calm = runner::makeSolverStatistics("Limited", solver, appendNothing);
  validateEquals(calm.second.size(), 3U);
  validateEquals(calm.second[2].label, std::string("λ_max"));
  validateNumericalEquals(calm.second[2].maximum, 7.0);

  solver.troubledCells = 5;
  const auto troubled  = runner::makeSolverStatistics("Limited", solver, appendNothing);
  validateEquals(troubled.second.size(), 4U);
  validateEquals(troubled.second[3].label, std::string("Troubled"));
  validateEquals(troubled.second[3].count, 5L);
}

void runner::tests::SolverStatisticsTest::testAdditionalStatisticsFollowTheMeshWidth() {
  const PlainSolver solver;
  const auto        statistics =
    runner::makeSolverStatistics("Plain", solver, [](std::vector<runner::SolverStatistic>& additionalStatistics) {
      additionalStatistics.push_back(runner::makeCountStatistic("Tracers", 3L));
    });
  validateEquals(statistics.second.size(), 3U);
  validateEquals(statistics.second[2].label, std::string("Tracers"));
  validateEquals(statistics.second[2].count, 3L);
}

void runner::tests::SolverStatisticsTest::testParticleStatisticsCountTheParticles() {
  const auto statistics = runner::makeParticleStatistics("Tracers", 42);
  validateEquals(statistics.first, std::string("Tracers"));
  validateEquals(statistics.second.size(), 1U);
  validateEquals(statistics.second[0].label, std::string("Particles"));
  validateEquals(statistics.second[0].count, 42L);
}
