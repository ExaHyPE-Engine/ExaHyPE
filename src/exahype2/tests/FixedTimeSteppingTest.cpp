// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FixedTimeSteppingTest.h"

#include "exahype2/FixedTimeStepping.h"

namespace {
  class Probe final : public exahype2::FixedTimeStepping {
  public:
    using exahype2::FixedTimeStepping::setFixedTimeStepSize;

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
} // namespace

exahype2::tests::FixedTimeSteppingTest::FixedTimeSteppingTest() :
  tarch::tests::TestCase("exahype2::tests::FixedTimeSteppingTest") {}

void exahype2::tests::FixedTimeSteppingTest::run() { testMethod(testTimeStepSizeIsZeroUntilStored); }

void exahype2::tests::FixedTimeSteppingTest::testTimeStepSizeIsZeroUntilStored() {
  Probe solver;
  validateNumericalEquals(solver.getTimeStepSize(), 0.0);

  solver.setFixedTimeStepSize(0.25);
  validateNumericalEquals(solver.getTimeStepSize(), 0.25);
}
