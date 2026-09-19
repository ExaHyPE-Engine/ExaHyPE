// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/amr/LevelJumpEvents.h"
#include "exahype2/dg/rk/DynamicAMR.h"
#include "exahype2/dg/rk/tests/StageFixtures.h"

namespace {
  using ButcherTableau = exahype2::dg::rk::tests::ButcherTableau;
  using CellLabel      = exahype2::dg::rk::tests::CellLabel;
  using DGMatrices     = exahype2::dg::rk::tests::DGMatrices;
  using FaceLabel      = exahype2::dg::rk::tests::FaceLabel;
  using Quadrature     = exahype2::dg::rk::tests::Quadrature;
  using TestSolver     = exahype2::dg::rk::tests::TestSolver;

  constexpr int FluxElementsPerFace  = exahype2::dg::rk::tests::FluxElementsPerFace;
  constexpr int NodesPerCell         = exahype2::dg::rk::tests::NodesPerCell;
  constexpr int NumberOfData         = exahype2::dg::rk::tests::NumberOfData;
  constexpr int RkStages             = exahype2::dg::rk::tests::RkStages;
  constexpr int StateElementsPerFace = exahype2::dg::rk::tests::StateElementsPerFace;
  constexpr int ValuesPerStage       = exahype2::dg::rk::tests::ValuesPerStage;

  constexpr int CellStateValues = NodesPerCell * NumberOfData;
  constexpr int StageValues     = RkStages * ValuesPerStage;
  constexpr int SubfacesPerFace = exahype2::amr::SubfacesPerFace;

  using FaceState = exahype2::dg::rk::tests::ValuePatch<2 * StateElementsPerFace>;
  using FaceFlux  = exahype2::dg::rk::tests::ValuePatch<2 * FluxElementsPerFace>;
  using CellState = exahype2::dg::rk::tests::ValuePatch<CellStateValues>;
  using CellStage = exahype2::dg::rk::tests::ValuePatch<StageValues>;

  constexpr double ConstantState  = 2.5;
  constexpr double ParentStamp    = 1.25;
  constexpr double ChildStamp     = 3.0;
  constexpr double CellTimeStamp  = 1.0;
  constexpr double TimeStepSize   = 0.25;
  constexpr double FaceStamp      = 2.0;
  constexpr double UntouchedEntry = 9.0;

  //! Returns the value that marks the estimate of one Runge-Kutta stage.
  double stageValue(const int stage) { return 1.0 + stage; }

  void fillStages(CellStage& stages) {
    for (int stage = 0; stage < RkStages; stage++) {
      for (int value = 0; value < ValuesPerStage; value++) {
        stages.data()[stage * ValuesPerStage + value] = stageValue(stage);
      }
    }
  }

  void fillCellState(CellState& cellState, const double value) {
    for (int entry = 0; entry < CellStateValues; entry++) {
      cellState.data()[entry] = value;
    }
  }

  //! Clears the transfer flag of every side, so a test tells which side a kernel flags.
  void releaseEveryTransferFlag(FaceLabel* const labels) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      labels[face].setRequiresDataTransfer(0, false);
      labels[face].setRequiresDataTransfer(1, false);
    }
  }

  struct TransferGrid final {
    CellLabel cellLabel;
    FaceLabel labels[peano4::FacesPerCell]{};
    FaceState states[peano4::FacesPerCell]{};
    FaceFlux  fluxes[peano4::FacesPerCell]{};
    double    stageEstimates[StageValues]{};
    double    cellState[CellStateValues]{};
  };

  void fillTransferGrid(TransferGrid& grid) {
    grid.cellLabel.setTimeStamp(CellTimeStamp);
    grid.cellLabel.setTimeStepSize(TimeStepSize);
    grid.cellLabel.setRequiresCriterionEvaluation(true);
    releaseEveryTransferFlag(grid.labels);
    for (int entry = 0; entry < StageValues; entry++) {
      grid.stageEstimates[entry] = UntouchedEntry;
    }
    for (int entry = 0; entry < CellStateValues; entry++) {
      grid.cellState[entry] = ConstantState;
    }
  }

  void runProjectStageAfterTransfer(const TestSolver& solver, TransferGrid& grid) {
    const auto faceLabels = exahype2::dg::rk::tests::makeFaceEnumerator(grid.labels);
    const auto faceStates = exahype2::dg::rk::tests::makeFaceEnumerator(grid.states);
    const auto faceFluxes = exahype2::dg::rk::tests::makeFaceEnumerator(grid.fluxes);
    exahype2::dg::rk::
      projectStageAfterTransfer<TestSolver, false, false, false, false, false, Quadrature, DGMatrices, ButcherTableau>(
        exahype2::dg::rk::tests::makeCellMarker(),
        solver,
        grid.cellLabel,
        faceLabels,
        faceStates,
        faceFluxes,
        grid.cellState,
        grid.stageEstimates
      );
  }
} // namespace

exahype2::dg::rk::tests::DynamicAMRTest::DynamicAMRTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::DynamicAMRTest") {}

void exahype2::dg::rk::tests::DynamicAMRTest::run() {
  testMethod(testCreateFaceClearsTheBuffersAndTakesTheParentTimeStamp);
  testMethod(testCreateCellClearsTheStagesDuringGridInitialisation);
  testMethod(testCreateCellProlongsEveryStageOfARunningTimeStep);
  testMethod(testDestroyCellRestrictsEveryBufferAndFlagsTheParentFaces);
  testMethod(testRestrictHangingFaceTraceReducesTheSubfacesAndEvaluatesTheCoarseFlux);
  testMethod(testProjectStageAfterTransferMarksTheOwnFaceSides);
  testMethod(testProjectStageAfterTransferSkipsTheGridInitialisation);
  testMethod(testCellThatOwesDataIntegratesItsVolumeAgain);
}

/**
 * A created face starts with an empty trace and an empty flux and carries the time stamp of its
 * parent cell on both sides.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testCreateFaceClearsTheBuffersAndTakesTheParentTimeStamp() {
  FaceState faceState;
  FaceFlux  faceFlux;
  faceState.fill(1.0);
  faceFlux.fill(1.0);

  CellLabel coarseCellLabel;
  coarseCellLabel.setTimeStamp(ParentStamp);
  FaceLabel fineFaceLabel;

  exahype2::dg::rk::createFace(faceState, faceFlux, coarseCellLabel, fineFaceLabel);

  for (int value = 0; value < FaceState::Cardinality; value++) {
    validateNumericalEqualsWithParams1(faceState.data()[value], 0.0, value);
  }
  for (int value = 0; value < FaceFlux::Cardinality; value++) {
    validateNumericalEqualsWithParams1(faceFlux.data()[value], 0.0, value);
  }
  validateNumericalEquals(fineFaceLabel.getNewTimeStamp(0), ParentStamp);
  validateNumericalEquals(fineFaceLabel.getOldTimeStamp(1), ParentStamp);
}

/**
 * A cell the grid initialisation creates takes the solution of its parent and starts with empty
 * stage buffers.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testCreateCellClearsTheStagesDuringGridInitialisation() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterGridInitialisation(solver);

  CellState coarseCellState;
  CellState fineCellState;
  CellStage coarseStages;
  CellStage fineStages;
  fillCellState(coarseCellState, ConstantState);
  fillStages(coarseStages);
  fillStages(fineStages);

  CellLabel coarseCellLabel;
  CellLabel fineCellLabel;
  coarseCellLabel.setTimeStamp(ParentStamp);

  exahype2::dg::rk::createCell<TestSolver, DGMatrices>(
    exahype2::dg::rk::tests::makeChildCellMarker(0),
    solver,
    coarseCellState,
    fineCellState,
    coarseStages,
    fineStages,
    coarseCellLabel,
    fineCellLabel
  );

  for (int value = 0; value < CellStateValues; value++) {
    validateNumericalEqualsWithParams1(fineCellState.data()[value], ConstantState, value);
  }
  for (int value = 0; value < StageValues; value++) {
    validateNumericalEqualsWithParams1(fineStages.data()[value], 0.0, value);
  }
  validateNumericalEquals(fineCellLabel.getTimeStamp(), ParentStamp);
  validateTrue(fineCellLabel.getRequiresCriterionEvaluation());
}

/**
 * A cell created during a time step takes the estimate of every Runge-Kutta stage of its parent.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testCreateCellProlongsEveryStageOfARunningTimeStep() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);

  CellState coarseCellState;
  CellState fineCellState;
  CellStage coarseStages;
  CellStage fineStages;
  fillCellState(coarseCellState, ConstantState);
  fillStages(coarseStages);

  CellLabel coarseCellLabel;
  CellLabel fineCellLabel;

  exahype2::dg::rk::createCell<TestSolver, DGMatrices>(
    exahype2::dg::rk::tests::makeChildCellMarker(ChildrenPerCell - 1),
    solver,
    coarseCellState,
    fineCellState,
    coarseStages,
    fineStages,
    coarseCellLabel,
    fineCellLabel
  );

  for (int stage = 0; stage < RkStages; stage++) {
    for (int value = 0; value < ValuesPerStage; value++) {
      validateNumericalEqualsWithParams2(
        fineStages.data()[stage * ValuesPerStage + value],
        stageValue(stage),
        stage,
        value
      );
    }
  }
  solver.reduceLocalUpdates();
}

/**
 * A complete sibling set restricts the solution and every stage estimate onto the parent and
 * flags the parent's own face sides.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testDestroyCellRestrictsEveryBufferAndFlagsTheParentFaces() {
  exahype2::amr::ChildDataStage<double> cellStateStage(ChildrenPerCell, CellStateValues);
  exahype2::amr::ChildDataStage<double> stageEstimatesStage(ChildrenPerCell, StageValues);

  double coarseCellState[CellStateValues]{};
  double coarseStages[StageValues]{};

  CellState fineCellState;
  CellStage fineStages;
  fillCellState(fineCellState, ConstantState);
  fillStages(fineStages);

  FaceLabel coarseFaceLabels[peano4::FacesPerCell]{};
  releaseEveryTransferFlag(coarseFaceLabels);
  const auto faceLabels = exahype2::dg::rk::tests::makeFaceEnumerator(coarseFaceLabels);

  CellLabel coarseCellLabel;
  for (int child = 0; child < ChildrenPerCell; child++) {
    CellLabel fineCellLabel;
    fineCellLabel.setTimeStamp(ChildStamp);
    fineCellLabel.setTimeStepSize(TimeStepSize);
    exahype2::dg::rk::destroyCell<TestSolver, DGMatrices>(
      exahype2::dg::rk::tests::makeChildCellMarker(child),
      cellStateStage,
      stageEstimatesStage,
      coarseCellState,
      fineCellState.data(),
      coarseStages,
      fineStages.data(),
      faceLabels,
      coarseCellLabel,
      fineCellLabel
    );
  }

  for (int value = 0; value < CellStateValues; value++) {
    validateNumericalEqualsWithParams1(coarseCellState[value], ConstantState, value);
  }
  for (int stage = 0; stage < RkStages; stage++) {
    for (int value = 0; value < ValuesPerStage; value++) {
      validateNumericalEqualsWithParams2(coarseStages[stage * ValuesPerStage + value], stageValue(stage), stage, value);
    }
  }
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateTrue(faceLabels(face).getRequiresDataTransfer(ownSide));
    validateFalse(faceLabels(face).getRequiresDataTransfer(1 - ownSide));
  }
  validateNumericalEquals(coarseCellLabel.getTimeStamp(), ChildStamp);
  validateNumericalEquals(coarseCellLabel.getTimeStepSize(), TimeStepSize);
}

/**
 * A coarse face takes the trace of its subfaces, reports the update and evaluates the flux of
 * the restricted side on the coarse face.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::
  testRestrictHangingFaceTraceReducesTheSubfacesAndEvaluatesTheCoarseFlux() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);

  exahype2::amr::ChildDataStage<double> stage(SubfacesPerFace, StateElementsPerFace);
  double                                coarseFaceState[2 * StateElementsPerFace]{};
  double                                coarseFaceFlux[2 * FluxElementsPerFace]{};
  FaceState                             fineFaceState;
  fineFaceState.fill(ConstantState);

  FaceLabel coarseFaceLabel;
  FaceLabel fineFaceLabel;
  fineFaceLabel.setUpdatedTimeStamp(1, FaceStamp);

  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    exahype2::dg::rk::restrictHangingFaceTrace<TestSolver, true, false, Quadrature, DGMatrices>(
      exahype2::dg::rk::tests::makeSubfaceMarker(0, subface),
      stage,
      coarseFaceState,
      fineFaceState.data(),
      coarseFaceFlux,
      coarseFaceLabel,
      fineFaceLabel,
      TimeStepSize
    );
  }

  validateTrue(coarseFaceLabel.getUpdated(1));
  validateNumericalEquals(coarseFaceLabel.getUpdatedTimeStamp(1), FaceStamp);
  for (int value = 0; value < StateElementsPerFace; value++) {
    validateNumericalEqualsWithParams1(coarseFaceState[StateElementsPerFace + value], ConstantState, value);
    validateNumericalEqualsWithParams1(coarseFaceState[value], 0.0, value);
  }

  // The flux along the first axis repeats the restricted trace and adds the time stamp the
  // coarse face carries.
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    validateNumericalEqualsWithParams1(
      coarseFaceFlux[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      ConstantState + FaceStamp,
      faceNode
    );
    validateNumericalEqualsWithParams1(coarseFaceFlux[faceNode * NumberOfUnknowns], 0.0, faceNode);
  }
  solver.reduceLocalUpdates();
}

/**
 * A cell that owes no data to a face projects its stage onto the faces, marks the sides it
 * writes and reports that its refinement criterion has been evaluated.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testProjectStageAfterTransferMarksTheOwnFaceSides() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  validateEquals(solver.getRkStageAfterDynamicMeshRefinement(), 1);

  TransferGrid grid;
  fillTransferGrid(grid);

  runProjectStageAfterTransfer(solver, grid);

  const double stageTimeStamp = CellTimeStamp + ButcherTableau::RelativeTimeStepSizes[1] * TimeStepSize;
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateTrue(grid.labels[face].getUpdated(ownSide));
    validateFalse(grid.labels[face].getRequiresDataTransfer(ownSide));
    validateNumericalEqualsWithParams1(grid.labels[face].getUpdatedTimeStamp(ownSide), stageTimeStamp, face);
  }
  for (int entry = 0; entry < StageValues; entry++) {
    validateNumericalEqualsWithParams1(grid.stageEstimates[entry], UntouchedEntry, entry);
  }
  validateFalse(grid.cellLabel.getRequiresCriterionEvaluation());
  solver.reduceLocalUpdates();
}

/**
 * A cell the grid initialisation transfers writes no face data, and its refinement criterion
 * still counts as evaluated.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testProjectStageAfterTransferSkipsTheGridInitialisation() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterGridInitialisation(solver);

  TransferGrid grid;
  fillTransferGrid(grid);

  runProjectStageAfterTransfer(solver, grid);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateFalse(grid.labels[face].getUpdated(ownSide));
    for (int value = 0; value < 2 * StateElementsPerFace; value++) {
      validateNumericalEqualsWithParams2(grid.states[face].data()[value], 0.0, face, value);
    }
  }
  validateFalse(grid.cellLabel.getRequiresCriterionEvaluation());
}

/**
 * A cell that owes data to one of its faces integrates its volume again, which overwrites the
 * estimate of the stage the mesh refinement interrupted.
 */
void exahype2::dg::rk::tests::DynamicAMRTest::testCellThatOwesDataIntegratesItsVolumeAgain() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  exahype2::dg::rk::tests::advanceOneSweep(solver);
  const int rungeKuttaStage = solver.getRkStageAfterDynamicMeshRefinement();

  TransferGrid grid;
  fillTransferGrid(grid);
  grid.labels[0].setRequiresDataTransfer(1, true);

  runProjectStageAfterTransfer(solver, grid);

  // A solver without terms integrates a vanishing right-hand side.
  for (int value = 0; value < ValuesPerStage; value++) {
    validateNumericalEqualsWithParams1(grid.stageEstimates[rungeKuttaStage * ValuesPerStage + value], 0.0, value);
  }
  for (int stage = 0; stage < RkStages; stage++) {
    if (stage != rungeKuttaStage) {
      for (int value = 0; value < ValuesPerStage; value++) {
        validateNumericalEqualsWithParams2(
          grid.stageEstimates[stage * ValuesPerStage + value],
          UntouchedEntry,
          stage,
          value
        );
      }
    }
  }
  solver.reduceLocalUpdates();
}
