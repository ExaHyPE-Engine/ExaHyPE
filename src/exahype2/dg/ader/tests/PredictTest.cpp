// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PredictTest.h"

#include "exahype2/dg/ader/Predict.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 2;
  constexpr int    NumberOfUnknowns           = 1;
  constexpr int    NumberOfAuxiliaryVariables = 0;
  constexpr double TimeStamp                  = 0.25;
  constexpr double TimeStepSize               = 0.5;
  constexpr double CellExtent                 = 1.0;
  constexpr double UntouchedValue             = -7.0;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateElementsPerFace = CellLayout::NodesPerFace * CellLayout::NumberOfData;
  constexpr int FluxElementsPerFace  = CellLayout::NodesPerFace * NumberOfUnknowns;
  constexpr int CellStateSize        = CellLayout::NodesPerCell * CellLayout::NumberOfData;

  /**
   * The nodes sit at the two ends of the reference interval, so the value of a face node is the
   * value of the cell node that lies on that face.
   */
  struct Tables final {
    double quadraturePoints[NumberOfDofsPerAxis]{0.0, 1.0};
    double quadratureWeights[NumberOfDofsPerAxis]{0.5, 0.5};
    double inverseQuadratureWeights[NumberOfDofsPerAxis]{2.0, 2.0};
    double cellQuadratureWeights[CellLayout::NodesPerCell]{};
    double inverseCellQuadratureWeights[CellLayout::NodesPerCell]{};
    double stiffnessOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{-0.5, -0.5, 0.5, 0.5};
    double stiffnessOperatorTransposed[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{-0.5, 0.5, -0.5, 0.5};
    double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{-1.0, 1.0, -1.0, 1.0};
    double inversePicardOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{1.0, 0.0, 0.0, 1.0};
    double leftFaceCoefficients[NumberOfDofsPerAxis]{1.0, 0.0};
    double rightFaceCoefficients[NumberOfDofsPerAxis]{0.0, 1.0};
  };

  Tables makeTables() {
    Tables           tables;
    constexpr double CellWeight = (DIMENSIONS == 3) ? 0.125 : 0.25;
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      tables.cellQuadratureWeights[cellNode]        = CellWeight;
      tables.inverseCellQuadratureWeights[cellNode] = 1.0 / CellWeight;
    }
    return tables;
  }

  exahype2::dg::ader::BasisTables<double> collect(const Tables& tables) {
    return exahype2::dg::ader::BasisTables<double>{
      tables.quadraturePoints,
      tables.quadratureWeights,
      tables.inverseQuadratureWeights,
      tables.cellQuadratureWeights,
      tables.inverseCellQuadratureWeights,
      tables.stiffnessOperator,
      tables.stiffnessOperatorTransposed,
      tables.derivativeOperator,
      tables.inversePicardOperator,
      tables.leftFaceCoefficients,
      tables.rightFaceCoefficients
    };
  }

  /**
   * Returns the cell node that carries the given face node at the given position along the normal.
   */
  int cellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    int lowerAxesStride = 1;
    for (int axis = 0; axis < direction; axis++) {
      lowerAxesStride *= NumberOfDofsPerAxis;
    }
    const int lowerAxesIndex  = faceNode % lowerAxesStride;
    const int higherAxesIndex = faceNode / lowerAxesStride;
    return (higherAxesIndex * NumberOfDofsPerAxis + normalIndex) * lowerAxesStride + lowerAxesIndex;
  }

  double cellStateEntry(const int cellNode) { return 1.0 + 0.25 * cellNode; }

  template <int Values>
  struct FaceRecord final {
    static constexpr int Cardinality = Values;

    double values[Values]{};

    double* data() { return values; }
  };

  using FaceState = FaceRecord<2 * StateElementsPerFace>;
  using FaceFlux  = FaceRecord<2 * FluxElementsPerFace>;

  struct FaceLabel final {
    double newTimeStamp[2]{0.0, 0.0};
    bool   updated[2]{false, false};
    bool   requiresDataTransfer[2]{true, true};
    bool   aboveHanging[2]{false, false};
    bool   isHanging{false};

    [[nodiscard]] bool getAboveHanging(const int side) const { return aboveHanging[side]; }
    [[nodiscard]] bool getIsHanging() const { return isHanging; }

    void setNewTimeStamp(const int side, const double value) { newTimeStamp[side] = value; }
    void setUpdated(const int side, const bool value) { updated[side] = value; }
    void setRequiresDataTransfer(const int side, const bool value) { requiresDataTransfer[side] = value; }
  };

  struct CellLabel final {
    double timeStamp{TimeStamp};
    double timeStepSize{0.0};
    bool   predictorDiverged{false};

    [[nodiscard]] double getTimeStamp() const { return timeStamp; }

    void setTimeStepSize(const double value) { timeStepSize = value; }
    void setPredictorDiverged(const bool value) { predictorDiverged = value; }
  };

  /**
   * Hands the templates the entities of one cell without the pooled containers of the traversal.
   */
  template <typename Element>
  struct Enumerator final {
    Element* elements[peano4::FacesPerCell]{};

    Element& operator()(const int index) const { return *elements[index]; }
  };

  /**
   * Keeps what the prediction reports about the cell it ran on.
   */
  struct SolverReport {
    int    numberOfUpdates{0};
    double reportedTimeStepSize{0.0};
    double reportedTimeStamp{0.0};
    double reportedMinCellSize{0.0};
    double reportedMaxCellSize{0.0};
    bool   reportedHangingFaces{false};

    void update(
      const double timeStepSize,
      const double timeStamp,
      const double minCellSize,
      const double maxCellSize,
      const bool   hasHangingFaces
    ) {
      numberOfUpdates++;
      reportedTimeStepSize = timeStepSize;
      reportedTimeStamp    = timeStamp;
      reportedMinCellSize  = minCellSize;
      reportedMaxCellSize  = maxCellSize;
      reportedHangingFaces = hasHangingFaces;
    }
  };

  struct InertLinearSolver final : SolverReport {};

  /**
   * Writes a finite flux that moves the first Picard iterate past the divergence bound.
   */
  struct DivergingLimitedSolver final : SolverReport {
    static constexpr bool   UsesSubcellLimiter      = true;
    static constexpr double PicardResidualTolerance = 1.0e-10;
    static constexpr double FluxScaling             = 1.0e30;

    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] = FluxScaling * state[unknown];
      }
    }
  };

  /**
   * Counts the calls of the point-source term per source.
   */
  struct PointSourceSolver final : SolverReport {
    static constexpr int NumberOfPointSources = 2;

    static inline double pointSourceLocation[NumberOfPointSources][DIMENSIONS]{};
    static inline int    pointSourceCalls[NumberOfPointSources]{};

    static void pointSource(
      [[maybe_unused]] const double* const state,
      [[maybe_unused]] const double* const position,
      [[maybe_unused]] const double        timeStamp,
      [[maybe_unused]] const double        timeStepSize,
      double* const                        forceVector,
      const int                            sourceIndex
    ) {
      pointSourceCalls[sourceIndex]++;
      forceVector[0] = 1.0;
    }
  };

  /**
   * Holds the entities of one cell and hands out the enumerators the templates take.
   */
  struct Cell final {
    FaceLabel faceLabels[peano4::FacesPerCell]{};
    FaceState faceStates[peano4::FacesPerCell]{};
    FaceFlux  faceFluxes[peano4::FacesPerCell]{};
    CellLabel cellLabel{};
    double    cellState[CellStateSize]{};
    double    previousCellState[CellStateSize]{};

    Enumerator<FaceLabel> faceLabelEnumerator{};
    Enumerator<FaceState> faceStateEnumerator{};
    Enumerator<FaceFlux>  faceFluxEnumerator{};
  };

  void fill(Cell& cell) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cell.cellState[cellNode * CellLayout::NumberOfData]         = cellStateEntry(cellNode);
      cell.previousCellState[cellNode * CellLayout::NumberOfData] = UntouchedValue;
    }
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      for (int entry = 0; entry < 2 * StateElementsPerFace; entry++) {
        cell.faceStates[face].values[entry] = UntouchedValue;
      }
      for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
        cell.faceFluxes[face].values[entry] = UntouchedValue;
      }
      cell.faceLabelEnumerator.elements[face] = &cell.faceLabels[face];
      cell.faceStateEnumerator.elements[face] = &cell.faceStates[face];
      cell.faceFluxEnumerator.elements[face]  = &cell.faceFluxes[face];
    }
  }

  peano4::dataview::CellMarker makeCellMarker() {
    peano4::dataview::CellMarker marker;
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellExtent);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(0.5 * CellExtent);
    return marker;
  }

  /**
   * Runs the prediction of one cell of a linear system whose terms are all switched off.
   */
  void predictInertCell(Cell& cell, InertLinearSolver& solver, const Tables& tables) {
    exahype2::dg::ader::predict<
      InertLinearSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      false>(
      makeCellMarker(),
      solver,
      cell.cellLabel,
      cell.faceLabelEnumerator,
      cell.faceStateEnumerator,
      cell.faceFluxEnumerator,
      cell.cellState,
      cell.previousCellState,
      collect(tables),
      collect(tables),
      TimeStepSize
    );
  }

  /**
   * Runs the prediction of one cell of a linear system whose only term is its point sources.
   */
  void predictPointSourceCell(Cell& cell, PointSourceSolver& solver, const Tables& tables) {
    exahype2::dg::ader::predict<
      PointSourceSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      true,
      false,
      false,
      false,
      false,
      false,
      false,
      true>(
      makeCellMarker(),
      solver,
      cell.cellLabel,
      cell.faceLabelEnumerator,
      cell.faceStateEnumerator,
      cell.faceFluxEnumerator,
      cell.cellState,
      cell.previousCellState,
      collect(tables),
      collect(tables),
      TimeStepSize
    );
  }

  /**
   * Places the two point sources on the diagonal of the cell at the given fractions of its extent
   * and clears their call counts.
   */
  void placePointSources(const double firstFraction, const double secondFraction) {
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      PointSourceSolver::pointSourceLocation[0][axis] = firstFraction * CellExtent;
      PointSourceSolver::pointSourceLocation[1][axis] = secondFraction * CellExtent;
    }
    PointSourceSolver::pointSourceCalls[0] = 0;
    PointSourceSolver::pointSourceCalls[1] = 0;
  }
} // namespace

exahype2::dg::ader::tests::PredictTest::PredictTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::PredictTest") {}

void exahype2::dg::ader::tests::PredictTest::run() {
  testMethod(testThePredictionCopiesTheCellStateIntoThePreviousBuffer);
  testMethod(testTheEstimatesReachTheCellsOwnFaceHalves);
  testMethod(testThePredictionStampsTheCellsOwnFaceHalves);
  testMethod(testTheCellReportsItselfToTheSolver);
  testMethod(testADivergedPredictorFlagsTheCell);
  testMethod(testThePredictorEvaluatesOnlyThePointSourcesInsideTheCell);
}

/**
 * The prediction copies the cell state into the previous buffer.
 */
void exahype2::dg::ader::tests::PredictTest::testThePredictionCopiesTheCellStateIntoThePreviousBuffer() {
  const Tables      tables = makeTables();
  Cell              cell;
  InertLinearSolver solver;
  fill(cell);

  predictInertCell(cell, solver, tables);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      cell.previousCellState[cellNode * CellLayout::NumberOfData],
      cellStateEntry(cellNode),
      cellNode
    );
  }
}

/**
 * The lower faces of the cell receive its estimates in their upper half and the upper faces in
 * their lower half, and the opposite half of every face keeps its value.
 */
void exahype2::dg::ader::tests::PredictTest::testTheEstimatesReachTheCellsOwnFaceHalves() {
  const Tables      tables = makeTables();
  Cell              cell;
  InertLinearSolver solver;
  fill(cell);

  predictInertCell(cell, solver, tables);

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      const double lower = cellStateEntry(cellNodeOnFace(direction, faceNode, 0));
      const double upper = cellStateEntry(cellNodeOnFace(direction, faceNode, NumberOfDofsPerAxis - 1));

      validateNumericalEqualsWithParams2(
        cell.faceStates[direction].values[StateElementsPerFace + faceNode * CellLayout::NumberOfData],
        lower,
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        cell.faceStates[direction].values[faceNode * CellLayout::NumberOfData],
        UntouchedValue,
        direction,
        faceNode
      );

      validateNumericalEqualsWithParams2(
        cell.faceStates[direction + DIMENSIONS].values[faceNode * CellLayout::NumberOfData],
        upper,
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        cell.faceStates[direction + DIMENSIONS].values[StateElementsPerFace + faceNode * CellLayout::NumberOfData],
        UntouchedValue,
        direction,
        faceNode
      );

      // The system evaluates no flux, so the cell's own half of the face flux stays zero.
      validateNumericalEqualsWithParams2(
        cell.faceFluxes[direction].values[FluxElementsPerFace + faceNode * NumberOfUnknowns],
        0.0,
        direction,
        faceNode
      );
      validateNumericalEqualsWithParams2(
        cell.faceFluxes[direction + DIMENSIONS].values[faceNode * NumberOfUnknowns],
        0.0,
        direction,
        faceNode
      );
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      cell.cellState[cellNode * CellLayout::NumberOfData],
      cellStateEntry(cellNode),
      cellNode
    );
  }
}

/**
 * The prediction stamps the cell's own face halves and clears their transfer flags.
 */
void exahype2::dg::ader::tests::PredictTest::testThePredictionStampsTheCellsOwnFaceHalves() {
  const Tables      tables = makeTables();
  Cell              cell;
  InertLinearSolver solver;
  fill(cell);

  predictInertCell(cell, solver, tables);

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    validateNumericalEqualsWithParams1(cell.faceLabels[direction].newTimeStamp[1], TimeStamp, direction);
    validateNumericalEqualsWithParams1(cell.faceLabels[direction].newTimeStamp[0], 0.0, direction);
    validateEqualsWithParams1(cell.faceLabels[direction].updated[1], true, direction);
    validateEqualsWithParams1(cell.faceLabels[direction].updated[0], false, direction);
    validateEqualsWithParams1(cell.faceLabels[direction].requiresDataTransfer[1], false, direction);
    validateEqualsWithParams1(cell.faceLabels[direction].requiresDataTransfer[0], true, direction);

    validateNumericalEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].newTimeStamp[0], TimeStamp, direction);
    validateNumericalEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].newTimeStamp[1], 0.0, direction);
    validateEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].updated[0], true, direction);
    validateEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].updated[1], false, direction);
    validateEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].requiresDataTransfer[0], false, direction);
    validateEqualsWithParams1(cell.faceLabels[direction + DIMENSIONS].requiresDataTransfer[1], true, direction);
  }
}

/**
 * The cell reports its step size, its stamp, its extents and its hanging-face flag to the solver.
 */
void exahype2::dg::ader::tests::PredictTest::testTheCellReportsItselfToTheSolver() {
  const Tables      tables = makeTables();
  Cell              cell;
  InertLinearSolver solver;
  fill(cell);

  predictInertCell(cell, solver, tables);

  validateNumericalEquals(cell.cellLabel.timeStepSize, TimeStepSize);
  validateEquals(cell.cellLabel.predictorDiverged, false);
  validateEquals(solver.numberOfUpdates, 1);
  validateNumericalEquals(solver.reportedTimeStepSize, TimeStepSize);
  validateNumericalEquals(solver.reportedTimeStamp, TimeStamp);
  validateNumericalEquals(solver.reportedMinCellSize, CellExtent);
  validateNumericalEquals(solver.reportedMaxCellSize, CellExtent);
  validateEquals(solver.reportedHangingFaces, false);

  Cell              hangingCell;
  InertLinearSolver hangingSolver;
  fill(hangingCell);
  hangingCell.faceLabels[DIMENSIONS].aboveHanging[1] = true;

  predictInertCell(hangingCell, hangingSolver, tables);
  validateEquals(hangingSolver.reportedHangingFaces, true);
}

/**
 * A diverged predictor flags the cell for the limiter.
 */
void exahype2::dg::ader::tests::PredictTest::testADivergedPredictorFlagsTheCell() {
  const Tables           tables = makeTables();
  Cell                   cell;
  DivergingLimitedSolver solver;
  fill(cell);

  exahype2::dg::ader::predict<
    DivergingLimitedSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    false,
    false,
    false,
    false,
    false,
    false>(
    makeCellMarker(),
    solver,
    cell.cellLabel,
    cell.faceLabelEnumerator,
    cell.faceStateEnumerator,
    cell.faceFluxEnumerator,
    cell.cellState,
    cell.previousCellState,
    collect(tables),
    collect(tables),
    TimeStepSize
  );

  validateEquals(cell.cellLabel.predictorDiverged, true);
  validateEquals(solver.numberOfUpdates, 1);
}

/**
 * The predictor evaluates the point source inside the cell and no point source outside it.
 */
void exahype2::dg::ader::tests::PredictTest::testThePredictorEvaluatesOnlyThePointSourcesInsideTheCell() {
  const Tables      tables = makeTables();
  PointSourceSolver solver;

  Cell cellWithASource;
  fill(cellWithASource);
  placePointSources(0.25, 2.0);
  predictPointSourceCell(cellWithASource, solver, tables);
  validateTrue(PointSourceSolver::pointSourceCalls[0] > 0);
  validateEquals(PointSourceSolver::pointSourceCalls[1], 0);

  Cell cellWithoutASource;
  fill(cellWithoutASource);
  placePointSources(2.0, 3.0);
  predictPointSourceCell(cellWithoutASource, solver, tables);
  validateEquals(PointSourceSolver::pointSourceCalls[0], 0);
  validateEquals(PointSourceSolver::pointSourceCalls[1], 0);
  validateEquals(solver.numberOfUpdates, 2);
}
