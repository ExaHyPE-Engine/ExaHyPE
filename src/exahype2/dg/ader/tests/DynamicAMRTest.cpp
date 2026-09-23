// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMRTest.h"

#include "exahype2/RefinementControl.h"
#include "exahype2/RefinementControlService.h"
#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/ader/DynamicAMR.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/sfc/ActiveCurve.h"

namespace {
  constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
  constexpr int ChildrenPerCell   = peano4::sfc::ActiveCurve::ChildrenPerCell;
  constexpr int SubfacesPerFace   = exahype2::amr::SubfacesPerFace;

  constexpr int DofsPerAxis   = 2;
  constexpr int Unknowns      = 1;
  constexpr int MatrixEntries = SubdivisionFactor * DofsPerAxis * DofsPerAxis;

  constexpr int FaceNodes = exahype2::dg::NodesPerFace<DofsPerAxis>;
  constexpr int CellNodes = exahype2::dg::NodesPerCell<DofsPerAxis>;

  constexpr int FaceSideValues = FaceNodes * Unknowns;
  constexpr int CellValues     = CellNodes * Unknowns;

  constexpr double ConstantState  = 1.75;
  constexpr double UntouchedValue = -7.0;

  /**
   * Returns an operator that maps every node onto the node of the same index.
   */
  constexpr std::array<double, MatrixEntries> diagonalOperator(const double scale) {
    std::array<double, MatrixEntries> result{};
    for (int interval = 0; interval < SubdivisionFactor; interval++) {
      for (int node = 0; node < DofsPerAxis; node++) {
        result[interval * DofsPerAxis * DofsPerAxis + node * DofsPerAxis + node] = scale;
      }
    }
    return result;
  }

  //! Returns the quadrature weight of every node of the cell.
  constexpr std::array<double, CellNodes> makeCellWeights(const double weight) {
    std::array<double, CellNodes> result{};
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      result[cellNode] = weight;
    }
    return result;
  }

  constexpr std::array<double, MatrixEntries> Prolongation = diagonalOperator(1.0);
  constexpr std::array<double, MatrixEntries> Restriction  = diagonalOperator(1.0 / SubdivisionFactor);
  constexpr std::array<double, DofsPerAxis>   Nodes{0.0, 1.0};
  constexpr std::array<double, DofsPerAxis>   Weights{0.5, 0.5};
  constexpr std::array<double, DofsPerAxis>   InverseWeights{2.0, 2.0};
  // The Picard system scales the cell state with these weights and divides by them again, so a
  // weight of one per node lets a solver without terms reproduce its input.
  constexpr std::array<double, CellNodes>                 CellWeights        = makeCellWeights(1.0);
  constexpr std::array<double, CellNodes>                 InverseCellWeights = makeCellWeights(1.0);
  constexpr std::array<double, DofsPerAxis * DofsPerAxis> ZeroOperator{};
  // The temporal system of a solver without terms holds the initial value at every time node, so
  // its inverse carries the right-hand side of the first time node onto both.
  constexpr std::array<double, DofsPerAxis * DofsPerAxis> InversePicardOperator{1.0, 0.0, 1.0, 0.0};
  constexpr std::array<double, DofsPerAxis>               LeftValues{1.0, 0.0};
  constexpr std::array<double, DofsPerAxis>               RightValues{0.0, 1.0};

  exahype2::dg::ader::BasisTables<double> testTables() {
    return exahype2::dg::ader::BasisTables<double>{
      Nodes.data(),
      Weights.data(),
      InverseWeights.data(),
      CellWeights.data(),
      InverseCellWeights.data(),
      ZeroOperator.data(),
      ZeroOperator.data(),
      ZeroOperator.data(),
      InversePicardOperator.data(),
      LeftValues.data(),
      RightValues.data()
    };
  }

  /**
   * A solver without terms, so the predictor reproduces its input.
   */
  class TestSolver final {
  public:
    static constexpr int    DGOrder                    = DofsPerAxis - 1;
    static constexpr int    NumberOfUnknowns           = Unknowns;
    static constexpr int    NumberOfAuxiliaryVariables = 0;
    static constexpr bool   UsesSubcellLimiter         = false;
    static constexpr double PicardResidualTolerance    = 1.0e-10;

    inline static std::bitset<DIMENSIONS> PeriodicBoundaries{};
  };

  template <int Values>
  class TestRecord final {
  public:
    double*       data() { return _values.data(); }
    const double* data() const { return _values.data(); }

  private:
    std::array<double, Values> _values{};
  };

  using FaceState = TestRecord<2 * FaceSideValues>;
  using FaceFlux  = TestRecord<2 * FaceSideValues>;

  /**
   * The label fields the ADER-DG transfer of a face reads and writes.
   */
  class TestFaceLabel final {
  public:
    bool getBoundary() const { return _boundary; }
    bool getIsHanging() const { return _isHanging; }
    bool getRequiresDataTransfer(const int side) const { return _requiresDataTransfer[side]; }

    void setBoundary(const bool value) { _boundary = value; }
    void setIsHanging(const bool value) { _isHanging = value; }
    void setRequiresDataTransfer(const int side, const bool value) { _requiresDataTransfer[side] = value; }

  private:
    bool _boundary{false};
    bool _isHanging{false};
    bool _requiresDataTransfer[2]{false, false};
  };

  /**
   * The label fields the ADER-DG transfer of a cell reads and writes.
   */
  class TestCellLabel final {
  public:
    double getTimeStamp() const { return _timeStamp; }
    double getTimeStepSize() const { return _timeStepSize; }
    bool   getHasUpdated() const { return _hasUpdated; }
    bool   getRequiresCriterionEvaluation() const { return _requiresCriterionEvaluation; }

    void setTimeStamp(const double value) { _timeStamp = value; }
    void setTimeStepSize(const double value) { _timeStepSize = value; }
    void setHasUpdated(const bool value) { _hasUpdated = value; }
    void setRequiresCriterionEvaluation(const bool value) { _requiresCriterionEvaluation = value; }

  private:
    double _timeStamp{0.0};
    double _timeStepSize{0.0};
    bool   _hasUpdated{false};
    bool   _requiresCriterionEvaluation{false};
  };

  tarch::la::Vector<DIMENSIONS, int> childPosition(const int child) {
    tarch::la::Vector<DIMENSIONS, int> result;
    int                                remainder = child;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      result(axis) = remainder % SubdivisionFactor;
      remainder /= SubdivisionFactor;
    }
    return result;
  }

  peano4::dataview::CellMarker createCellMarker(const int child) {
    peano4::dataview::CellMarker marker{};
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]      = 1.0 / SubdivisionFactor;
      marker.centre[axis] = 0.5;
    }
    marker.relativePositionWithinFatherCell = childPosition(child);
    return marker;
  }

  peano4::dataview::FaceMarker createFaceMarker(const int faceNumber, const int subfaceIndex) {
    peano4::dataview::FaceMarker marker{};
    const int                    normal    = faceNumber % DIMENSIONS;
    int                          remainder = subfaceIndex;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      marker.h[axis]          = 1.0 / SubdivisionFactor;
      marker.cellCentre[axis] = 0.5;
      if (axis != normal) {
        marker.relativePositionWithinFatherCell(axis) = remainder % SubdivisionFactor;
        remainder /= SubdivisionFactor;
      }
    }
    marker.relativePositionWithinFatherCell(normal) = faceNumber < DIMENSIONS ? 0 : SubdivisionFactor - 1;
    marker.select(faceNumber);
    return marker;
  }

  template <typename Element>
  peano4::dataview::FaceEnumerator<Element> enumerate(std::array<Element, peano4::FacesPerCell>& elements) {
    std::array<int, peano4::FacesPerCell> slots{};
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      slots[face] = face;
    }
    return peano4::dataview::FaceEnumerator<Element>(elements.data(), slots.data(), 0);
  }
} // namespace

exahype2::dg::ader::tests::DynamicAMRTest::DynamicAMRTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::DynamicAMRTest") {}

void exahype2::dg::ader::tests::DynamicAMRTest::run() {
  testMethod(testInterpolateHangingFaceFillsTheCoarseHalf);
  testMethod(testStagedFaceFluxHalvesReduceOntoTheCoarseFace);
  testMethod(testCreateCellProlongsBothBuffers);
  testMethod(testDestroyCellRestrictsBothBuffersAndFlagsTheParentFaces);
  testMethod(testReleaseFacesAfterSubcellTransferClearsThePersistentFaces);
  testMethod(testRederivePredictionWaitsForAFaceThatAwaitsData);
  testMethod(testHangingFacesTakeARederivedTraceAndARiemannSolve);
  testMethod(testRederivePredictionWaitsForASettledMeshAndAnAdvancedCell);
}

/**
 * Checks that a hanging face takes the estimates of the coarse cell on the half of that cell.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testInterpolateHangingFaceFillsTheCoarseHalf() {
  std::array<double, 2 * FaceSideValues> coarseFaceState{};
  std::array<double, 2 * FaceSideValues> coarseFaceFlux{};
  std::array<double, 2 * FaceSideValues> fineFaceState{};
  std::array<double, 2 * FaceSideValues> fineFaceFlux{};
  coarseFaceState.fill(ConstantState);
  coarseFaceFlux.fill(ConstantState);

  exahype2::dg::ader::interpolateHangingFace<TestSolver>(
    createFaceMarker(0, 0),
    Prolongation.data(),
    coarseFaceState.data(),
    fineFaceState.data(),
    coarseFaceFlux.data(),
    fineFaceFlux.data()
  );

  for (int value = 0; value < FaceSideValues; value++) {
    validateNumericalEquals(fineFaceState[value], ConstantState);
    validateNumericalEquals(fineFaceFlux[value], ConstantState);
    validateNumericalEquals(fineFaceState[FaceSideValues + value], 0.0);
  }
}

/**
 * Checks that the flux estimates of a complete subface set reduce onto the coarse face.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testStagedFaceFluxHalvesReduceOntoTheCoarseFace() {
  exahype2::amr::ChildDataStage<double>  stage(SubfacesPerFace, FaceSideValues);
  std::array<double, 2 * FaceSideValues> coarseFaceFlux{};
  std::array<double, 2 * FaceSideValues> fineFaceFlux{};
  fineFaceFlux.fill(ConstantState);

  for (int subface = 0; subface < SubfacesPerFace; subface++) {
    exahype2::dg::ader::stageHangingFaceFlux<TestSolver>(
      createFaceMarker(0, subface),
      stage,
      coarseFaceFlux.data(),
      fineFaceFlux.data()
    );
  }
  exahype2::dg::ader::restrictFaceFluxHalves<TestSolver>(stage, Restriction.data(), coarseFaceFlux.data());

  validateFalse(stage.hasFrame(coarseFaceFlux.data()));
  validateFalse(stage.hasFrame(coarseFaceFlux.data() + FaceSideValues));
  for (int value = 0; value < FaceSideValues; value++) {
    validateNumericalEquals(coarseFaceFlux[FaceSideValues + value], ConstantState);
  }
}

/**
 * Checks that a created child takes the state and the previous state of its parent.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testCreateCellProlongsBothBuffers() {
  std::array<double, CellValues> coarseCellState{};
  std::array<double, CellValues> coarsePreviousCellState{};
  std::array<double, CellValues> fineCellState{};
  std::array<double, CellValues> finePreviousCellState{};
  coarseCellState.fill(ConstantState);
  coarsePreviousCellState.fill(ConstantState);

  TestCellLabel coarseCellLabel;
  TestCellLabel fineCellLabel;
  coarseCellLabel.setTimeStamp(2.0);
  coarseCellLabel.setTimeStepSize(0.5);
  coarseCellLabel.setHasUpdated(true);

  exahype2::dg::ader::createCell<TestSolver>(
    createCellMarker(0),
    Prolongation.data(),
    coarseCellState.data(),
    fineCellState.data(),
    coarsePreviousCellState.data(),
    finePreviousCellState.data(),
    coarseCellLabel,
    fineCellLabel
  );

  for (int value = 0; value < CellValues; value++) {
    validateNumericalEquals(fineCellState[value], ConstantState);
    validateNumericalEquals(finePreviousCellState[value], ConstantState);
  }
  validateNumericalEquals(fineCellLabel.getTimeStamp(), 2.0);
  validateTrue(fineCellLabel.getHasUpdated());
  validateTrue(fineCellLabel.getRequiresCriterionEvaluation());
}

/**
 * Checks that a complete sibling set restricts onto its parent and flags the faces of the parent.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testDestroyCellRestrictsBothBuffersAndFlagsTheParentFaces() {
  exahype2::amr::ChildDataStage<double> cellStateStage(ChildrenPerCell, CellValues);
  exahype2::amr::ChildDataStage<double> previousCellStateStage(ChildrenPerCell, CellValues);

  std::array<double, CellValues> coarseCellState{};
  std::array<double, CellValues> coarsePreviousCellState{};
  std::array<double, CellValues> fineCellState{};
  std::array<double, CellValues> finePreviousCellState{};
  fineCellState.fill(ConstantState);
  finePreviousCellState.fill(ConstantState);

  std::array<TestFaceLabel, peano4::FacesPerCell> coarseFaceLabelRecords{};
  const auto                                      coarseFaceLabels = enumerate(coarseFaceLabelRecords);

  TestCellLabel coarseCellLabel;
  for (int child = 0; child < ChildrenPerCell; child++) {
    TestCellLabel fineCellLabel;
    fineCellLabel.setTimeStamp(4.0);
    fineCellLabel.setTimeStepSize(0.125);
    exahype2::dg::ader::destroyCell<TestSolver>(
      createCellMarker(child),
      cellStateStage,
      previousCellStateStage,
      Restriction.data(),
      coarseCellState.data(),
      fineCellState.data(),
      coarsePreviousCellState.data(),
      finePreviousCellState.data(),
      coarseFaceLabels,
      coarseCellLabel,
      fineCellLabel
    );
  }

  for (int value = 0; value < CellValues; value++) {
    validateNumericalEquals(coarseCellState[value], ConstantState);
    validateNumericalEquals(coarsePreviousCellState[value], ConstantState);
  }
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateTrue(coarseFaceLabels(face).getRequiresDataTransfer(ownSide));
  }
  validateNumericalEquals(coarseCellLabel.getTimeStamp(), 4.0);
  validateNumericalEquals(coarseCellLabel.getTimeStepSize(), 0.125);
}

/**
 * Checks that a limited solver clears the transfer flags without re-deriving anything.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testReleaseFacesAfterSubcellTransferClearsThePersistentFaces() {
  std::array<TestFaceLabel, peano4::FacesPerCell> faceLabelRecords{};
  const auto                                      faceLabels = enumerate(faceLabelRecords);
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    faceLabels(face).setRequiresDataTransfer(0, true);
    faceLabels(face).setRequiresDataTransfer(1, true);
  }

  TestCellLabel cellLabel;
  cellLabel.setRequiresCriterionEvaluation(true);

  exahype2::dg::ader::releaseFacesAfterSubcellTransfer(faceLabels, cellLabel);

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    validateFalse(faceLabels(face).getRequiresDataTransfer(ownSide));
    validateTrue(faceLabels(face).getRequiresDataTransfer(1 - ownSide));
  }
  validateFalse(cellLabel.getRequiresCriterionEvaluation());
}

/**
 * Checks that the re-derivation runs on a cell whose face awaits data and on no other.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testRederivePredictionWaitsForAFaceThatAwaitsData() {
  std::array<FaceState, peano4::FacesPerCell>     faceStates{};
  std::array<FaceFlux, peano4::FacesPerCell>      faceFluxes{};
  std::array<TestFaceLabel, peano4::FacesPerCell> faceLabelRecords{};
  const auto                                      faceStateEnumerator = enumerate(faceStates);
  const auto                                      faceFluxEnumerator  = enumerate(faceFluxes);
  const auto                                      faceLabels          = enumerate(faceLabelRecords);

  std::array<double, CellValues> cellState{};
  std::array<double, CellValues> previousCellState{};
  previousCellState.fill(ConstantState);

  TestCellLabel cellLabel;
  cellLabel.setTimeStamp(1.0);
  cellLabel.setTimeStepSize(0.25);
  cellLabel.setRequiresCriterionEvaluation(true);

  exahype2::dg::ader::rederivePrediction<TestSolver, true, false, false, false, false, false, false, false>(
    createCellMarker(0),
    cellLabel,
    faceLabels,
    faceStateEnumerator,
    faceFluxEnumerator,
    cellState.data(),
    previousCellState.data(),
    testTables(),
    testTables()
  );
  validateNumericalEquals(cellState[0], 0.0);
  validateFalse(cellLabel.getRequiresCriterionEvaluation());

  // The nonlinear predictor iterates on a right-hand side that every term of this solver
  // leaves at zero, so the state it starts from is its own fixed point.
  cellLabel.setRequiresCriterionEvaluation(true);
  faceLabels(0).setRequiresDataTransfer(1, true);
  exahype2::dg::ader::rederivePrediction<TestSolver, false, false, false, false, false, false, false, false>(
    createCellMarker(0),
    cellLabel,
    faceLabels,
    faceStateEnumerator,
    faceFluxEnumerator,
    cellState.data(),
    previousCellState.data(),
    testTables(),
    testTables()
  );
  validateNumericalEquals(cellState[0], ConstantState);
  validateFalse(faceLabels(0).getRequiresDataTransfer(1));
  validateFalse(cellLabel.getRequiresCriterionEvaluation());
}

/**
 * Checks that hanging-face solves receive the cell's trace, time stamp and physical boundary flag.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testHangingFacesTakeARederivedTraceAndARiemannSolve() {
  std::array<FaceState, peano4::FacesPerCell>     faceStates{};
  std::array<FaceFlux, peano4::FacesPerCell>      faceFluxes{};
  std::array<TestFaceLabel, peano4::FacesPerCell> faceLabelRecords{};
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    std::fill_n(faceStates[face].data(), 2 * FaceSideValues, UntouchedValue);
    std::fill_n(faceFluxes[face].data(), 2 * FaceSideValues, UntouchedValue);
  }
  const auto faceStateEnumerator = enumerate(faceStates);
  const auto faceFluxEnumerator  = enumerate(faceFluxes);
  const auto faceLabels          = enumerate(faceLabelRecords);

  std::array<double, CellValues> previousCellState{};
  previousCellState.fill(ConstantState);

  TestCellLabel cellLabel;
  cellLabel.setTimeStamp(1.0);
  cellLabel.setTimeStepSize(0.25);

  constexpr double PassedTimeStepSize  = 0.5;
  const auto       marker              = createCellMarker(0);
  int              riemannSolves       = 0;
  bool             expectedBoundary    = false;
  const auto       solveRiemannProblem = [&](
                                           double* const                                leftFluxEstimates,
                                           double* const                                rightFluxEstimates,
                                           double* const                                leftStateEstimates,
                                           double* const                                rightStateEstimates,
                                           const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
                                           const double                                 timeStamp,
                                           const double                                 timeStepSize,
                                           const int                                    direction,
                                           const bool                                   isBoundary,
                                           const int                                    faceNumber
                                         ) {
    riemannSolves++;
    validateEquals(faceNumber, 0);
    validateEquals(direction, 0);
    validateEquals(isBoundary, expectedBoundary);
    validateNumericalEquals(timeStamp, 1.0);
    validateNumericalEquals(timeStepSize, PassedTimeStepSize);
    validateNumericalEquals(faceCentre(0), marker.getX()(0) - 0.5 * marker.getH()(0));
    validateEquals(leftFluxEstimates, faceFluxes[0].data());
    validateEquals(rightFluxEstimates, faceFluxes[0].data() + FaceSideValues);
    validateEquals(leftStateEstimates, faceStates[0].data());
    validateEquals(rightStateEstimates, faceStates[0].data() + FaceSideValues);
  };
  const auto solveHangingFaces = [&]() {
    exahype2::dg::ader::
      solveRiemannProblemsOnHangingFaces<TestSolver, false, false, false, false, false, false, false, false>(
        marker,
        cellLabel,
        faceLabels,
        faceStateEnumerator,
        faceFluxEnumerator,
        previousCellState.data(),
        testTables(),
        testTables(),
        PassedTimeStepSize,
        solveRiemannProblem
      );
  };

  solveHangingFaces();
  validateEquals(riemannSolves, 0);
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int value = 0; value < 2 * FaceSideValues; value++) {
      validateNumericalEqualsWithParams2(faceStates[face].data()[value], UntouchedValue, face, value);
    }
  }

  // The lower face of the first axis hangs, so the cell owns the upper half of that face.
  faceLabelRecords[0].setIsHanging(true);
  solveHangingFaces();
  validateEquals(riemannSolves, 1);
  for (int value = 0; value < FaceSideValues; value++) {
    validateNumericalEqualsWithParams1(faceStates[0].data()[FaceSideValues + value], ConstantState, value);
    validateNumericalEqualsWithParams1(faceFluxes[0].data()[FaceSideValues + value], 0.0, value);
    validateNumericalEqualsWithParams1(faceStates[0].data()[value], UntouchedValue, value);
    validateNumericalEqualsWithParams1(faceFluxes[0].data()[value], UntouchedValue, value);
  }
  for (int face = 1; face < peano4::FacesPerCell; face++) {
    for (int value = 0; value < 2 * FaceSideValues; value++) {
      validateNumericalEqualsWithParams2(faceStates[face].data()[value], UntouchedValue, face, value);
      validateNumericalEqualsWithParams2(faceFluxes[face].data()[value], UntouchedValue, face, value);
    }
  }

  faceLabelRecords[0].setBoundary(true);
  expectedBoundary = true;
  solveHangingFaces();
  validateEquals(riemannSolves, 2);

  TestSolver::PeriodicBoundaries[0] = true;
  expectedBoundary                  = false;
  solveHangingFaces();
  validateEquals(riemannSolves, 3);
  TestSolver::PeriodicBoundaries.reset();
}

/**
 * Checks that the re-derivation waits while committed grid control events remain and skips a
 * cell whose time step size is zero.
 */
void exahype2::dg::ader::tests::DynamicAMRTest::testRederivePredictionWaitsForASettledMeshAndAnAdvancedCell() {
  std::array<FaceState, peano4::FacesPerCell>     faceStates{};
  std::array<FaceFlux, peano4::FacesPerCell>      faceFluxes{};
  std::array<TestFaceLabel, peano4::FacesPerCell> faceLabelRecords{};
  const auto                                      faceStateEnumerator = enumerate(faceStates);
  const auto                                      faceFluxEnumerator  = enumerate(faceFluxes);
  const auto                                      faceLabels          = enumerate(faceLabelRecords);
  faceLabels(0).setRequiresDataTransfer(1, true);

  std::array<double, CellValues> cellState{};
  std::array<double, CellValues> previousCellState{};
  previousCellState.fill(ConstantState);

  TestCellLabel cellLabel;
  cellLabel.setTimeStamp(1.0);
  cellLabel.setTimeStepSize(0.25);

  const auto rederive = [&]() {
    cellLabel.setRequiresCriterionEvaluation(true);
    exahype2::dg::ader::rederivePrediction<TestSolver, false, false, false, false, false, false, false, false>(
      createCellMarker(0),
      cellLabel,
      faceLabels,
      faceStateEnumerator,
      faceFluxEnumerator,
      cellState.data(),
      previousCellState.data(),
      testTables(),
      testTables()
    );
  };

  auto&                       service = exahype2::RefinementControlService::getInstance();
  exahype2::RefinementControl control;
  control.addCommand(
    tarch::la::Vector<DIMENSIONS, double>(0.5),
    tarch::la::Vector<DIMENSIONS, double>(1.0 / SubdivisionFactor),
    exahype2::RefinementCommand::Refine
  );
  service.merge(control);
  service.finishRefinementCriterionStep();
  validateTrue(service.hasGridControlEvents());

  rederive();
  validateNumericalEquals(cellState[0], 0.0);
  validateTrue(faceLabels(0).getRequiresDataTransfer(1));
  validateFalse(cellLabel.getRequiresCriterionEvaluation());

  service.realiseCommittedEventsWithoutSweep();
  cellLabel.setTimeStepSize(0.0);
  rederive();
  validateNumericalEquals(cellState[0], 0.0);
  validateTrue(faceLabels(0).getRequiresDataTransfer(1));
  validateFalse(cellLabel.getRequiresCriterionEvaluation());
}
