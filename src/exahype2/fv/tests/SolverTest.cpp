// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SolverTest.h"

#include "exahype2/fv/Solver.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"

namespace {
  constexpr int PatchSize        = 4;
  constexpr int NumberOfUnknowns = 2;

  class Adaptive final
    : public exahype2::fv::Solver<Adaptive, PatchSize, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping> {
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
    : public exahype2::fv::Solver<SecondAdaptive, PatchSize, NumberOfUnknowns, 0, exahype2::AdaptiveTimeStepping> {
  public:
    SecondAdaptive() :
      Solver(0.5) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double minTerminalTime() const override { return 10.0; }
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 1.0; }
  };

  class Fixed final : public exahype2::fv::Solver<Fixed, PatchSize, NumberOfUnknowns, 0, exahype2::FixedTimeStepping> {
  public:
    Fixed() :
      Solver(0.1) {}

    void startSimulation() override {}
    void finishSimulation() override {}

  protected:
    [[nodiscard]] double maxAdmissibleMeshWidth() const override { return 2.0; }
  };

  template <typename SolverType>
  std::string state(const SolverType& solver) {
    return SolverType::toString(solver.getSolverState());
  }

  template <typename SolverType>
  void enterTimeStepping(SolverType& solver) {
    solver.startGridInitialisationStep();
    solver.finishGridInitialisationStep();
    solver.startTimeStep();
  }
} // namespace

exahype2::fv::tests::SolverTest::SolverTest() :
  tarch::tests::TestCase("exahype2::fv::tests::SolverTest") {}

void exahype2::fv::tests::SolverTest::run() {
  testMethod(testStatesFollowTheSweeps);
  testMethod(testUpdateFeedsTheStatistics);
  testMethod(testAdaptiveTimeStepFollowsTheEigenvalue);
  testMethod(testFixedTimeStepScalesWithTheMeshWidth);
  testMethod(testSolversOfEqualParametersKeepSeparateStatistics);
  testMethod(testSweepPredicatesFollowTheMarkerAndTheSweep);
}

/** Checks that the solver state follows the grid construction, initialisation, time step, suspension and plot. */
void exahype2::fv::tests::SolverTest::testStatesFollowTheSweeps() {
  Adaptive solver;
  validateEquals(state(solver), std::string("GridConstruction"));
  solver.startGridConstructionStep();
  solver.finishGridConstructionStep();

  solver.startGridInitialisationStep();
  validateEquals(state(solver), std::string("GridInitialisation"));
  validateEquals(solver.mayPlot(), true);
  validateEquals(solver.hasStoredFaceData(), false);
  solver.finishGridInitialisationStep();
  validateEquals(solver.hasStoredFaceData(), true);

  solver.startTimeStep();
  validateEquals(state(solver), std::string("TimeStep"));
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);

  solver.suspendSolversForOneGridSweep();
  validateEquals(state(solver), std::string("Suspended"));
  solver.startTimeStep();
  validateEquals(state(solver), std::string("TimeStep"));

  solver.startPlottingStep();
  validateEquals(state(solver), std::string("Plotting"));
}

/** Checks that the cell updates of a sweep reach the patch size, time step size and time stamp statistics. */
void exahype2::fv::tests::SolverTest::testUpdateFeedsTheStatistics() {
  Adaptive solver;
  enterTimeStepping(solver);
  solver.update(0.1, 0.0, 1.0, 1.0);
  solver.updateTimeStampAndPatchSize(0.0, 0.5, 1.0);
  solver.setMaxEigenvalue(1.0);
  solver.reduceLocalUpdates();
  solver.finishTimeStep();

  validateNumericalEquals(solver.getMinPatchSize(), 0.5);
  validateNumericalEquals(solver.getMaxPatchSize(), 1.0);
  validateNumericalEquals(solver.getMinTimeStepSize(), 0.1);
  validateNumericalEquals(solver.getMinTimeStamp(), 0.0);
  validateEquals(solver.getNumberOfDoFs(), peano4::power(PatchSize, DIMENSIONS) * NumberOfUnknowns);
}

/** Checks that the admissible time step size of an adaptive solver follows the reduced maximum eigenvalue. */
void exahype2::fv::tests::SolverTest::testAdaptiveTimeStepFollowsTheEigenvalue() {
  Adaptive solver;
  enterTimeStepping(solver);
  solver.update(0.1, 0.0, 1.0, 1.0);
  solver.setMaxEigenvalue(2.0);
  solver.reduceLocalUpdates();
  solver.finishTimeStep();
  validateNumericalEquals(solver.getMaxEigenvalue(), 2.0);

  // The proposal of one time step is the admissible size of the next one.
  solver.startTimeStep();
  validateNumericalEquals(solver.getTimeStepSize(), 0.5 * (1.0 / PatchSize) / (DIMENSIONS * 2.0));
}

/** Checks that the time step size of a fixed solver scales with the smallest mesh width. */
void exahype2::fv::tests::SolverTest::testFixedTimeStepScalesWithTheMeshWidth() {
  Fixed solver;
  enterTimeStepping(solver);
  solver.update(0.0, 0.0, 1.0, 1.0);
  solver.reduceLocalUpdates();
  solver.finishTimeStep();
  validateNumericalEquals(solver.getTimeStepSize(), 0.1 * (1.0 / PatchSize) / 2.0);
}

/** Checks that two solvers that reach the same class template collect their cells in records of their own. */
void exahype2::fv::tests::SolverTest::testSolversOfEqualParametersKeepSeparateStatistics() {
  constexpr double FirstSolverTimeStamp  = 5.0;
  constexpr double SecondSolverTimeStamp = 2.0;
  constexpr double CoarsePatchSize       = 1.0;
  constexpr double FinePatchSize         = 0.5;

  Adaptive       first;
  SecondAdaptive second;
  enterTimeStepping(first);
  enterTimeStepping(second);

  first.update(0.1, FirstSolverTimeStamp, CoarsePatchSize, CoarsePatchSize);
  second.update(0.1, SecondSolverTimeStamp, FinePatchSize, FinePatchSize);
  first.setMaxEigenvalue(1.0);
  second.setMaxEigenvalue(1.0);
  first.reduceLocalUpdates();
  second.reduceLocalUpdates();
  first.finishTimeStep();
  second.finishTimeStep();

  validateNumericalEquals(first.getMinTimeStamp(), FirstSolverTimeStamp);
  validateNumericalEquals(first.getMinPatchSize(), CoarsePatchSize);
  validateNumericalEquals(second.getMinTimeStamp(), SecondSolverTimeStamp);
  validateNumericalEquals(second.getMinPatchSize(), FinePatchSize);
}

/** Checks which cells and faces hold a patch in each sweep, and that a refined one holds none. */
void exahype2::fv::tests::SolverTest::testSweepPredicatesFollowTheMarkerAndTheSweep() {
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
