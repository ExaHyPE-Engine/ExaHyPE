// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AdaptiveTimeSteppingTest.h"

#include "exahype2/AdaptiveTimeStepping.h"
#include "tarch/mpi/Rank.h"

namespace {
  class Probe final : public exahype2::AdaptiveTimeStepping {
  public:
    explicit Probe(const exahype2::AdaptiveTimeStepping::Stability stability) :
      exahype2::AdaptiveTimeStepping(stability) {}

    [[nodiscard]] bool hasPositiveStabilityEigenvalue() const {
      return exahype2::AdaptiveTimeStepping::hasPositiveStabilityEigenvalue();
    }
    void proposeAdmissibleTimeStepSize(const double timeStepSizeFromCFL, const double terminalTime) {
      exahype2::AdaptiveTimeStepping::proposeAdmissibleTimeStepSize(timeStepSizeFromCFL, terminalTime);
    }
    void reduceMaxEigenvaluesAcrossRanks() { exahype2::AdaptiveTimeStepping::reduceMaxEigenvaluesAcrossRanks(); }
    void startAdaptiveTimeStep() { exahype2::AdaptiveTimeStepping::startAdaptiveTimeStep(); }
    void resetTimeStepStatistics() { exahype2::Solver::resetTimeStepStatistics(); }

    [[nodiscard]] double minTerminalTime() const override { return 10.0; }
    [[nodiscard]] double maxEigenvalueThisTimeStep() const { return _maxEigenvalueThisTimeStep; }
    [[nodiscard]] double maxScaledEigenvalueThisTimeStep() const { return _maxScaledEigenvalueThisTimeStep; }
    [[nodiscard]] double admissibleTimeStepSizeThisTimeStep() const { return _admissibleTimeStepSizeThisTimeStep; }

    void                 startGridConstructionStep() override {}
    void                 finishGridConstructionStep() override {}
    void                 suspendSolversForOneGridSweep() override {}
    void                 startGridInitialisationStep() override {}
    void                 finishGridInitialisationStep() override {}
    void                 startTimeStep() override {}
    void                 finishTimeStep() override {}
    void                 startPlottingStep() override {}
    void                 finishPlottingStep() override {}
    [[nodiscard]] double getMaxPatchSize() const override { return getMaxCellSize(); }
    [[nodiscard]] double getMinPatchSize() const override { return getMinCellSize(); }
    [[nodiscard]] int    getNumberOfDoFs() const override { return 0; }
    [[nodiscard]] bool   mayPlot() const override { return true; }
    void                 startSimulation() override {}
    void                 finishSimulation() override {}
  };

  void recordTimeStamp(Probe& solver, const double timeStamp) {
    solver.resetTimeStepStatistics();
    auto updates = exahype2::Solver::LocalUpdates{};
    updates.addTimeStamp(timeStamp);
    solver.mergeLocalUpdates(updates);
  }
} // namespace

exahype2::tests::AdaptiveTimeSteppingTest::AdaptiveTimeSteppingTest() :
  tarch::tests::TestCase("exahype2::tests::AdaptiveTimeSteppingTest") {}

void exahype2::tests::AdaptiveTimeSteppingTest::run() {
  testMethod(testMaxEigenvaluesKeepTheLargestPositiveValue);
  testMethod(testStartTimeStepCommitsTheProposalAndClearsTheMaxima);
  testMethod(testRankReductionTakesTheMaximaOfAllRanks);
  testMethod(testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum);
  testMethod(testProposalStopsAtTheTerminalTime);
}

void exahype2::tests::AdaptiveTimeSteppingTest::testMaxEigenvaluesKeepTheLargestPositiveValue() {
  Probe solver{exahype2::AdaptiveTimeStepping::Stability::PhysicalEigenvalue};
  validateNumericalEquals(solver.getMaxEigenvalue(), 0.0);

  constexpr double SmallEigenvalue = 1e-15;
  solver.setMaxEigenvalue(SmallEigenvalue);
  solver.setMaxScaledEigenvalue(SmallEigenvalue);
  validateEquals(solver.maxEigenvalueThisTimeStep(), SmallEigenvalue);
  validateEquals(solver.maxScaledEigenvalueThisTimeStep(), SmallEigenvalue);
  validateEquals(solver.hasPositiveStabilityEigenvalue(), true);

  solver.setMaxEigenvalue(0.5);
  solver.setMaxEigenvalue(2.0);
  solver.setMaxEigenvalue(1.0);
  solver.setMaxEigenvalue(0.0);
  solver.setMaxScaledEigenvalue(3.0);
  solver.setMaxScaledEigenvalue(4.0);

  validateNumericalEquals(solver.maxEigenvalueThisTimeStep(), 2.0);
  validateNumericalEquals(solver.maxScaledEigenvalueThisTimeStep(), 4.0);
  validateNumericalEquals(solver.getMaxEigenvalue(), 0.0);
}

void exahype2::tests::AdaptiveTimeSteppingTest::testStartTimeStepCommitsTheProposalAndClearsTheMaxima() {
  Probe solver{exahype2::AdaptiveTimeStepping::Stability::PhysicalEigenvalue};
  recordTimeStamp(solver, 1.0);
  solver.setMaxEigenvalue(2.0);
  solver.setMaxScaledEigenvalue(3.0);
  solver.proposeAdmissibleTimeStepSize(0.5, 10.0);
  validateNumericalEquals(solver.getTimeStepSize(), 0.0);

  solver.startAdaptiveTimeStep();
  validateNumericalEquals(solver.getTimeStepSize(), 0.5);
  validateNumericalEquals(solver.maxEigenvalueThisTimeStep(), 0.0);
  validateNumericalEquals(solver.maxScaledEigenvalueThisTimeStep(), 0.0);
}

void exahype2::tests::AdaptiveTimeSteppingTest::testRankReductionTakesTheMaximaOfAllRanks() {
  const auto rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
  const auto numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());

  Probe solver{exahype2::AdaptiveTimeStepping::Stability::ScaledEigenvalue};
  solver.setMaxEigenvalue(1.0 + rank);
  solver.setMaxScaledEigenvalue(10.0 + rank);
  solver.reduceMaxEigenvaluesAcrossRanks();

  validateNumericalEquals(solver.getMaxEigenvalue(), numberOfRanks);
  validateNumericalEquals(solver.maxEigenvalueThisTimeStep(), numberOfRanks);
  validateNumericalEquals(solver.maxScaledEigenvalueThisTimeStep(), 9.0 + numberOfRanks);
}

void exahype2::tests::AdaptiveTimeSteppingTest::testStabilityEigenvalueIsThePhysicalOrTheScaledMaximum() {
  Probe physical{exahype2::AdaptiveTimeStepping::Stability::PhysicalEigenvalue};
  physical.setMaxScaledEigenvalue(1.0);
  validateEquals(physical.hasPositiveStabilityEigenvalue(), false);
  physical.setMaxEigenvalue(1.0);
  validateEquals(physical.hasPositiveStabilityEigenvalue(), true);

  Probe scaled{exahype2::AdaptiveTimeStepping::Stability::ScaledEigenvalue};
  scaled.setMaxEigenvalue(1.0);
  validateEquals(scaled.hasPositiveStabilityEigenvalue(), false);
  scaled.setMaxScaledEigenvalue(1.0);
  validateEquals(scaled.hasPositiveStabilityEigenvalue(), true);
}

void exahype2::tests::AdaptiveTimeSteppingTest::testProposalStopsAtTheTerminalTime() {
  Probe solver{exahype2::AdaptiveTimeStepping::Stability::PhysicalEigenvalue};
  recordTimeStamp(solver, 1.0);

  solver.proposeAdmissibleTimeStepSize(0.5, 1.25);
  validateNumericalEquals(solver.admissibleTimeStepSizeThisTimeStep(), 0.25);
  solver.proposeAdmissibleTimeStepSize(0.5, 3.0);
  validateNumericalEquals(solver.admissibleTimeStepSizeThisTimeStep(), 0.5);
}
