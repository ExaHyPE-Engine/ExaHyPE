// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CorrectAndFlagTest.h"

#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/limiting/CorrectAndFlag.h"
#include "exahype2/dg/limiting/SubcellLayout.h"
#include "exahype2/dg/limiting/tests/LimiterFixtures.h"
#include "peano4/Constants.h"

namespace {
  using AdaptiveSolver = exahype2::dg::limiting::tests::TestSolver<true>;
  using FixedSolver    = exahype2::dg::limiting::tests::TestSolver<false>;
  using Layout         = exahype2::dg::limiting::SubcellLayout<AdaptiveSolver>;
  using CellLabel      = exahype2::dg::limiting::tests::TestCellLabel;
  using LimiterStatus  = exahype2::dg::LimiterStatus;

  using FaceLabel    = exahype2::dg::limiting::tests::TestFaceLabel;
  using FaceLabels   = exahype2::dg::limiting::tests::TestEnumerator<FaceLabel, peano4::FacesPerCell>;
  using VertexLabel  = exahype2::dg::limiting::tests::TestVertexLabel<1>;
  using VertexLabels = exahype2::dg::limiting::tests::TestEnumerator<VertexLabel, peano4::VerticesPerCell>;

  // Every column sums to one, so a constant candidate keeps its value on the subgrid.
  constexpr double
    DgToFvMatrix[Layout::NumberOfDofsPerAxis * Layout::NumberOfSubcellsPerAxis]{1.0, 0.5, 0.0, 0.0, 0.5, 1.0};
  constexpr double DgToLobattoMatrix[Layout::NumberOfDofsPerAxis * Layout::NumberOfDofsPerAxis]{1.0, 0.0, 0.0, 1.0};
  constexpr double QuadraturePoints1D[Layout::NumberOfDofsPerAxis]{0.25, 0.75};

  constexpr double CellCentre   = 0.5;
  constexpr double CellWidth    = 1.0;
  constexpr double TimeStamp    = 1.0;
  constexpr double TimeStepSize = 0.25;

  CellLabel createCellLabel() {
    CellLabel cellLabel;
    cellLabel.timeStamp    = TimeStamp;
    cellLabel.timeStepSize = TimeStepSize;
    return cellLabel;
  }

  /**
   * Tests a constant candidate with the admissibility term alone and returns the faces the verdict
   * reaches.
   */
  template <typename SolverType>
  void correctAndFlagConstantCandidate(
    SolverType& solver,
    CellLabel&  cellLabel,
    FaceLabel (&faces)[peano4::FacesPerCell],
    const double candidateValue,
    const bool   usesAdmissibilityCriterion
  ) {
    const FaceLabels faceLabels{faces};
    double           candidate[Layout::NumberOfNodes];
    std::fill_n(candidate, Layout::NumberOfNodes, candidateValue);
    const auto marker = exahype2::dg::limiting::tests::TestGeometry::cellMarker(CellCentre, CellWidth);
    if (usesAdmissibilityCriterion) {
      exahype2::dg::limiting::correctAndFlag<SolverType, true>(
        marker,
        solver,
        cellLabel,
        faceLabels,
        candidate,
        DgToFvMatrix,
        QuadraturePoints1D
      );
    } else {
      exahype2::dg::limiting::correctAndFlag<SolverType, false>(
        marker,
        solver,
        cellLabel,
        faceLabels,
        candidate,
        DgToFvMatrix,
        QuadraturePoints1D
      );
    }
  }

  /**
   * Tests a constant candidate against a previous solution of 0.5 and vertices whose previous
   * observables span [0, 1], and returns the status of the cell.
   */
  LimiterStatus statusAfterTheMaximumPrinciple(AdaptiveSolver& solver, const double candidateValue) {
    CellLabel   cellLabel = createCellLabel();
    FaceLabel   faces[peano4::FacesPerCell];
    VertexLabel vertices[peano4::VerticesPerCell];
    for (auto& vertexLabel : vertices) {
      vertexLabel.setDmpMinimum(0, 0.0);
      vertexLabel.setDmpMaximum(0, 1.0);
    }
    const FaceLabels   faceLabels{faces};
    const VertexLabels vertexLabels{vertices};
    double             candidate[Layout::NumberOfNodes];
    double             previousSolution[Layout::NumberOfNodes];
    std::fill_n(candidate, Layout::NumberOfNodes, candidateValue);
    std::fill_n(previousSolution, Layout::NumberOfNodes, 0.5);

    exahype2::dg::limiting::correctAndFlag<AdaptiveSolver, true, 1, false>(
      exahype2::dg::limiting::tests::TestGeometry::cellMarker(CellCentre, CellWidth),
      solver,
      cellLabel,
      faceLabels,
      vertexLabels,
      candidate,
      previousSolution,
      DgToFvMatrix,
      DgToLobattoMatrix,
      DgToFvMatrix,
      QuadraturePoints1D,
      0.0,
      0.0
    );
    return cellLabel.getLimiterStatus();
  }
} // namespace

exahype2::dg::limiting::tests::CorrectAndFlagTest::CorrectAndFlagTest() :
  tarch::tests::TestCase("exahype2::dg::limiting::tests::CorrectAndFlagTest") {}

void exahype2::dg::limiting::tests::CorrectAndFlagTest::run() {
  testMethod(testDivergedPredictorMarksTheCellTroubled);
  testMethod(testAdmissibleCandidateKeepsTheGalerkinScheme);
  testMethod(testInadmissibleCandidateReachesTheAdjacentFaces);
  testMethod(testAdaptiveSolverTakesTheWaveSpeedOfAnAcceptedCandidate);
  testMethod(testTheNeighbourhoodRangeDecidesTheMaximumPrinciple);
  testMethod(testAnAdmissibleCellWithATroubledHistoryStaysLimited);
  testMethod(testACellWithoutARegularPreviousStatusSkipsTheWaveSpeed);
  testMethod(testAFixedStepSolverTakesTheVerdictWithoutAWaveSpeed);
}

/**
 * Checks that a cell whose predictor diverged becomes troubled.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testDivergedPredictorMarksTheCellTroubled() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  cellLabel.setPredictorDiverged(true);
  FaceLabel faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, 1.0, false);

  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Troubled));
  validateTrue(cellLabel.getWasTroubledInPreviousStep());
  validateEquals(solver.numberOfTroubledCells, 1);
}

/**
 * Checks that an admissible candidate of an untroubled cell keeps the Galerkin scheme.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testAdmissibleCandidateKeepsTheGalerkinScheme() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  FaceLabel      faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, 1.0, true);

  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Regular));
  validateFalse(cellLabel.getWasTroubledInPreviousStep());
  validateEquals(solver.numberOfTroubledCells, 0);
}

/**
 * Checks that the status of a rejected candidate reaches the side of each face the cell owns.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testInadmissibleCandidateReachesTheAdjacentFaces() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  FaceLabel      faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, -1.0, true);

  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Troubled));
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int ownSide = faceNumber < DIMENSIONS ? 1 : 0;
    validateEqualsWithParams1(
      static_cast<int>(faces[faceNumber].getLimiterStatus(ownSide)),
      static_cast<int>(LimiterStatus::Troubled),
      faceNumber
    );
  }
}

/**
 * Checks that an adaptive solver reads the wave speed of a candidate it accepts.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testAdaptiveSolverTakesTheWaveSpeedOfAnAcceptedCandidate() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  FaceLabel      faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, 1.0, true);

  validateNumericalEquals(solver.maxPhysicalEigenvalue, AdaptiveSolver::AdvectionSpeed);
  validateNumericalEquals(solver.maxScaledEigenvalue, DIMENSIONS * AdaptiveSolver::AdvectionSpeed / CellWidth);
}

/**
 * Checks that the discrete maximum principle accepts a candidate inside the range the adjacent
 * vertices widen and rejects one outside it, while the admissibility term accepts both.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testTheNeighbourhoodRangeDecidesTheMaximumPrinciple() {
  AdaptiveSolver acceptingSolver;
  validateEquals(
    static_cast<int>(statusAfterTheMaximumPrinciple(acceptingSolver, 0.9)),
    static_cast<int>(LimiterStatus::Regular)
  );
  validateEquals(acceptingSolver.numberOfTroubledCells, 0);

  AdaptiveSolver rejectingSolver;
  validateEquals(
    static_cast<int>(statusAfterTheMaximumPrinciple(rejectingSolver, 5.0)),
    static_cast<int>(LimiterStatus::Troubled)
  );
  validateEquals(rejectingSolver.numberOfTroubledCells, 1);
}

/**
 * Checks that an admissible cell that was troubled in the previous step stays on the subcell
 * scheme, counts as troubled and reports no wave speed.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testAnAdmissibleCellWithATroubledHistoryStaysLimited() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  cellLabel.setWasTroubledInPreviousStep(true);
  FaceLabel faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, 1.0, true);

  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Limited));
  validateFalse(cellLabel.getWasTroubledInPreviousStep());
  validateEquals(solver.numberOfTroubledCells, 1);
  validateNumericalEquals(solver.maxPhysicalEigenvalue, 0.0);
}

/**
 * Checks that an accepted candidate of a cell that was not regular in the previous step reports
 * no wave speed.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testACellWithoutARegularPreviousStatusSkipsTheWaveSpeed() {
  AdaptiveSolver solver;
  CellLabel      cellLabel = createCellLabel();
  cellLabel.setPreviousLimiterStatus(LimiterStatus::Limited);
  FaceLabel faces[peano4::FacesPerCell];

  correctAndFlagConstantCandidate(solver, cellLabel, faces, 1.0, true);

  validateEquals(static_cast<int>(cellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Regular));
  validateEquals(solver.numberOfTroubledCells, 0);
  validateNumericalEquals(solver.maxPhysicalEigenvalue, 0.0);
}

/**
 * Checks that a fixed-step solver takes the verdict of a candidate without reading a wave speed.
 */
void exahype2::dg::limiting::tests::CorrectAndFlagTest::testAFixedStepSolverTakesTheVerdictWithoutAWaveSpeed() {
  FixedSolver solver;
  CellLabel   acceptedCellLabel = createCellLabel();
  FaceLabel   acceptedFaces[peano4::FacesPerCell];
  correctAndFlagConstantCandidate(solver, acceptedCellLabel, acceptedFaces, 1.0, true);
  validateEquals(static_cast<int>(acceptedCellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Regular));

  CellLabel rejectedCellLabel = createCellLabel();
  FaceLabel rejectedFaces[peano4::FacesPerCell];
  correctAndFlagConstantCandidate(solver, rejectedCellLabel, rejectedFaces, -1.0, true);
  validateEquals(static_cast<int>(rejectedCellLabel.getLimiterStatus()), static_cast<int>(LimiterStatus::Troubled));

  validateEquals(solver.numberOfTroubledCells, 1);
  validateNumericalEquals(solver.maxPhysicalEigenvalue, 0.0);
}
