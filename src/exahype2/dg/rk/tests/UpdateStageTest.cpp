// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "UpdateStageTest.h"

#include "exahype2/dg/rk/StageScratch.h"
#include "exahype2/dg/rk/UpdateStage.h"
#include "exahype2/dg/rk/tests/StageFixtures.h"

namespace {
  using AdaptiveTestSolver = exahype2::dg::rk::tests::AdaptiveTestSolver;
  using ButcherTableau     = exahype2::dg::rk::tests::ButcherTableau;
  using CellLabel          = exahype2::dg::rk::tests::CellLabel;
  using DGMatrices         = exahype2::dg::rk::tests::DGMatrices;
  using FaceLabel          = exahype2::dg::rk::tests::FaceLabel;
  using Quadrature         = exahype2::dg::rk::tests::Quadrature;
  using StageTerms         = exahype2::dg::rk::tests::StageTerms;
  using TestSolver         = exahype2::dg::rk::tests::TestSolver;

  constexpr int FluxElementsPerFace  = exahype2::dg::rk::tests::FluxElementsPerFace;
  constexpr int NodesPerCell         = exahype2::dg::rk::tests::NodesPerCell;
  constexpr int NodesPerFace         = exahype2::dg::rk::tests::NodesPerFace;
  constexpr int NumberOfData         = exahype2::dg::rk::tests::NumberOfData;
  constexpr int RkStages             = exahype2::dg::rk::tests::RkStages;
  constexpr int StateElementsPerFace = exahype2::dg::rk::tests::StateElementsPerFace;
  constexpr int NumberOfUnknowns     = exahype2::dg::rk::tests::NumberOfUnknowns;

  constexpr double MaxEigenvalue = exahype2::dg::rk::tests::MaxEigenvalue;

  using FaceState = exahype2::dg::rk::tests::ValuePatch<2 * StateElementsPerFace>;
  using FaceFlux  = exahype2::dg::rk::tests::ValuePatch<2 * FluxElementsPerFace>;

  constexpr double CellTimeStamp    = 0.5;
  constexpr double CellTimeStepSize = 0.125;
  constexpr double LowerFaceFlux    = 2.0;
  constexpr double UpperFaceFlux    = 3.0;

  struct HookRecorder final {
    int           stageCalls{0};
    int           timeStepCalls{0};
    const double* stageState{nullptr};
    const double* stageUpdate{nullptr};
    const double* combinedState{nullptr};
    double        combinedTimeStamp{0.0};
  };

  struct CellBuffers final {
    double cellState[NodesPerCell * NumberOfData]{};
    double rightHandSideEstimates[RkStages * NodesPerCell * NumberOfUnknowns]{};
  };

  void fillCell(CellBuffers& buffers, const double filler) {
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      buffers.cellState[cellNode * NumberOfData] = 1.0 + cellNode;
    }
    for (int entry = 0; entry < RkStages * NodesPerCell * NumberOfUnknowns; entry++) {
      buffers.rightHandSideEstimates[entry] = filler;
    }
  }

  //! Returns the buffer the kernel combines into, which still holds the values of its last call.
  const double* stageStateOfTheLastCall() {
    return exahype2::dg::rk::stageScratch<NodesPerCell * NumberOfData, double>();
  }

  struct FaceBuffers final {
    FaceLabel labels[peano4::FacesPerCell]{};
    FaceState states[peano4::FacesPerCell]{};
    FaceFlux  fluxes[peano4::FacesPerCell]{};
  };

  void fillFaces(FaceBuffers& faces, const double stateFiller, const double fluxFiller) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faces.states[face].fill(stateFiller);
      faces.fluxes[face].fill(fluxFiller);
    }
  }

  void setFaceFluxesTheCellReads(FaceBuffers& faces) {
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int entry = 0; entry < FluxElementsPerFace; entry++) {
        faces.fluxes[direction].data()[FluxElementsPerFace + entry] = LowerFaceFlux;
        faces.fluxes[direction + DIMENSIONS].data()[entry]          = UpperFaceFlux;
      }
    }
  }

  double expectedFaceIntegral(const int cellNode, const double timeStepSize) {
    const auto cellSize  = exahype2::dg::rk::tests::makeCellSize();
    const auto nodeIndex = exahype2::dg::rk::tests::nodeIndexOf(cellNode);
    double     expected  = 0.0;
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      const double flux = nodeIndex(direction) == 0 ? LowerFaceFlux : -UpperFaceFlux;
      expected += timeStepSize / cellSize(direction) * flux;
    }
    return expected;
  }

  //! Returns the wave speed of one cell divided by its extents, which sets the admissible step.
  double scaledEigenvalueOfTheCell() {
    const auto cellSize = exahype2::dg::rk::tests::makeCellSize();
    double     result   = 0.0;
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      result += MaxEigenvalue / cellSize(direction);
    }
    return result;
  }

  template <typename SolverType>
  void runUpdateStage(
    SolverType&   solver,
    CellLabel&    cellLabel,
    FaceBuffers&  faces,
    CellBuffers&  buffers,
    HookRecorder& recorder
  ) {
    const auto faceLabels = exahype2::dg::rk::tests::makeFaceEnumerator(faces.labels);
    const auto faceStates = exahype2::dg::rk::tests::makeFaceEnumerator(faces.states);
    const auto faceFluxes = exahype2::dg::rk::tests::makeFaceEnumerator(faces.fluxes);
    exahype2::dg::rk::
      updateStage<SolverType, false, false, false, false, false, Quadrature, DGMatrices, ButcherTableau>(
        exahype2::dg::rk::tests::makeCellMarker(),
        solver,
        cellLabel,
        faceLabels,
        faceStates,
        faceFluxes,
        buffers.cellState,
        buffers.rightHandSideEstimates,
        [&](const auto* const oldQ, auto* const dQdt) {
          recorder.stageCalls++;
          recorder.stageState  = oldQ;
          recorder.stageUpdate = dQdt;
        },
        [&](auto* const newQ, const double timeStamp) {
          recorder.timeStepCalls++;
          recorder.combinedState     = newQ;
          recorder.combinedTimeStamp = timeStamp;
        }
      );
  }

  template <bool IsLinear, bool UseRiemannSolver>
  void runFaceKernel(
    const peano4::dataview::FaceMarker& marker,
    const TestSolver&                   solver,
    FaceLabel&                          faceLabel,
    FaceState&                          faceState,
    FaceFlux&                           faceFlux
  ) {
    exahype2::dg::rk::applyBoundaryConditionsAndSolveRiemannProblem<
      TestSolver,
      IsLinear,
      UseRiemannSolver,
      true,
      false,
      false,
      Quadrature>(marker, solver, faceLabel, faceState.data(), faceFlux.data());
  }

  //! Fills the interior half of a face with a ramp and the exterior half with zeros.
  void fillBoundaryFace(const int faceNumber, FaceState& faceState, FaceFlux& faceFlux) {
    const int insideSide  = faceNumber < DIMENSIONS ? 1 : 0;
    const int outsideSide = faceNumber < DIMENSIONS ? 0 : 1;
    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      faceState.data()[outsideSide * StateElementsPerFace + faceNode * NumberOfData]   = 0.0;
      faceState.data()[insideSide * StateElementsPerFace + faceNode * NumberOfData]    = 1.0 + faceNode;
      faceFlux.data()[outsideSide * FluxElementsPerFace + faceNode * NumberOfUnknowns] = 0.0;
      faceFlux.data()[insideSide * FluxElementsPerFace + faceNode * NumberOfUnknowns]  = 0.5 + faceNode;
    }
  }
} // namespace

exahype2::dg::rk::tests::UpdateStageTest::UpdateStageTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::UpdateStageTest") {}

void exahype2::dg::rk::tests::UpdateStageTest::run() {
  testMethod(testSweepAfterGridInitialisationOpensTheFirstStage);
  testMethod(testStageAddsTheFaceIntegralToThePrecedingEstimate);
  testMethod(testFirstSweepCombinesTheEstimatesAndStampsTheCell);
  testMethod(testSweepAfterGridInitialisationReportsTheEigenvalueOfTheCell);
  testMethod(testFirstSweepOfAnAdaptiveSolverTakesTheAdmissibleTimeStepSize);
  testMethod(testBoundaryFaceFillsTheExteriorTraceAndMirrorsTheTimeStamps);
  testMethod(testUpperBoundaryFaceCopiesTheTimeStampsOntoTheExteriorHalf);
  testMethod(testBoundaryFaceOfARefinedCellKeepsItsTraceAndFlux);
  testMethod(testPeriodicFaceKeepsBothTracesAndTimeStamps);
  testMethod(testInteriorFaceKeepsTheTimeStampsAndSolvesTheRiemannProblem);
  testMethod(testUserRiemannSolverReceivesTheFaceAndWritesBothFluxes);
  testMethod(testLinearFaceFluxesFollowTheStateJump);
  testMethod(testTimeStampsRollOverOnTheFirstSweepOnly);
}

/**
 * The sweep after the grid initialisation combines no completed stage, writes the solver's time
 * step size into the cell and projects the first stage onto the faces.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testSweepAfterGridInitialisationOpensTheFirstStage() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  solver.setTimeStepSize(CellTimeStepSize);
  validateEquals(solver.getRkStage(), -1);

  constexpr double StateFiller = 9.0;
  constexpr double FluxFiller  = 5.0;
  CellBuffers      buffers;
  FaceBuffers      faces;
  HookRecorder     recorder;
  CellLabel        cellLabel;
  fillCell(buffers, 7.0);
  fillFaces(faces, StateFiller, FluxFiller);
  cellLabel.setTimeStamp(CellTimeStamp);

  runUpdateStage(solver, cellLabel, faces, buffers, recorder);

  validateEquals(recorder.stageCalls, 0);
  validateEquals(recorder.timeStepCalls, 0);
  validateNumericalEquals(cellLabel.getTimeStepSize(), CellTimeStepSize);
  validateNumericalEquals(cellLabel.getTimeStamp(), CellTimeStamp);
  validateEquals(cellLabel.getHasUpdated(), false);

  // The first row of the tableau vanishes, so the stage state repeats the cell state, and the
  // volume integral of a solver without terms clears the first stage's estimate.
  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      stageStateOfTheLastCall()[cellNode * NumberOfData],
      buffers.cellState[cellNode * NumberOfData],
      cellNode
    );
    validateNumericalEqualsWithParams1(buffers.rightHandSideEstimates[cellNode * NumberOfUnknowns], 0.0, cellNode);
    validateNumericalEqualsWithParams1(
      buffers.rightHandSideEstimates[NodesPerCell * NumberOfUnknowns + cellNode * NumberOfUnknowns],
      7.0,
      cellNode
    );
  }

  const double stageTimeStamp = CellTimeStamp + ButcherTableau::RelativeTimeStepSizes[0] * CellTimeStepSize;
  for (int direction = 0; direction < DIMENSIONS; direction++) {
    validateEquals(faces.labels[direction].getUpdated(1), true);
    validateEquals(faces.labels[direction].getUpdated(0), false);
    validateEquals(faces.labels[direction + DIMENSIONS].getUpdated(0), true);
    validateNumericalEquals(faces.labels[direction].getUpdatedTimeStamp(1), stageTimeStamp);
    validateNumericalEquals(faces.labels[direction + DIMENSIONS].getUpdatedTimeStamp(0), stageTimeStamp);
    validateEquals(faces.labels[direction].getRequiresDataTransfer(1), false);
    validateEquals(faces.labels[direction].getRequiresDataTransfer(0), true);

    for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
      const int lowerNode = exahype2::dg::rk::tests::cellNodeOnFaceLine(direction, faceNode, 0);
      const int upperNode = exahype2::dg::rk::tests::cellNodeOnFaceLine(
        direction,
        faceNode,
        exahype2::dg::rk::tests::NumberOfDofsPerAxis - 1
      );
      validateNumericalEqualsWithParams2(
        faces.states[direction].data()[StateElementsPerFace + faceNode * NumberOfData],
        buffers.cellState[lowerNode * NumberOfData],
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        faces.states[direction + DIMENSIONS].data()[faceNode * NumberOfData],
        buffers.cellState[upperNode * NumberOfData],
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        faces.states[direction].data()[faceNode * NumberOfData],
        StateFiller,
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        faces.fluxes[direction].data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
        0.0,
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        faces.fluxes[direction].data()[faceNode * NumberOfUnknowns],
        FluxFiller,
        direction,
        faceNode
      );
    }
  }
  solver.reduceLocalUpdates();
}

/**
 * A stage that is not the first one of a time step adds the face integrals to the estimate of
 * the preceding stage and leaves the cell label untouched.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testStageAddsTheFaceIntegralToThePrecedingEstimate() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.getRkStage(), 1);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), false);

  CellBuffers  buffers;
  FaceBuffers  faces;
  HookRecorder recorder;
  CellLabel    cellLabel;
  fillCell(buffers, 0.0);
  fillFaces(faces, 0.0, 0.0);
  setFaceFluxesTheCellReads(faces);
  cellLabel.setTimeStamp(CellTimeStamp);
  cellLabel.setTimeStepSize(CellTimeStepSize);

  runUpdateStage(solver, cellLabel, faces, buffers, recorder);

  validateEquals(recorder.stageCalls, 1);
  validateEquals(recorder.timeStepCalls, 0);
  validateEquals(recorder.stageState, static_cast<const double*>(buffers.cellState));
  validateEquals(recorder.stageUpdate, static_cast<const double*>(buffers.rightHandSideEstimates));
  validateNumericalEquals(cellLabel.getTimeStamp(), CellTimeStamp);
  validateEquals(cellLabel.getHasUpdated(), false);

  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    const double expected = expectedFaceIntegral(cellNode, CellTimeStepSize);
    validateNumericalEqualsWithParams1(buffers.rightHandSideEstimates[cellNode * NumberOfUnknowns], expected, cellNode);
    // The second row of the tableau weights the preceding stage with one.
    validateNumericalEqualsWithParams1(
      stageStateOfTheLastCall()[cellNode * NumberOfData],
      buffers.cellState[cellNode * NumberOfData] + expected,
      cellNode
    );
    validateNumericalEqualsWithParams1(
      buffers.rightHandSideEstimates[NodesPerCell * NumberOfUnknowns + cellNode * NumberOfUnknowns],
      0.0,
      cellNode
    );
  }

  const double stageTimeStamp = CellTimeStamp + ButcherTableau::RelativeTimeStepSizes[1] * CellTimeStepSize;
  validateNumericalEquals(faces.labels[0].getUpdatedTimeStamp(1), stageTimeStamp);
  solver.reduceLocalUpdates();
}

/**
 * The first sweep of a time step combines the estimates into the solution, advances the cell's
 * time stamp by the preceding step size and takes the solver's next step size.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testFirstSweepCombinesTheEstimatesAndStampsTheCell() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.getRkStage(), 0);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);

  constexpr double FirstEstimate = 4.0;
  CellBuffers      buffers;
  FaceBuffers      faces;
  HookRecorder     recorder;
  CellLabel        cellLabel;
  fillCell(buffers, 0.0);
  for (int entry = 0; entry < NodesPerCell * NumberOfUnknowns; entry++) {
    buffers.rightHandSideEstimates[entry] = FirstEstimate;
  }
  fillFaces(faces, 0.0, 0.0);
  setFaceFluxesTheCellReads(faces);
  cellLabel.setTimeStamp(CellTimeStamp);
  cellLabel.setTimeStepSize(CellTimeStepSize);

  double solutionBeforeUpdate[NodesPerCell * NumberOfData];
  for (int entry = 0; entry < NodesPerCell * NumberOfData; entry++) {
    solutionBeforeUpdate[entry] = buffers.cellState[entry];
  }
  const double nextTimeStepSize = solver.getTimeStepSize();

  runUpdateStage(solver, cellLabel, faces, buffers, recorder);

  validateEquals(recorder.stageCalls, 1);
  validateEquals(recorder.timeStepCalls, 1);
  validateEquals(
    recorder.stageUpdate,
    static_cast<const double*>(buffers.rightHandSideEstimates + NodesPerCell * NumberOfUnknowns)
  );
  validateEquals(recorder.combinedState, static_cast<const double*>(buffers.cellState));
  validateNumericalEquals(recorder.combinedTimeStamp, CellTimeStamp);

  validateNumericalEquals(cellLabel.getTimeStamp(), CellTimeStamp + CellTimeStepSize);
  validateNumericalEquals(cellLabel.getTimeStepSize(), nextTimeStepSize);
  validateEquals(cellLabel.getHasUpdated(), true);

  for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
    // The final weights take the first estimate alone.
    const double expectedSolution = solutionBeforeUpdate[cellNode * NumberOfData] + FirstEstimate;
    validateNumericalEqualsWithParams1(buffers.cellState[cellNode * NumberOfData], expectedSolution, cellNode);
    validateNumericalEqualsWithParams1(stageStateOfTheLastCall()[cellNode * NumberOfData], expectedSolution, cellNode);
    validateNumericalEqualsWithParams1(
      buffers.rightHandSideEstimates[NodesPerCell * NumberOfUnknowns + cellNode * NumberOfUnknowns],
      expectedFaceIntegral(cellNode, CellTimeStepSize),
      cellNode
    );
    validateNumericalEqualsWithParams1(buffers.rightHandSideEstimates[cellNode * NumberOfUnknowns], 0.0, cellNode);
  }
  solver.reduceLocalUpdates();
}

/**
 * An adaptive solver takes the wave speed of the cell from the sweep after the grid
 * initialisation and derives the admissible time step size of the first time step from it.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testSweepAfterGridInitialisationReportsTheEigenvalueOfTheCell() {
  AdaptiveTestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);

  CellBuffers  buffers;
  FaceBuffers  faces;
  HookRecorder recorder;
  CellLabel    cellLabel;
  fillCell(buffers, 0.0);
  fillFaces(faces, 0.0, 0.0);
  cellLabel.setTimeStamp(CellTimeStamp);

  runUpdateStage(solver, cellLabel, faces, buffers, recorder);

  validateNumericalEquals(cellLabel.getTimeStepSize(), solver.getTimeStepSize());

  solver.reduceLocalUpdates();
  solver.finishTimeStep();
  solver.startTimeStep();
  validateEquals(solver.isLastGridSweepOfTimeStep(), true);
  solver.finishTimeStep();

  // The closing sweep proposes the size, and the first sweep of the next time step takes it.
  solver.startTimeStep();

  validateNumericalEquals(solver.getMaxEigenvalue(), MaxEigenvalue);
  const double expected = (1.0 / scaledEigenvalueOfTheCell()) * (1.0 / (2.0 * exahype2::dg::rk::tests::RkOrder - 1.0))
    * (1.0 / (exahype2::dg::rk::tests::DgOrder * exahype2::dg::rk::tests::DgOrder));
  validateNumericalEquals(solver.getTimeStepSize(), expected);
}

/**
 * The first sweep of a time step of an adaptive solver stamps the cell with the admissible time
 * step size of the closed time step.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testFirstSweepOfAnAdaptiveSolverTakesTheAdmissibleTimeStepSize() {
  AdaptiveTestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  validateTrue(solver.getTimeStepSize() > 0.0);

  CellBuffers  buffers;
  FaceBuffers  faces;
  HookRecorder recorder;
  CellLabel    cellLabel;
  fillCell(buffers, 0.0);
  fillFaces(faces, 0.0, 0.0);
  setFaceFluxesTheCellReads(faces);
  cellLabel.setTimeStamp(CellTimeStamp);
  cellLabel.setTimeStepSize(CellTimeStepSize);

  const double admissibleTimeStepSize = solver.getTimeStepSize();

  runUpdateStage(solver, cellLabel, faces, buffers, recorder);

  validateEquals(recorder.timeStepCalls, 1);
  validateNumericalEquals(cellLabel.getTimeStamp(), CellTimeStamp + CellTimeStepSize);
  validateNumericalEquals(cellLabel.getTimeStepSize(), admissibleTimeStepSize);
  validateEquals(cellLabel.getHasUpdated(), true);
  solver.reduceLocalUpdates();
}

/**
 * A boundary face fills its exterior trace through the solver's boundary condition, copies the
 * interior time stamps onto the exterior half and solves the Riemann problem of both traces.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testBoundaryFaceFillsTheExteriorTraceAndMirrorsTheTimeStamps() {
  constexpr double InteriorTimeStamp = 2.0;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  FaceLabel faceLabel;
  faceLabel.setBoundary(true);
  faceLabel.setUpdatedTimeStamp(0, 1.0);
  faceLabel.setUpdatedTimeStamp(1, InteriorTimeStamp);
  faceLabel.setNewTimeStamp(0, 3.0);
  faceLabel.setNewTimeStamp(1, 4.0);
  faceLabel.setOldTimeStamp(0, 5.0);
  faceLabel.setOldTimeStamp(1, 6.0);

  FaceState faceState;
  FaceFlux  faceFlux;
  fillBoundaryFace(0, faceState, faceFlux);

  runFaceKernel<false, false>(exahype2::dg::rk::tests::makeFaceMarker(0), solver, faceLabel, faceState, faceFlux);

  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(0), InteriorTimeStamp);
  validateNumericalEquals(faceLabel.getNewTimeStamp(0), 4.0);
  validateNumericalEquals(faceLabel.getOldTimeStamp(0), 6.0);
  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(1), InteriorTimeStamp);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double interior = 1.0 + faceNode;
    const double exterior = 2.0 * interior;
    validateNumericalEqualsWithParams1(faceState.data()[faceNode * NumberOfData], exterior, faceNode);

    // The flux along the first axis repeats the state and adds the time stamp of the face, and
    // the Rusanov solver averages the two fluxes and adds the jump of the two traces.
    const double exteriorFlux = exterior + InteriorTimeStamp;
    const double expected     = 0.5 * (exteriorFlux + (0.5 + faceNode) + MaxEigenvalue * (exterior - interior));
    validateNumericalEqualsWithParams1(faceFlux.data()[faceNode * NumberOfUnknowns], expected, faceNode);
    validateNumericalEqualsWithParams1(
      faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      expected,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * The upper boundary face of an axis copies the time stamps of its interior half, which is the
 * first one, onto its exterior half.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testUpperBoundaryFaceCopiesTheTimeStampsOntoTheExteriorHalf() {
  constexpr int UpperFace = DIMENSIONS;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  FaceLabel faceLabel;
  faceLabel.setBoundary(true);
  faceLabel.setUpdatedTimeStamp(0, 3.0);
  faceLabel.setUpdatedTimeStamp(1, 7.0);
  faceLabel.setNewTimeStamp(0, 1.0);
  faceLabel.setNewTimeStamp(1, 5.0);
  faceLabel.setOldTimeStamp(0, 2.0);
  faceLabel.setOldTimeStamp(1, 6.0);

  FaceState faceState;
  FaceFlux  faceFlux;
  fillBoundaryFace(UpperFace, faceState, faceFlux);

  runFaceKernel<false, false>(
    exahype2::dg::rk::tests::makeFaceMarker(UpperFace),
    solver,
    faceLabel,
    faceState,
    faceFlux
  );

  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(1), 3.0);
  validateNumericalEquals(faceLabel.getNewTimeStamp(1), 1.0);
  validateNumericalEquals(faceLabel.getOldTimeStamp(1), 2.0);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double interior = 1.0 + faceNode;
    validateNumericalEqualsWithParams1(
      faceState.data()[StateElementsPerFace + faceNode * NumberOfData],
      2.0 * interior,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * The boundary face of a refined cell keeps its trace and its flux, because the boundary faces
 * of the children carry the data of the domain boundary.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testBoundaryFaceOfARefinedCellKeepsItsTraceAndFlux() {
  constexpr double StateFiller = 7.0;
  constexpr double FluxFiller  = 3.0;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  for (int refinedCase = 0; refinedCase < 2; refinedCase++) {
    FaceLabel faceLabel;
    faceLabel.setBoundary(true);
    faceLabel.setUpdatedTimeStamp(0, 1.0);
    faceLabel.setUpdatedTimeStamp(1, 2.0);

    FaceState faceState;
    FaceFlux  faceFlux;
    faceState.fill(StateFiller);
    faceFlux.fill(FluxFiller);

    auto marker = exahype2::dg::rk::tests::makeFaceMarker(0);
    if (refinedCase == 0) {
      marker.cellHasBeenRefined = true;
    } else {
      marker.cellWillBeRefined = true;
    }

    runFaceKernel<false, false>(marker, solver, faceLabel, faceState, faceFlux);

    for (int entry = 0; entry < 2 * StateElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(faceState.data()[entry], StateFiller, refinedCase, entry);
    }
    for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(faceFlux.data()[entry], FluxFiller, refinedCase, entry);
    }
    validateNumericalEqualsWithParams1(faceLabel.getUpdatedTimeStamp(0), 1.0, refinedCase);
  }
  solver.reduceLocalUpdates();
}

/**
 * A face on a periodic boundary keeps both traces and both time stamps, because the neighbour
 * across the domain writes the second trace.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testPeriodicFaceKeepsBothTracesAndTimeStamps() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  StageTerms::PeriodicBoundaries.set(0);

  FaceLabel faceLabel;
  faceLabel.setBoundary(true);
  faceLabel.setUpdatedTimeStamp(0, 1.0);
  faceLabel.setUpdatedTimeStamp(1, 2.0);

  FaceState faceState;
  FaceFlux  faceFlux;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    faceState.data()[faceNode * NumberOfData]                          = 1.0 + faceNode;
    faceState.data()[StateElementsPerFace + faceNode * NumberOfData]   = 3.0 + faceNode;
    faceFlux.data()[faceNode * NumberOfUnknowns]                       = 0.25;
    faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns] = 0.75;
  }

  runFaceKernel<false, false>(exahype2::dg::rk::tests::makeFaceMarker(0), solver, faceLabel, faceState, faceFlux);

  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(0), 1.0);
  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(1), 2.0);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double leftState  = 1.0 + faceNode;
    const double rightState = 3.0 + faceNode;
    validateNumericalEqualsWithParams1(faceState.data()[faceNode * NumberOfData], leftState, faceNode);
    const double expected = 0.5 * (0.25 + 0.75 + MaxEigenvalue * (leftState - rightState));
    validateNumericalEqualsWithParams1(faceFlux.data()[faceNode * NumberOfUnknowns], expected, faceNode);
  }
  runFaceKernel<false, true>(exahype2::dg::rk::tests::makeFaceMarker(0), solver, faceLabel, faceState, faceFlux);
  validateEquals(StageTerms::riemannSolverCalls, 1);
  validateEquals(StageTerms::riemannSolverBoundary, false);
  StageTerms::PeriodicBoundaries.reset();
  solver.reduceLocalUpdates();
}

/**
 * A face inside the domain keeps both traces and both time stamps and solves the Riemann
 * problem of the two projected fluxes.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testInteriorFaceKeepsTheTimeStampsAndSolvesTheRiemannProblem() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  FaceLabel faceLabel;
  faceLabel.setUpdatedTimeStamp(0, 1.0);
  faceLabel.setUpdatedTimeStamp(1, 2.0);

  FaceState faceState;
  FaceFlux  faceFlux;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    faceState.data()[faceNode * NumberOfData]                          = 1.0 + faceNode;
    faceState.data()[StateElementsPerFace + faceNode * NumberOfData]   = 3.0 + faceNode;
    faceFlux.data()[faceNode * NumberOfUnknowns]                       = 0.25;
    faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns] = 0.75;
  }

  runFaceKernel<false, false>(exahype2::dg::rk::tests::makeFaceMarker(0), solver, faceLabel, faceState, faceFlux);

  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(0), 1.0);
  validateNumericalEquals(faceLabel.getUpdatedTimeStamp(1), 2.0);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double leftState  = 1.0 + faceNode;
    const double rightState = 3.0 + faceNode;
    validateNumericalEqualsWithParams1(faceState.data()[faceNode * NumberOfData], leftState, faceNode);
    const double expected = 0.5 * (0.25 + 0.75 + MaxEigenvalue * (leftState - rightState));
    validateNumericalEqualsWithParams1(faceFlux.data()[faceNode * NumberOfUnknowns], expected, faceNode);
    validateNumericalEqualsWithParams1(
      faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      expected,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * A solver that brings its own Riemann solver receives the face number, the normal and the
 * boundary flag of the face, and its result replaces both flux halves.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testUserRiemannSolverReceivesTheFaceAndWritesBothFluxes() {
  constexpr int    UpperFace         = DIMENSIONS;
  constexpr double InteriorTimeStamp = 2.5;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  FaceLabel faceLabel;
  faceLabel.setUpdatedTimeStamp(0, InteriorTimeStamp);
  faceLabel.setUpdatedTimeStamp(1, 4.0);

  FaceState faceState;
  FaceFlux  faceFlux;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    faceState.data()[faceNode * NumberOfData]                          = 1.0 + faceNode;
    faceState.data()[StateElementsPerFace + faceNode * NumberOfData]   = 3.0 + faceNode;
    faceFlux.data()[faceNode * NumberOfUnknowns]                       = 0.25;
    faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns] = 0.75;
  }

  runFaceKernel<false, true>(
    exahype2::dg::rk::tests::makeFaceMarker(UpperFace),
    solver,
    faceLabel,
    faceState,
    faceFlux
  );

  validateEquals(StageTerms::riemannSolverCalls, 1);
  validateEquals(StageTerms::riemannSolverFaceNumber, UpperFace);
  validateEquals(StageTerms::riemannSolverDirection, 0);
  validateEquals(StageTerms::riemannSolverBoundary, false);
  validateNumericalEquals(StageTerms::riemannSolverTimeStamp, InteriorTimeStamp);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    validateNumericalEqualsWithParams1(faceFlux.data()[faceNode * NumberOfUnknowns], 1.0 + faceNode, faceNode);
    validateNumericalEqualsWithParams1(
      faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      3.0 + faceNode,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * A linear solver evaluates its flux on half the state jump and adds the wave speed times that
 * jump to the flux of one side and subtracts it from the other.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testLinearFaceFluxesFollowTheStateJump() {
  constexpr double InteriorTimeStamp = 2.0;
  constexpr double StateJump         = 2.0;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);

  FaceLabel faceLabel;
  faceLabel.setUpdatedTimeStamp(0, 1.0);
  faceLabel.setUpdatedTimeStamp(1, InteriorTimeStamp);

  FaceState faceState;
  FaceFlux  faceFlux;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    faceState.data()[faceNode * NumberOfData]                          = 1.0 + faceNode;
    faceState.data()[StateElementsPerFace + faceNode * NumberOfData]   = 1.0 + faceNode + StateJump;
    faceFlux.data()[faceNode * NumberOfUnknowns]                       = 0.25;
    faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns] = 0.75;
  }

  runFaceKernel<true, false>(exahype2::dg::rk::tests::makeFaceMarker(0), solver, faceLabel, faceState, faceFlux);

  const double halfJump    = 0.5 * StateJump;
  const double centralFlux = halfJump + InteriorTimeStamp;
  const double dissipation = MaxEigenvalue * halfJump;
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    validateNumericalEqualsWithParams1(
      faceFlux.data()[faceNode * NumberOfUnknowns],
      centralFlux - dissipation,
      faceNode
    );
    validateNumericalEqualsWithParams1(
      faceFlux.data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      centralFlux + dissipation,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * Only the first sweep of a time step moves the time stamps of a face, and only on the halves
 * a cell has written.
 */
void exahype2::dg::rk::tests::UpdateStageTest::testTimeStampsRollOverOnTheFirstSweepOnly() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), false);

  FaceLabel faceLabel;
  faceLabel.setUpdated(0, true);
  faceLabel.setUpdated(1, false);
  faceLabel.setUpdatedTimeStamp(0, 1.0);
  faceLabel.setUpdatedTimeStamp(1, 2.0);
  faceLabel.setNewTimeStamp(0, 3.0);
  faceLabel.setNewTimeStamp(1, 4.0);
  faceLabel.setOldTimeStamp(0, 5.0);
  faceLabel.setOldTimeStamp(1, 6.0);

  exahype2::dg::rk::rollOverFaceTimeStamps(solver, faceLabel);
  validateNumericalEquals(faceLabel.getNewTimeStamp(0), 3.0);
  validateNumericalEquals(faceLabel.getOldTimeStamp(0), 5.0);

  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.isFirstGridSweepOfTimeStep(), true);
  exahype2::dg::rk::rollOverFaceTimeStamps(solver, faceLabel);
  validateNumericalEquals(faceLabel.getOldTimeStamp(0), 3.0);
  validateNumericalEquals(faceLabel.getNewTimeStamp(0), 1.0);
  validateNumericalEquals(faceLabel.getOldTimeStamp(1), 6.0);
  validateNumericalEquals(faceLabel.getNewTimeStamp(1), 4.0);
  solver.reduceLocalUpdates();
}
