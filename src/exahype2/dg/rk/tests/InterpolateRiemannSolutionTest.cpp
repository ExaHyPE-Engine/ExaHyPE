// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InterpolateRiemannSolutionTest.h"

#include "exahype2/dg/rk/InterpolateRiemannSolution.h"
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
  constexpr int NodesPerFace         = exahype2::dg::rk::tests::NodesPerFace;
  constexpr int NumberOfData         = exahype2::dg::rk::tests::NumberOfData;
  constexpr int RkStages             = exahype2::dg::rk::tests::RkStages;
  constexpr int StateElementsPerFace = exahype2::dg::rk::tests::StateElementsPerFace;
  constexpr int NumberOfUnknowns     = exahype2::dg::rk::tests::NumberOfUnknowns;

  using FaceState = exahype2::dg::rk::tests::ValuePatch<2 * StateElementsPerFace>;
  using FaceFlux  = exahype2::dg::rk::tests::ValuePatch<2 * FluxElementsPerFace>;

  constexpr int    LowerHangingFace  = 0;
  constexpr int    UpperHangingFace  = DIMENSIONS;
  constexpr double CellTimeStamp     = 0.5;
  constexpr double CellTimeStepSize  = 0.125;
  constexpr double RelativeStageStep = 0.25;
  constexpr double StateFiller       = 8.0;
  constexpr double FluxFiller        = 6.0;
  constexpr double CoarseLowerTrace  = 10.0;
  constexpr double CoarseUpperTrace  = 20.0;

  constexpr double StageTimeStamp = CellTimeStamp + RelativeStageStep * CellTimeStepSize;

  struct Grid final {
    CellLabel cellLabel;
    double    cellState[NodesPerCell * NumberOfData]{};
    //! Every estimate stays zero, so the linear combination of any stage is the cell state.
    double    rightHandSideEstimates[RkStages * NodesPerCell * NumberOfUnknowns]{};
    FaceLabel labels[peano4::FacesPerCell]{};
    FaceState fineStates[peano4::FacesPerCell]{};
    FaceState coarseStates[peano4::FacesPerCell]{};
    FaceFlux  fineFluxes[peano4::FacesPerCell]{};
  };

  void fillGrid(Grid& grid) {
    grid.cellLabel.setTimeStamp(CellTimeStamp);
    grid.cellLabel.setTimeStepSize(CellTimeStepSize);
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      grid.cellState[cellNode * NumberOfData] = 1.0 + cellNode;
    }
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      grid.fineStates[face].fill(StateFiller);
      grid.fineFluxes[face].fill(FluxFiller);
      for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
        grid.coarseStates[face].data()[faceNode * NumberOfData]                        = CoarseLowerTrace + faceNode;
        grid.coarseStates[face].data()[StateElementsPerFace + faceNode * NumberOfData] = CoarseUpperTrace + faceNode;
      }
    }
  }

  template <bool UseRiemannSolver = false>
  void runInterpolateRiemannSolution(const peano4::dataview::CellMarker& marker, Grid& grid) {
    const auto faceLabels   = exahype2::dg::rk::tests::makeFaceEnumerator(grid.labels);
    const auto fineStates   = exahype2::dg::rk::tests::makeFaceEnumerator(grid.fineStates);
    const auto coarseStates = exahype2::dg::rk::tests::makeFaceEnumerator(grid.coarseStates);
    const auto fineFluxes   = exahype2::dg::rk::tests::makeFaceEnumerator(grid.fineFluxes);
    exahype2::dg::rk::interpolateRiemannSolution<
      TestSolver,
      false,
      UseRiemannSolver,
      true,
      false,
      false,
      Quadrature,
      DGMatrices,
      ButcherTableau>(
      marker,
      grid.cellLabel,
      faceLabels,
      fineStates,
      coarseStates,
      fineFluxes,
      grid.cellState,
      grid.rightHandSideEstimates,
      0,
      RelativeStageStep
    );
  }

  double prolongationScale(const int faceNumber, const tarch::la::Vector<DIMENSIONS, int>& childPosition) {
    const int direction = faceNumber % DIMENSIONS;
    double    result    = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis != direction) {
        result *= 1.0 + childPosition(axis);
      }
    }
    return result;
  }
} // namespace

exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::InterpolateRiemannSolutionTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::InterpolateRiemannSolutionTest") {}

void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::run() {
  testMethod(testCellWithoutHangingFaceKeepsEveryFace);
  testMethod(testHangingFaceTakesTheCoarseTraceAndTheCellTrace);
  testMethod(testHangingFaceSolvesTheRiemannProblemAtTheStageTimeStamp);
  testMethod(testUpperHangingFaceTakesTheCoarseTraceOfItsChildPosition);
  testMethod(testPeriodicHangingFaceCallsTheInteriorRiemannSolver);
}

/**
 * Checks that a periodic hanging face receives an interior Riemann flux.
 */
void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::testPeriodicHangingFaceCallsTheInteriorRiemannSolver() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  TestSolver::PeriodicBoundaries.set(0);

  Grid grid;
  fillGrid(grid);
  grid.labels[LowerHangingFace].setIsHanging(true);
  grid.labels[LowerHangingFace].setBoundary(true);
  runInterpolateRiemannSolution<true>(exahype2::dg::rk::tests::makeCellMarker(), grid);

  validateEquals(TestSolver::riemannSolverCalls, 1);
  validateEquals(TestSolver::riemannSolverBoundary, false);
  validateNumericalEquals(TestSolver::riemannSolverTimeStamp, StageTimeStamp);

  TestSolver::PeriodicBoundaries.reset();
  solver.reduceLocalUpdates();
}

/**
 * A cell whose faces all persist writes neither a trace nor a flux.
 */
void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::testCellWithoutHangingFaceKeepsEveryFace() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  Grid grid;
  fillGrid(grid);

  runInterpolateRiemannSolution(exahype2::dg::rk::tests::makeCellMarker(), grid);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < 2 * StateElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(grid.fineStates[face].data()[entry], StateFiller, face, entry);
    }
    for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(grid.fineFluxes[face].data()[entry], FluxFiller, face, entry);
    }
  }
  solver.reduceLocalUpdates();
}

/**
 * Checks that hanging faces combine coarse and fine traces without changing persistent faces.
 */
void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::testHangingFaceTakesTheCoarseTraceAndTheCellTrace() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  Grid grid;
  fillGrid(grid);
  grid.labels[LowerHangingFace].setIsHanging(true);

  runInterpolateRiemannSolution(exahype2::dg::rk::tests::makeCellMarker(), grid);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    // The first child's prolongation is the identity.
    validateNumericalEqualsWithParams1(
      grid.fineStates[LowerHangingFace].data()[faceNode * NumberOfData],
      CoarseLowerTrace + faceNode,
      faceNode
    );
    const int lowerNode = exahype2::dg::rk::tests::cellNodeOnFaceLine(LowerHangingFace, faceNode, 0);
    validateNumericalEqualsWithParams1(
      grid.fineStates[LowerHangingFace].data()[StateElementsPerFace + faceNode * NumberOfData],
      grid.cellState[lowerNode * NumberOfData],
      faceNode
    );
  }

  for (int face = 1; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < 2 * StateElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(grid.fineStates[face].data()[entry], StateFiller, face, entry);
    }
    for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
      validateNumericalEqualsWithParams2(grid.fineFluxes[face].data()[entry], FluxFiller, face, entry);
    }
  }
  solver.reduceLocalUpdates();
}

/**
 * Checks that hanging-face fluxes use the stage's time stamp.
 */
void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::
  testHangingFaceSolvesTheRiemannProblemAtTheStageTimeStamp() {
  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  Grid grid;
  fillGrid(grid);
  grid.labels[LowerHangingFace].setIsHanging(true);

  runInterpolateRiemannSolution(exahype2::dg::rk::tests::makeCellMarker(), grid);

  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const double leftState = CoarseLowerTrace + faceNode;
    const double rightState =
      grid.cellState[exahype2::dg::rk::tests::cellNodeOnFaceLine(LowerHangingFace, faceNode, 0) * NumberOfData];
    const double expected = 0.5
      * (leftState + rightState + 2.0 * StageTimeStamp
         + exahype2::dg::rk::tests::MaxEigenvalue * (leftState - rightState));
    validateNumericalEqualsWithParams1(
      grid.fineFluxes[LowerHangingFace].data()[faceNode * NumberOfUnknowns],
      expected,
      faceNode
    );
    validateNumericalEqualsWithParams1(
      grid.fineFluxes[LowerHangingFace].data()[FluxElementsPerFace + faceNode * NumberOfUnknowns],
      expected,
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}

/**
 * Checks that upper hanging faces prolong the upper coarse trace at the child's position.
 */
void exahype2::dg::rk::tests::InterpolateRiemannSolutionTest::
  testUpperHangingFaceTakesTheCoarseTraceOfItsChildPosition() {
  constexpr int LastChild = exahype2::dg::rk::tests::ChildrenPerCell - 1;

  TestSolver solver;
  exahype2::dg::rk::tests::enterTimeStepping(solver);
  Grid grid;
  fillGrid(grid);
  grid.labels[UpperHangingFace].setIsHanging(true);

  runInterpolateRiemannSolution(exahype2::dg::rk::tests::makeChildCellMarker(LastChild), grid);

  const double scale = prolongationScale(UpperHangingFace, exahype2::dg::rk::tests::positionOfChild(LastChild));
  for (int faceNode = 0; faceNode < NodesPerFace; faceNode++) {
    const int upperNode = exahype2::dg::rk::tests::cellNodeOnFaceLine(
      UpperHangingFace % DIMENSIONS,
      faceNode,
      exahype2::dg::rk::tests::NumberOfDofsPerAxis - 1
    );
    validateNumericalEqualsWithParams1(
      grid.fineStates[UpperHangingFace].data()[faceNode * NumberOfData],
      grid.cellState[upperNode * NumberOfData],
      faceNode
    );
    validateNumericalEqualsWithParams1(
      grid.fineStates[UpperHangingFace].data()[StateElementsPerFace + faceNode * NumberOfData],
      scale * (CoarseUpperTrace + faceNode),
      faceNode
    );
  }
  solver.reduceLocalUpdates();
}
