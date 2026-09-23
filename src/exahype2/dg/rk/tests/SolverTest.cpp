// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/dg/rk/Solver.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"

namespace {
  constexpr int DgOrder          = 3;
  constexpr int RkOrder          = 2;
  constexpr int RkStages         = 2;
  constexpr int NumberOfUnknowns = 2;

  constexpr double CflFactor             = 0.5;
  constexpr double FixedFactor           = 0.25;
  constexpr double TerminalTime          = 10.0;
  constexpr double ReportedTimeStepSize  = 0.1;
  constexpr double ReportedEigenvalue    = 1.0;
  constexpr double ScaledEigenvalue      = 4.0;
  constexpr double CoarseCellSize        = 1.0;
  constexpr double FineCellSize          = 0.5;
  constexpr double FirstSolverTimeStamp  = 5.0;
  constexpr double SecondSolverTimeStamp = 2.0;

  class Adaptive final
    : public exahype2::dg::rk::
        Solver<Adaptive, DgOrder, RkOrder, RkStages, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping> {
  public:
    Adaptive() :
      Solver(CflFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return TerminalTime; }
  };

  class SecondAdaptive final
    : public exahype2::dg::rk::
        Solver<SecondAdaptive, DgOrder, RkOrder, RkStages, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping> {
  public:
    SecondAdaptive() :
      Solver(CflFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return TerminalTime; }
  };

  class Fixed final : public exahype2::dg::rk::
                        Solver<Fixed, DgOrder, RkOrder, RkStages, NumberOfUnknowns, 0, exahype2::FixedTimeStepping> {
  public:
    Fixed() :
      Solver(FixedFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}
  };

  std::string state(const Adaptive& solver) { return Adaptive::toString(solver.getSolverState()); }

  template <typename SolverType>
  void enterTimeStepping(SolverType& solver) {
    solver.startGridInitialisationStep();
    solver.finishGridInitialisationStep();
    solver.startTimeStep();
  }

  template <typename SolverType>
  void addOneCell(SolverType& solver, const double timeStamp, const double cellSize) {
    solver.update(ReportedTimeStepSize, timeStamp, cellSize);
    if constexpr (SolverType::UsesAdaptiveTimeStepping) {
      solver.setMaxEigenvalue(ReportedEigenvalue);
      solver.setMaxScaledEigenvalue(ScaledEigenvalue);
    }
  }

  template <typename SolverType>
  void reportOneCell(SolverType& solver, const double cellSize) {
    addOneCell(solver, 0.0, cellSize);
    solver.reduceLocalUpdates();
  }
} // namespace

exahype2::dg::rk::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::SolverTest") {}

void exahype2::dg::rk::tests::SolverTest::run() {
  testMethod(testStagesAlternateAfterInitialisation);
  testMethod(testMeshRefinementSweepPrecedesTheStage);
  testMethod(testPlotAfterInitialisationReturnsToTheFirstTimeStep);
  testMethod(testSuspendedSweepSchedulesTheFirstStage);
  testMethod(testAdaptiveTimeStepFollowsTheDampedEigenvalue);
  testMethod(testFixedTimeStepFollowsTheSmallestCell);
  testMethod(testSolversOfEqualParametersKeepSeparateStatistics);
  testMethod(testSweepPredicatesFollowTheMarkerAndTheSweep);
}

/** The grid initialisation takes the place of stage zero, so stage one follows it. */
void exahype2::dg::rk::tests::SolverTest::testStagesAlternateAfterInitialisation() {
  Adaptive solver;
  validateEquals(state(solver), std::string("grid-construction"));
  enterTimeStepping(solver);
  validateEquals(state(solver), std::string("time-step-after-grid-initialisation"));
  validateEquals(solver.getRkStage(), -1);
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();

  solver.startTimeStep();
  validateEquals(solver.isRungeKuttaStage(1), true);
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), false);
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();

  solver.startTimeStep();
  validateEquals(solver.getRkStage(), 0);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  validateEquals(solver.getNumberOfDoFs(), peano4::power(DgOrder + 1, DIMENSIONS) * NumberOfUnknowns);
  validateEquals(state(solver), std::string("runge-kutta-step"));
}

/** A dynamic mesh refinement inserts its sweep before the next Runge-Kutta stage. */
void exahype2::dg::rk::tests::SolverTest::testMeshRefinementSweepPrecedesTheStage() {
  Adaptive solver;
  enterTimeStepping(solver);
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();
  solver.startTimeStep();
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();
  validateEquals(solver.usesDynamicMeshRefinementSweeps(), false);

  solver.startDynamicMeshRefinementStep();
  solver.finishDynamicMeshRefinementStep();
  solver.startTimeStep();
  validateEquals(state(solver), std::string("dynamic-mesh-refinement"));
  validateEquals(solver.usesDynamicMeshRefinementSweeps(), true);
  validateEquals(solver.getRkStage(), -1);
  validateEquals(solver.getRkStageAfterDynamicMeshRefinement(), 0);

  solver.startTimeStep();
  validateEquals(solver.getRkStage(), 0);
}

/** A plot after the grid initialisation returns the solver to the first time step. */
void exahype2::dg::rk::tests::SolverTest::testPlotAfterInitialisationReturnsToTheFirstTimeStep() {
  Adaptive solver;
  solver.startGridInitialisationStep();
  validateEquals(solver.mayPlot(), true);
  solver.startPlottingStep();
  validateEquals(state(solver), std::string("plotting-after-grid-initialisation"));
  validateEquals(solver.mayPlot(), true);
  solver.finishPlottingStep();
  solver.startTimeStep();
  validateEquals(state(solver), std::string("time-step-after-grid-initialisation"));
}

/** A sweep the solver sits out is followed by the first stage of a time step. */
void exahype2::dg::rk::tests::SolverTest::testSuspendedSweepSchedulesTheFirstStage() {
  Adaptive solver;
  enterTimeStepping(solver);
  solver.suspendSolversForOneGridSweep();
  validateEquals(state(solver), std::string("suspended"));
  validateEquals(solver.getRkStage(), -1);

  solver.startTimeStep();
  validateEquals(state(solver), std::string("runge-kutta-step"));
  validateEquals(solver.getRkStage(), 0);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
}

/** The admissible time step size follows the maximum eigenvalue damped by the Runge-Kutta and Galerkin orders. */
void exahype2::dg::rk::tests::SolverTest::testAdaptiveTimeStepFollowsTheDampedEigenvalue() {
  Adaptive solver;
  enterTimeStepping(solver);
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();
  solver.startTimeStep();
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();
  validateNumericalEquals(solver.getMaxEigenvalue(), ReportedEigenvalue);

  solver.startTimeStep();
  validateNumericalEquals(solver.getPreviousStageTimeStepSize(), ReportedTimeStepSize);
  const double expected =
    CflFactor * (1.0 / ScaledEigenvalue) * (1.0 / (2.0 * RkOrder - 1.0)) * (1.0 / (DgOrder * DgOrder));
  validateNumericalEquals(solver.getTimeStepSize(), expected);
}

/** The time step size of a solver without a CFL condition follows the width of its smallest cell. */
void exahype2::dg::rk::tests::SolverTest::testFixedTimeStepFollowsTheSmallestCell() {
  Fixed solver;
  enterTimeStepping(solver);
  reportOneCell(solver, CoarseCellSize);
  reportOneCell(solver, FineCellSize);
  solver.finishTimeStep();

  // The two cell sizes make the mesh adaptive, so a mesh-refinement sweep runs before the stage
  // that closes the time step.
  solver.startTimeStep();
  validateEquals(solver.usesDynamicMeshRefinementSweeps(), true);
  solver.finishTimeStep();

  solver.startTimeStep();
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);
  reportOneCell(solver, CoarseCellSize);
  solver.finishTimeStep();

  const double expected = FixedFactor * FineCellSize / RkOrder / RkOrder;
  validateNumericalEquals(solver.getTimeStepSize(), expected);
  validateNumericalEquals(solver.getMinCellSize(), FineCellSize);
}

/** Two solvers that reach the same class template collect their cells in records of their own. */
void exahype2::dg::rk::tests::SolverTest::testSolversOfEqualParametersKeepSeparateStatistics() {
  Adaptive       first;
  SecondAdaptive second;
  enterTimeStepping(first);
  enterTimeStepping(second);

  for (int sweep = 0; sweep < 2; sweep++) {
    addOneCell(first, FirstSolverTimeStamp, CoarseCellSize);
    addOneCell(second, SecondSolverTimeStamp, FineCellSize);
    first.reduceLocalUpdates();
    second.reduceLocalUpdates();
    first.finishTimeStep();
    second.finishTimeStep();
    first.startTimeStep();
    second.startTimeStep();
  }

  validateNumericalEquals(first.getMinTimeStamp(), FirstSolverTimeStamp);
  validateNumericalEquals(first.getMinCellSize(), CoarseCellSize);
  validateNumericalEquals(second.getMinTimeStamp(), SecondSolverTimeStamp);
  validateNumericalEquals(second.getMinCellSize(), FineCellSize);
}

/** Which cells and faces hold their nodes in each sweep, and that a refined one holds none. */
void exahype2::dg::rk::tests::SolverTest::testSweepPredicatesFollowTheMarkerAndTheSweep() {
  peano4::dataview::CellMarker cell;
  peano4::dataview::FaceMarker face;
  face.select(0);

  Adaptive solver;
  validateEquals(solver.storesCellData(cell), false);
  validateEquals(solver.storesFaceData(face), false);
  validateEquals(solver.loadsCellData(cell), false);
  validateEquals(solver.loadsFaceData(face), false);

  solver.startGridInitialisationStep();
  validateEquals(solver.storesCellData(cell), true);
  validateEquals(solver.providesCellDataToKernels(cell), true);
  validateEquals(solver.storesFaceData(face), true);
  validateEquals(solver.loadsCellData(cell), false);
  validateEquals(solver.loadsFaceData(face), false);
  solver.finishGridInitialisationStep();

  solver.startTimeStep();
  validateEquals(solver.loadsCellData(cell), true);
  validateEquals(solver.loadsFaceData(face), true);

  cell.flags = peano4::dataview::CellMarker::WillBeRefined;
  validateEquals(solver.storesCellData(cell), false);
  validateEquals(solver.providesCellDataToKernels(cell), false);

  cell.flags = peano4::dataview::CellMarker::HasBeenRefined;
  validateEquals(solver.loadsCellData(cell), false);

  face.willBeRefinedMask = 1;
  validateEquals(solver.storesFaceData(face), false);

  face.hasBeenRefinedMask = 1;
  validateEquals(solver.loadsFaceData(face), false);
}
