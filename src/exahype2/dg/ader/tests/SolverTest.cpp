// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/LimitingSolverState.h"
#include "exahype2/dg/ader/Solver.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int    Order               = 3;
  constexpr int    NumberOfUnknowns    = 2;
  constexpr double FixedTimeStepFactor = 0.1;
  constexpr double SmallestMeshWidth   = 0.25;
  constexpr double LargestMeshWidth    = 0.5;

  class Adaptive final : public exahype2::dg::ader::
                           Solver<Adaptive, Order, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping, false, 0> {
  public:
    Adaptive() :
      Solver(0.5) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return 10.0; }
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 1.0; }
  };

  class SecondAdaptive final
    : public exahype2::dg::ader::
        Solver<SecondAdaptive, Order, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping, false, 0> {
  public:
    SecondAdaptive() :
      Solver(0.5) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return 10.0; }
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 1.0; }
  };

  class Fixed final
    : public exahype2::dg::ader::Solver<Fixed, Order, NumberOfUnknowns, 0, exahype2::FixedTimeStepping, false, 0> {
  public:
    Fixed() :
      Solver(FixedTimeStepFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 1.0; }
  };

  class Limited final
    : public exahype2::dg::ader::Solver<Limited, Order, NumberOfUnknowns, 0, exahype2::FixedTimeStepping, true, 0> {
  public:
    Limited() :
      Solver(FixedTimeStepFactor) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 1.0; }
  };

  template <typename SolverType>
  std::string state(const SolverType& solver) {
    return SolverType::toString(solver.getSolverState());
  }

  std::string limitingState(const Limited& solver) { return exahype2::dg::toString(solver.getLimitingSolverState()); }

  std::string limitingState(const exahype2::dg::LimitingSolverState state) { return exahype2::dg::toString(state); }

  /**
   * Runs the solver through one sweep in which one cell of the given stamp reports itself.
   */
  template <typename SolverType>
  void sweepWithOneCell(SolverType& solver, const double timeStamp, const bool hasHangingFaces) {
    solver.update(0.1, timeStamp, SmallestMeshWidth, LargestMeshWidth, hasHangingFaces);
    solver.reduceLocalUpdates();
    solver.finishTimeStep();
  }

  /**
   * Runs one sweep of the solver with one cell of the given stamp and hanging-face flag.
   */
  void sweep(Adaptive& solver, const double timeStamp, const bool hasHangingFaces) {
    solver.startTimeStep();
    solver.update(0.1, timeStamp, 1.0, 1.0, hasHangingFaces);
    solver.setMaxEigenvalue(1.0);
    solver.setMaxScaledEigenvalue(2.0);
    solver.reduceLocalUpdates();
    solver.finishTimeStep();
  }
} // namespace

exahype2::dg::ader::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::SolverTest") {}

void exahype2::dg::ader::tests::SolverTest::run() {
  testMethod(testPredictionPrecedesTheTimeStep);
  testMethod(testInitialPredictionSweepKeepsTheTimeStep);
  testMethod(testHangingFacesInsertAPredictionSweep);
  testMethod(testPlotAndSuspensionResumeAtTheRightSweep);
  testMethod(testLimiterStatesMapOntoTheGalerkinStates);
  testMethod(testAdaptiveTimeStepFollowsTheScaledEigenvalue);
  testMethod(testAFixedSolverStepsWithAFractionOfTheSmallestMeshWidth);
  testMethod(testALimitedSolverWalksThroughATroubledTimeStep);
  testMethod(testSolversOfEqualParametersKeepSeparateStatistics);
  testMethod(testSweepPredicatesFollowTheMarkerAndTheSweep);
}

/** Checks that the grid initialisation leads into an initial prediction sweep before the first time-step sweep. */
void exahype2::dg::ader::tests::SolverTest::testPredictionPrecedesTheTimeStep() {
  Adaptive solver;
  validateEquals(state(solver), std::string("GridConstruction"));
  solver.startGridInitialisationStep();
  validateEquals(state(solver), std::string("GridInitialisation"));
  validateEquals(solver.mayPlot(), true);
  solver.finishGridInitialisationStep();

  sweep(solver, 0.0, false);
  validateEquals(state(solver), std::string("InitialPrediction"));
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  validateEquals(solver.isLastGridSweepOfTimeStep(), false);

  sweep(solver, 0.0, false);
  validateEquals(state(solver), std::string("TimeStep"));
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);
  validateEquals(solver.getNumberOfDoFs(), peano4::power(Order + 1, DIMENSIONS) * NumberOfUnknowns);
  validateNumericalEquals(Adaptive::PNPM, exahype2::dg::ader::cflFactor(Order));
}

/** Checks that the initial prediction sweep proposes no time step size, so the next sweep's eigenvalue sets the first
 * one. */
void exahype2::dg::ader::tests::SolverTest::testInitialPredictionSweepKeepsTheTimeStep() {
  Adaptive solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();

  solver.startTimeStep();
  solver.update(0.1, 0.0, 1.0, 1.0, false);
  solver.reduceLocalUpdates();
  solver.finishTimeStep();
  validateEquals(state(solver), std::string("InitialPrediction"));
  validateNumericalEquals(solver.getMaxEigenvalue(), 0.0);
  validateNumericalEquals(solver.getTimeStepSize(), 0.0);
  validateEquals(tarch::hasNonCriticalAssertionBeenViolated(), false);

  sweep(solver, 0.0, false);
  solver.startTimeStep();
  validateNumericalEquals(solver.getTimeStepSize(), 0.5 * Adaptive::PNPM / 2.0);
}

/** Checks that a time step with hanging faces runs a prediction sweep on the hanging cells before the next time step.
 */
void exahype2::dg::ader::tests::SolverTest::testHangingFacesInsertAPredictionSweep() {
  Adaptive solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();
  sweep(solver, 0.0, false);
  sweep(solver, 0.0, true);
  validateEquals(state(solver), std::string("TimeStep"));

  solver.startTimeStep();
  validateEquals(state(solver), std::string("PredictionOnHangingCells"));
  validateEquals(solver.isPredictionOnHangingCellsSweep(), true);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), false);
  validateEquals(solver.reducesStatisticsThisSweep(), false);
  solver.finishTimeStep();
  sweep(solver, 0.1, false);
  validateEquals(state(solver), std::string("TimeStep"));
}

/** Checks that a plot and a suspension resume the solver with the sweep the interruption postponed. */
void exahype2::dg::ader::tests::SolverTest::testPlotAndSuspensionResumeAtTheRightSweep() {
  Adaptive solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();
  sweep(solver, 0.0, false);
  sweep(solver, 0.0, false);

  // At time zero every interruption returns to the initial prediction.
  solver.startPlottingStep();
  validateEquals(state(solver), std::string("Plotting"));
  sweep(solver, 0.0, false);
  validateEquals(state(solver), std::string("InitialPrediction"));
  sweep(solver, 0.5, false);
  validateEquals(state(solver), std::string("TimeStep"));

  solver.suspendSolversForOneGridSweep();
  validateEquals(solver.mayPlot(), true);
  solver.startTimeStep();
  validateEquals(state(solver), std::string("TimeStep"));
}

/** Checks that the limiting sweeps map onto the Galerkin solver states and count the troubled cells. */
void exahype2::dg::ader::tests::SolverTest::testLimiterStatesMapOntoTheGalerkinStates() {
  Limited solver;
  validateEquals(limitingState(solver), exahype2::dg::toString(exahype2::dg::LimitingSolverState::GridConstruction));
  solver.startGridInitialisationStep();
  validateEquals(limitingState(solver), exahype2::dg::toString(exahype2::dg::LimitingSolverState::GridInitialisation));
  validateEquals(state(solver), std::string("GridInitialisation"));
  solver.finishGridInitialisationStep();

  solver.startTimeStep();
  validateEquals(limitingState(solver), exahype2::dg::toString(exahype2::dg::LimitingSolverState::Predict));
  validateEquals(solver.isLimitingSweep(exahype2::dg::LimitingSolverState::Predict), true);
  validateEquals(solver.isLimitingSweep(exahype2::dg::LimitingSolverState::CorrectAndFlag), false);
  validateEquals(state(solver), std::string("InitialPrediction"));
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);

  solver.markCellAsTroubled();
  validateEquals(solver.getNumberOfTroubledCells(), 1L);
  validateEquals(solver.isFinalSubcellSpreadSweep(), false);
}

/** Checks that the admissible time step size of an adaptive solver follows the scaled maximum eigenvalue. */
void exahype2::dg::ader::tests::SolverTest::testAdaptiveTimeStepFollowsTheScaledEigenvalue() {
  Adaptive solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();
  sweep(solver, 0.0, false);
  sweep(solver, 0.0, false);
  validateNumericalEquals(solver.getMaxEigenvalue(), 1.0);

  solver.startTimeStep();
  validateNumericalEquals(solver.getTimeStepSize(), 0.5 * Adaptive::PNPM / 2.0);
}

/** Checks that a fixed-step solver reduces its statistics in both sweeps of a step and steps with a fraction of the
 * smallest mesh width. */
void exahype2::dg::ader::tests::SolverTest::testAFixedSolverStepsWithAFractionOfTheSmallestMeshWidth() {
  Fixed solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();

  solver.startTimeStep();
  validateEquals(state(solver), std::string("InitialPrediction"));
  validateEquals(solver.reducesStatisticsThisSweep(), true);
  sweepWithOneCell(solver, 0.0, false);

  solver.startTimeStep();
  validateEquals(state(solver), std::string("TimeStep"));
  validateEquals(solver.reducesStatisticsThisSweep(), true);
  sweepWithOneCell(solver, 0.0, false);
  validateNumericalEquals(solver.getTimeStepSize(), FixedTimeStepFactor * SmallestMeshWidth);
}

/** Checks that a troubled time step runs the corrector, every spreading ring and the recompute, and closes with the
 * stamps of the corrector. */
void exahype2::dg::ader::tests::SolverTest::testALimitedSolverWalksThroughATroubledTimeStep() {
  Limited solver;
  solver.startGridInitialisationStep();
  solver.finishGridInitialisationStep();

  solver.startTimeStep();
  validateEquals(state(solver), std::string("InitialPrediction"));
  validateEquals(solver.reducesStatisticsThisSweep(), true);
  sweepWithOneCell(solver, 0.3, true);

  solver.startTimeStep();
  validateEquals(limitingState(solver), limitingState(exahype2::dg::LimitingSolverState::PredictOnHangingCells));
  validateEquals(state(solver), std::string("PredictionOnHangingCells"));
  validateEquals(solver.reducesStatisticsThisSweep(), false);
  solver.finishTimeStep();

  solver.startTimeStep();
  validateEquals(limitingState(solver), limitingState(exahype2::dg::LimitingSolverState::CorrectAndFlag));
  validateEquals(state(solver), std::string("TimeStep"));
  solver.markCellAsTroubled();
  sweepWithOneCell(solver, 0.5, false);
  validateEquals(
    solver.getNumberOfTroubledCells(),
    static_cast<long>(tarch::mpi::Rank::getInstance().getNumberOfRanks())
  );
  validateEquals(solver.isLastGridSweepOfTimeStep(), false);

  for (int ring = 0; ring < exahype2::dg::NumberOfSpreadRings; ring++) {
    solver.startTimeStep();
    validateEquals(limitingState(solver), limitingState(exahype2::dg::LimitingSolverState::SpreadDomain));
    validateEquals(state(solver), std::string("TimeStep"));
    validateEquals(solver.isFinalSubcellSpreadSweep(), (ring + 1 == exahype2::dg::NumberOfSpreadRings));
    validateEquals(solver.reducesStatisticsThisSweep(), false);
    solver.finishTimeStep();
  }

  solver.startTimeStep();
  validateEquals(limitingState(solver), limitingState(exahype2::dg::LimitingSolverState::RecomputeSubcells));
  validateEquals(state(solver), std::string("TimeStep"));
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);
  validateEquals(solver.reducesStatisticsThisSweep(), true);
  solver.finishTimeStep();
  validateNumericalEquals(solver.getMinTimeStamp(), 0.5);
  validateNumericalEquals(solver.getTimeStepSize(), FixedTimeStepFactor * SmallestMeshWidth);

  solver.startPlottingStep();
  validateEquals(state(solver), std::string("Plotting"));
  solver.suspendSolversForOneGridSweep();
  validateEquals(state(solver), std::string("Suspended"));
}

/** Which cells and faces hold their nodes in each sweep, and that a refined one holds none. */
void exahype2::dg::ader::tests::SolverTest::testSweepPredicatesFollowTheMarkerAndTheSweep() {
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
  validateEquals(solver.isPredictionOnHangingCellsSweep(), false);
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

/** Checks that two solvers that reach the same class template collect their cells in records of their own. */
void exahype2::dg::ader::tests::SolverTest::testSolversOfEqualParametersKeepSeparateStatistics() {
  constexpr double FirstSolverTimeStamp  = 5.0;
  constexpr double SecondSolverTimeStamp = 2.0;

  const auto advanceOneSweep = [](auto& solver, const double timeStamp, const double cellSize) {
    solver.startTimeStep();
    solver.update(0.1, timeStamp, cellSize, cellSize, false);
    solver.setMaxEigenvalue(1.0);
    solver.setMaxScaledEigenvalue(2.0);
    solver.reduceLocalUpdates();
    solver.finishTimeStep();
  };

  Adaptive       first;
  SecondAdaptive second;
  first.startGridInitialisationStep();
  first.finishGridInitialisationStep();
  second.startGridInitialisationStep();
  second.finishGridInitialisationStep();

  for (int sweep = 0; sweep < 2; sweep++) {
    advanceOneSweep(first, FirstSolverTimeStamp, LargestMeshWidth);
    advanceOneSweep(second, SecondSolverTimeStamp, SmallestMeshWidth);
  }

  validateNumericalEquals(first.getMinTimeStamp(), FirstSolverTimeStamp);
  validateNumericalEquals(first.getMinCellSize(), LargestMeshWidth);
  validateNumericalEquals(second.getMinTimeStamp(), SecondSolverTimeStamp);
  validateNumericalEquals(second.getMinCellSize(), SmallestMeshWidth);
}
