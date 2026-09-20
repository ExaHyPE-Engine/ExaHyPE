// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverRepositoryTest.h"

#include "exahype2/SolverRepository.h"

namespace {
  struct RecordingSolver final {
    double minTimeStamp{0.0};
    double maxTimeStamp{0.0};
    double minTimeStepSize{0.0};
    double maxTimeStepSize{0.0};
    int    numberOfDoFs{0};
    bool   plotAllowed{true};
    bool   inFirstGridSweepOfTimeStep{false};
    bool   inLastGridSweepOfTimeStep{false};

    int startedSteps{0};
    int finishedSteps{0};
    int suspensions{0};

    [[nodiscard]] double getMinTimeStamp() const { return minTimeStamp; }
    [[nodiscard]] double getMaxTimeStamp() const { return maxTimeStamp; }
    [[nodiscard]] double getMinTimeStepSize() const { return minTimeStepSize; }
    [[nodiscard]] double getMaxTimeStepSize() const { return maxTimeStepSize; }
    [[nodiscard]] int    getNumberOfDoFs() const { return numberOfDoFs; }
    [[nodiscard]] bool   mayPlot() const { return plotAllowed; }
    [[nodiscard]] bool   isFirstGridSweepOfTimeStep() const { return inFirstGridSweepOfTimeStep; }
    [[nodiscard]] bool   isLastGridSweepOfTimeStep() const { return inLastGridSweepOfTimeStep; }

    void startGridConstructionStep() { startedSteps++; }
    void startGridInitialisationStep() { startedSteps++; }
    void startDynamicMeshRefinementStep() { startedSteps++; }
    void startTimeStep() { startedSteps++; }
    void startPlottingStep() { startedSteps++; }
    void startSimulation() { startedSteps++; }

    void finishGridConstructionStep() { finishedSteps++; }
    void finishGridInitialisationStep() { finishedSteps++; }
    void finishDynamicMeshRefinementStep() { finishedSteps++; }
    void finishTimeStep() { finishedSteps++; }
    void finishPlottingStep() { finishedSteps++; }
    void finishSimulation() { finishedSteps++; }

    void suspendSolversForOneGridSweep() { suspensions++; }
  };
} // namespace

exahype2::tests::SolverRepositoryTest::SolverRepositoryTest() :
  tarch::tests::TestCase("exahype2::tests::SolverRepositoryTest") {}

void exahype2::tests::SolverRepositoryTest::run() {
  testMethod(testTheRepositoryReducesTheValuesOfEverySolver);
  testMethod(testAPlotNeedsEverySolverAndASweepFlagNeedsOne);
  testMethod(testEverySweepCallReachesEverySolver);
}

/** The time stamps and step sizes of the project are the extrema over its solvers. */
void exahype2::tests::SolverRepositoryTest::testTheRepositoryReducesTheValuesOfEverySolver() {
  RecordingSolver coarse;
  RecordingSolver fine;
  coarse.minTimeStamp    = 0.5;
  coarse.maxTimeStamp    = 0.75;
  coarse.minTimeStepSize = 0.25;
  coarse.maxTimeStepSize = 0.25;
  coarse.numberOfDoFs    = 64;
  fine.minTimeStamp      = 0.25;
  fine.maxTimeStamp      = 0.5;
  fine.minTimeStepSize   = 0.125;
  fine.maxTimeStepSize   = 0.5;
  fine.numberOfDoFs      = 128;

  exahype2::SolverRepository repository{coarse, fine};
  validateNumericalEquals(repository.getMinTimeStamp(), 0.25);
  validateNumericalEquals(repository.getMaxTimeStamp(), 0.75);
  validateNumericalEquals(repository.getMinTimeStepSize(), 0.125);
  validateNumericalEquals(repository.getMaxTimeStepSize(), 0.5);
  validateEquals(repository.getNumberOfDoFs(), 192);
}

/** A plot waits for every solver, while one solver in its first or last sweep names the project's. */
void exahype2::tests::SolverRepositoryTest::testAPlotNeedsEverySolverAndASweepFlagNeedsOne() {
  RecordingSolver            first;
  RecordingSolver            second;
  exahype2::SolverRepository repository{first, second};
  validateEquals(repository.mayPlot(), true);
  validateEquals(repository.isFirstGridSweepOfTimeStep(), false);
  validateEquals(repository.isLastGridSweepOfTimeStep(), false);

  second.plotAllowed = false;
  validateEquals(repository.mayPlot(), false);

  first.inFirstGridSweepOfTimeStep = true;
  second.inLastGridSweepOfTimeStep = true;
  validateEquals(repository.isFirstGridSweepOfTimeStep(), true);
  validateEquals(repository.isLastGridSweepOfTimeStep(), true);
}

/** Every sweep call of the repository reaches every solver exactly once. */
void exahype2::tests::SolverRepositoryTest::testEverySweepCallReachesEverySolver() {
  RecordingSolver            first;
  RecordingSolver            second;
  exahype2::SolverRepository repository{first, second};

  repository.startGridConstructionStep();
  repository.startGridInitialisationStep();
  repository.startDynamicMeshRefinementStep();
  repository.startTimeStep();
  repository.startPlottingStep();
  repository.startSimulation();

  repository.finishGridConstructionStep();
  repository.finishGridInitialisationStep();
  repository.finishDynamicMeshRefinementStep();
  repository.finishTimeStep();
  repository.finishPlottingStep();
  repository.finishSimulation();

  repository.suspendSolversForOneGridSweep();

  validateEquals(first.startedSteps, 6);
  validateEquals(second.startedSteps, 6);
  validateEquals(first.finishedSteps, 6);
  validateEquals(second.finishedSteps, 6);
  validateEquals(first.suspensions, 1);
  validateEquals(second.suspensions, 1);
}
