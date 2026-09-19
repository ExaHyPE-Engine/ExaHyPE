// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CorrectTest.h"

#include "exahype2/dg/ader/Correct.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 2;
  constexpr int    NumberOfUnknowns           = 1;
  constexpr int    NumberOfAuxiliaryVariables = 0;
  constexpr double CellExtent                 = 2.0;
  constexpr double TimeStamp                  = 0.25;
  constexpr double TimeStepSize               = 0.5;
  constexpr double WaveSpeed                  = 3.0;
  constexpr double OutsideValue               = 5.0;
  constexpr double UntouchedValue             = -7.0;
  constexpr double UserFlux                   = 4.5;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int StateElementsPerFace = CellLayout::NodesPerFace * CellLayout::NumberOfData;
  constexpr int FluxElementsPerFace  = CellLayout::NodesPerFace * NumberOfUnknowns;
  constexpr int CellStateSize        = CellLayout::NodesPerCell * CellLayout::NumberOfData;

  double stampSeenByTheSolver = 0.0;

  /**
   * Records the arguments of the last call of the solver's own Riemann solver.
   */
  struct RiemannSolverCall final {
    int    numberOfCalls{0};
    double timeStamp{0.0};
    double timeStepSize{0.0};
    int    direction{-1};
    bool   isBoundary{false};
    int    faceNumber{-1};
  };

  RiemannSolverCall userRiemannSolverCall{};

  /**
   * Returns a constant wave speed and a constant outside state. PeriodicFirstAxis makes the
   * first axis periodic.
   */
  template <bool PeriodicFirstAxis>
  struct RiemannSolver final {
    static constexpr bool PeriodicBoundaries[DIMENSIONS] = {PeriodicFirstAxis};

    static double maxEigenvalue(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const double                                                  timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      [[maybe_unused]] const int                                    direction
    ) {
      stampSeenByTheSolver = timeStamp;
      return WaveSpeed;
    }

    static void boundaryConditions(
      [[maybe_unused]] const double* NOALIAS const                  insideState,
      double* NOALIAS const                                         outsideState,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const int                                    direction
    ) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        outsideState[value] = OutsideValue;
      }
    }

    static void riemannSolver(
      double* NOALIAS const                                         leftFluxEstimates,
      double* NOALIAS const                                         rightFluxEstimates,
      [[maybe_unused]] const double* NOALIAS const                  leftStateEstimates,
      [[maybe_unused]] const double* NOALIAS const                  rightStateEstimates,
      const double                                                  timeStamp,
      const double                                                  timeStepSize,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      const int                                                     direction,
      const bool                                                    isBoundary,
      const int                                                     faceNumber
    ) {
      userRiemannSolverCall.numberOfCalls++;
      userRiemannSolverCall.timeStamp    = timeStamp;
      userRiemannSolverCall.timeStepSize = timeStepSize;
      userRiemannSolverCall.direction    = direction;
      userRiemannSolverCall.isBoundary   = isBoundary;
      userRiemannSolverCall.faceNumber   = faceNumber;
      for (int entry = 0; entry < FluxElementsPerFace; entry++) {
        leftFluxEstimates[entry]  = UserFlux;
        rightFluxEstimates[entry] = UserFlux;
      }
    }

    int    numberOfUpdates{0};
    int    numberOfEigenvalueReports{0};
    double reportedTimeStepSize{0.0};
    double reportedTimeStamp{0.0};
    double reportedMinCellSize{0.0};
    double reportedMaxCellSize{0.0};
    double maxPhysicalEigenvalue{0.0};
    double maxScaledEigenvalue{0.0};

    void update(
      const double                timeStepSize,
      const double                timeStamp,
      const double                minCellSize,
      const double                maxCellSize,
      [[maybe_unused]] const bool hasHangingFaces
    ) {
      numberOfUpdates++;
      reportedTimeStepSize = timeStepSize;
      reportedTimeStamp    = timeStamp;
      reportedMinCellSize  = minCellSize;
      reportedMaxCellSize  = maxCellSize;
    }

    void setMaxEigenvalue(const double value) {
      numberOfEigenvalueReports++;
      maxPhysicalEigenvalue = value;
    }

    void setMaxScaledEigenvalue(const double value) { maxScaledEigenvalue = value; }
  };

  using InteriorSolver = RiemannSolver<false>;
  using PeriodicSolver = RiemannSolver<true>;

  struct FaceLabel final {
    double newTimeStamp[2]{0.0, 0.0};
    double oldTimeStamp[2]{0.0, 0.0};
    bool   updated[2]{false, false};
    bool   aboveHanging[2]{false, false};
    bool   boundary{false};

    [[nodiscard]] double getNewTimeStamp(const int side) const { return newTimeStamp[side]; }
    [[nodiscard]] double getOldTimeStamp(const int side) const { return oldTimeStamp[side]; }
    [[nodiscard]] bool   getAboveHanging(const int side) const { return aboveHanging[side]; }
    [[nodiscard]] bool   getBoundary() const { return boundary; }

    void setNewTimeStamp(const int side, const double value) { newTimeStamp[side] = value; }
    void setOldTimeStamp(const int side, const double value) { oldTimeStamp[side] = value; }
    void setUpdated(const int side, const bool value) { updated[side] = value; }
  };

  struct CellLabel final {
    double timeStamp{TimeStamp};
    double timeStepSize{TimeStepSize};
    bool   hasUpdated{false};

    [[nodiscard]] double getTimeStamp() const { return timeStamp; }
    [[nodiscard]] double getTimeStepSize() const { return timeStepSize; }

    void setTimeStamp(const double value) { timeStamp = value; }
    void setHasUpdated(const bool value) { hasUpdated = value; }
  };

  template <int Values>
  struct FaceRecord final {
    static constexpr int Cardinality = Values;

    double values[Values]{};

    double* data() { return values; }
  };

  using FaceFlux = FaceRecord<2 * FluxElementsPerFace>;

  /**
   * Hands the templates the entities of one cell without the pooled containers of the traversal.
   */
  template <typename Element>
  struct Enumerator final {
    Element* elements[peano4::FacesPerCell]{};

    Element& operator()(const int index) const { return *elements[index]; }
  };

  /**
   * The nodes sit at the two ends of the reference interval, so a face integral reaches the nodes
   * of that face alone.
   */
  struct Tables final {
    double faceQuadratureWeights[CellLayout::NodesPerFace]{};
    double inverseCellQuadratureWeights[CellLayout::NodesPerCell]{};
    double quadraturePoints[NumberOfDofsPerAxis]{0.0, 1.0};
    double leftFaceCoefficients[NumberOfDofsPerAxis]{1.0, 0.0};
    double rightFaceCoefficients[NumberOfDofsPerAxis]{0.0, 1.0};
  };

  Tables makeTables() {
    Tables tables;
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      tables.faceQuadratureWeights[faceNode] = 1.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      tables.inverseCellQuadratureWeights[cellNode] = 1.0;
    }
    return tables;
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

  double lowerFluxEntry(const int direction, const int faceNode) { return 2.0 + direction + 0.5 * faceNode; }

  double upperFluxEntry(const int direction, const int faceNode) { return 8.0 + direction + 0.5 * faceNode; }

  peano4::dataview::FaceMarker makeFaceMarker(const int faceNumber, const bool cellHasBeenRefined) {
    peano4::dataview::FaceMarker marker;
    marker.h                  = tarch::la::Vector<DIMENSIONS, double>(CellExtent);
    marker.cellCentre         = tarch::la::Vector<DIMENSIONS, double>(0.5 * CellExtent);
    marker.cellHasBeenRefined = cellHasBeenRefined;
    return marker.select(faceNumber);
  }

  peano4::dataview::CellMarker makeCellMarker() {
    peano4::dataview::CellMarker marker;
    marker.h      = tarch::la::Vector<DIMENSIONS, double>(CellExtent);
    marker.centre = tarch::la::Vector<DIMENSIONS, double>(0.5 * CellExtent);
    return marker;
  }

  /**
   * Holds one face and hands out the enumerator of the coarse labels the face bodies read.
   */
  struct Face final {
    FaceLabel             faceLabel{};
    FaceLabel             coarseFaceLabels[peano4::FacesPerCell]{};
    double                faceState[2 * StateElementsPerFace]{};
    double                faceFlux[2 * FluxElementsPerFace]{};
    Enumerator<FaceLabel> coarseFaceLabelEnumerator{};
  };

  void fill(Face& face) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      face.faceState[faceNode]                        = 2.0 + faceNode;
      face.faceState[StateElementsPerFace + faceNode] = 1.0;
      face.faceFlux[faceNode]                         = 10.0 + faceNode;
      face.faceFlux[FluxElementsPerFace + faceNode]   = 20.0;
    }
    for (int index = 0; index < peano4::FacesPerCell; index++) {
      face.coarseFaceLabelEnumerator.elements[index] = &face.coarseFaceLabels[index];
    }
  }

  /**
   * Holds the cell entities the correction reads and hands out their enumerators.
   */
  struct Cell final {
    FaceLabel faceLabels[peano4::FacesPerCell]{};
    FaceFlux  faceFluxes[peano4::FacesPerCell]{};
    CellLabel cellLabel{};
    double    cellState[CellStateSize]{};

    Enumerator<FaceLabel> faceLabelEnumerator{};
    Enumerator<FaceFlux>  faceFluxEnumerator{};
  };

  void fill(Cell& cell) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      cell.cellState[cellNode * CellLayout::NumberOfData] = cellStateEntry(cellNode);
    }
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
        cell.faceFluxes[face].values[entry] = UntouchedValue;
      }
      cell.faceLabelEnumerator.elements[face] = &cell.faceLabels[face];
      cell.faceFluxEnumerator.elements[face]  = &cell.faceFluxes[face];
    }
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
        cell.faceFluxes[direction].values[FluxElementsPerFace + faceNode] = lowerFluxEntry(direction, faceNode);
        cell.faceFluxes[direction + DIMENSIONS].values[faceNode]          = upperFluxEntry(direction, faceNode);
      }
    }
  }

  /**
   * Writes the state the cell holds once the face integrals of both faces of every axis are in.
   *
   * A face integral of the lower face reaches the cell nodes on that face with the positive
   * quotient of the time step size and the cell extent, and the upper face with its negative.
   */
  void expectedCellState(double (&expected)[CellStateSize]) {
    constexpr double Contribution = TimeStepSize / CellExtent;
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      expected[cellNode * CellLayout::NumberOfData] = cellStateEntry(cellNode);
    }
    for (int direction = 0; direction < DIMENSIONS; direction++) {
      for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
        expected[cellNodeOnFace(direction, faceNode, 0) * CellLayout::NumberOfData] +=
          Contribution * lowerFluxEntry(direction, faceNode);
        expected[cellNodeOnFace(direction, faceNode, NumberOfDofsPerAxis - 1) * CellLayout::NumberOfData] -=
          Contribution * upperFluxEntry(direction, faceNode);
      }
    }
  }

  /**
   * Runs the correction of one cell and returns how often the postprocessing ran.
   */
  template <bool ReportEigenvalues>
  int correctCell(Cell& cell, InteriorSolver& solver, const Tables& tables) {
    int postprocessingCalls = 0;
    exahype2::dg::ader::correct<
      InteriorSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      false,
      ReportEigenvalues>(
      makeCellMarker(),
      solver,
      cell.cellLabel,
      cell.faceLabelEnumerator,
      cell.faceFluxEnumerator,
      cell.cellState,
      tables.faceQuadratureWeights,
      tables.inverseCellQuadratureWeights,
      tables.leftFaceCoefficients,
      tables.rightFaceCoefficients,
      tables.quadraturePoints,
      [&]() { postprocessingCalls++; }
    );
    return postprocessingCalls;
  }

  /**
   * Runs the Riemann solve of one face with the given solver and choice of Riemann kernel.
   */
  template <typename SolverType, bool HangingFace, bool IsLinear = false, bool UseRiemannSolverTerm = false>
  void solveFace(Face& face, const Tables& tables, const int faceNumber, const bool cellHasBeenRefined) {
    exahype2::dg::ader::solveRiemannProblemOnFace<
      SolverType,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      IsLinear,
      HangingFace,
      UseRiemannSolverTerm,
      false,
      false,
      false,
      false>(
      makeFaceMarker(faceNumber, cellHasBeenRefined),
      face.faceLabel,
      face.coarseFaceLabelEnumerator,
      face.faceState,
      face.faceFlux,
      tables.quadraturePoints,
      tables.faceQuadratureWeights,
      TimeStepSize
    );
  }
} // namespace

exahype2::dg::ader::tests::CorrectTest::CorrectTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::CorrectTest") {}

void exahype2::dg::ader::tests::CorrectTest::run() {
  testMethod(testTheRiemannSolverWritesBothFaceHalves);
  testMethod(testAHangingFaceReadsTheCoarseStampAndKeepsItsFlags);
  testMethod(testTheBoundaryFaceOfARefinedCellKeepsItsFluxes);
  testMethod(testHangingFaceStampsRiseToTheCoarseFace);
  testMethod(testTheFaceIntegralsReachTheCellState);
  testMethod(testAFaceAboveAHangingNeighbourIsReadFromTheOtherHalf);
  testMethod(testTheCorrectionCommitsTheTimeStep);
  testMethod(testAnUnrefinedBoundaryFaceTakesTheBoundaryState);
  testMethod(testAPeriodicBoundaryFaceIsSolvedAsAnInteriorFace);
  testMethod(testTheSolversRiemannSolverReceivesTheFace);
  testMethod(testTheLinearRiemannSolverWritesOppositeDissipations);
}

/**
 * An interior face receives the Rusanov flux in both halves and reports both halves as updated.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheRiemannSolverWritesBothFaceHalves() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.newTimeStamp[0] = 0.3;
  face.faceLabel.newTimeStamp[1] = 0.7;
  stampSeenByTheSolver           = 0.0;

  solveFace<InteriorSolver, false>(face, tables, 0, false);

  validateNumericalEquals(stampSeenByTheSolver, 0.7);
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    // The Rusanov flux averages the two projected fluxes and adds the jump times the wave speed.
    const double expected = 0.5 * ((10.0 + faceNode) + 20.0 + WaveSpeed * ((2.0 + faceNode) - 1.0));
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], expected, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FluxElementsPerFace + faceNode], expected, faceNode);
  }
  validateEquals(face.faceLabel.updated[0], true);
  validateEquals(face.faceLabel.updated[1], true);
}

/**
 * A hanging face takes the stamp of the coarse face it hangs on and keeps its update flags.
 */
void exahype2::dg::ader::tests::CorrectTest::testAHangingFaceReadsTheCoarseStampAndKeepsItsFlags() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.newTimeStamp[0]           = 0.4;
  face.faceLabel.newTimeStamp[1]           = 0.7;
  face.coarseFaceLabels[0].newTimeStamp[1] = 0.9;
  stampSeenByTheSolver                     = 0.0;

  solveFace<InteriorSolver, true>(face, tables, 0, false);

  validateNumericalEquals(stampSeenByTheSolver, 0.9);
  validateEquals(face.faceLabel.updated[0], false);
  validateEquals(face.faceLabel.updated[1], false);
}

/**
 * The boundary face of a refined cell holds no estimates, so its fluxes stay untouched.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheBoundaryFaceOfARefinedCellKeepsItsFluxes() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.boundary = true;

  solveFace<InteriorSolver, false>(face, tables, 0, true);

  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], 10.0 + faceNode, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FluxElementsPerFace + faceNode], 20.0, faceNode);
  }
  validateEquals(face.faceLabel.updated[0], false);
  validateEquals(face.faceLabel.updated[1], false);
}

/**
 * The four stamps of a hanging face rise to those of the coarse face.
 */
void exahype2::dg::ader::tests::CorrectTest::testHangingFaceStampsRiseToTheCoarseFace() {
  Face face;
  fill(face);
  face.faceLabel.newTimeStamp[0]           = 0.4;
  face.faceLabel.newTimeStamp[1]           = 0.9;
  face.faceLabel.oldTimeStamp[0]           = 0.1;
  face.faceLabel.oldTimeStamp[1]           = 0.5;
  face.coarseFaceLabels[1].newTimeStamp[0] = 0.6;
  face.coarseFaceLabels[1].newTimeStamp[1] = 0.2;
  face.coarseFaceLabels[1].oldTimeStamp[0] = 0.3;
  face.coarseFaceLabels[1].oldTimeStamp[1] = 0.7;

  exahype2::dg::ader::mergeHangingFaceTimeStamps(
    makeFaceMarker(1, false),
    face.faceLabel,
    face.coarseFaceLabelEnumerator
  );

  validateNumericalEquals(face.faceLabel.newTimeStamp[0], 0.6);
  validateNumericalEquals(face.faceLabel.newTimeStamp[1], 0.9);
  validateNumericalEquals(face.faceLabel.oldTimeStamp[0], 0.3);
  validateNumericalEquals(face.faceLabel.oldTimeStamp[1], 0.7);
}

/**
 * The face integrals of the two faces of every axis reach the nodes those faces carry.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheFaceIntegralsReachTheCellState() {
  const Tables   tables = makeTables();
  Cell           cell;
  InteriorSolver solver;
  fill(cell);

  double expected[CellStateSize] = {};
  expectedCellState(expected);

  validateEquals(correctCell<false>(cell, solver, tables), 1);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      cell.cellState[cellNode * CellLayout::NumberOfData],
      expected[cellNode * CellLayout::NumberOfData],
      cellNode
    );
  }
  validateEquals(solver.numberOfEigenvalueReports, 0);
}

/**
 * A face above a hanging face holds the Riemann flux in the other half.
 */
void exahype2::dg::ader::tests::CorrectTest::testAFaceAboveAHangingNeighbourIsReadFromTheOtherHalf() {
  const Tables   tables = makeTables();
  Cell           cell;
  InteriorSolver solver;
  fill(cell);

  // The neighbour of the lower face of the first axis hangs, so its Riemann flux sits in the
  // lower half of that face.
  constexpr double HangingFlux       = 1.5;
  cell.faceLabels[0].aboveHanging[0] = true;
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    cell.faceFluxes[0].values[faceNode] = HangingFlux;
  }

  double expected[CellStateSize] = {};
  expectedCellState(expected);
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    const int cellNode = cellNodeOnFace(0, faceNode, 0);
    expected[cellNode * CellLayout::NumberOfData] +=
      (TimeStepSize / CellExtent) * (HangingFlux - lowerFluxEntry(0, faceNode));
  }

  validateEquals(correctCell<false>(cell, solver, tables), 1);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    validateNumericalEqualsWithParams1(
      cell.cellState[cellNode * CellLayout::NumberOfData],
      expected[cellNode * CellLayout::NumberOfData],
      cellNode
    );
  }
}

/**
 * Checks that the correction evaluates wave speeds at the updated time stamp.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheCorrectionCommitsTheTimeStep() {
  const Tables   tables = makeTables();
  Cell           cell;
  InteriorSolver solver;
  fill(cell);

  validateEquals(correctCell<true>(cell, solver, tables), 1);

  validateEquals(solver.numberOfEigenvalueReports, 1);
  validateNumericalEquals(stampSeenByTheSolver, TimeStamp + TimeStepSize);
  validateNumericalEquals(solver.maxPhysicalEigenvalue, WaveSpeed);
  validateNumericalEquals(solver.maxScaledEigenvalue, DIMENSIONS * WaveSpeed / CellExtent);

  validateNumericalEquals(cell.cellLabel.timeStamp, TimeStamp + TimeStepSize);
  validateEquals(cell.cellLabel.hasUpdated, true);

  validateEquals(solver.numberOfUpdates, 1);
  validateNumericalEquals(solver.reportedTimeStepSize, TimeStepSize);
  validateNumericalEquals(solver.reportedTimeStamp, TimeStamp + TimeStepSize);
  validateNumericalEquals(solver.reportedMinCellSize, CellExtent);
  validateNumericalEquals(solver.reportedMaxCellSize, CellExtent);
}

/**
 * An unrefined boundary face takes the boundary state into its outside half, reads the inner
 * stamp and receives one numerical flux in both halves.
 */
void exahype2::dg::ader::tests::CorrectTest::testAnUnrefinedBoundaryFaceTakesTheBoundaryState() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.boundary        = true;
  face.faceLabel.newTimeStamp[0] = 0.8;
  face.faceLabel.newTimeStamp[1] = 0.6;
  stampSeenByTheSolver           = 0.0;

  solveFace<InteriorSolver, false>(face, tables, 0, false);

  validateNumericalEquals(stampSeenByTheSolver, 0.6);
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    validateNumericalEqualsWithParams1(face.faceState[faceNode], OutsideValue, faceNode);
    validateNumericalEqualsWithParams1(face.faceState[StateElementsPerFace + faceNode], 1.0, faceNode);
    validateNumericalEqualsWithParams1(
      face.faceFlux[faceNode],
      face.faceFlux[FluxElementsPerFace + faceNode],
      faceNode
    );
  }
  validateEquals(face.faceLabel.updated[0], true);
  validateEquals(face.faceLabel.updated[1], true);
}

/**
 * A boundary face of a periodic axis keeps both state halves, reads the later stamp of its halves
 * and receives the Rusanov flux of an interior face.
 */
void exahype2::dg::ader::tests::CorrectTest::testAPeriodicBoundaryFaceIsSolvedAsAnInteriorFace() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.boundary        = true;
  face.faceLabel.newTimeStamp[0] = 0.8;
  face.faceLabel.newTimeStamp[1] = 0.6;
  stampSeenByTheSolver           = 0.0;

  solveFace<PeriodicSolver, false>(face, tables, 0, false);

  validateNumericalEquals(stampSeenByTheSolver, 0.8);
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    const double expected = 0.5 * ((10.0 + faceNode) + 20.0 + WaveSpeed * ((2.0 + faceNode) - 1.0));
    validateNumericalEqualsWithParams1(face.faceState[faceNode], 2.0 + faceNode, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], expected, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FluxElementsPerFace + faceNode], expected, faceNode);
  }
}

/**
 * The solver's own Riemann solver receives the stamp, the step size, the direction, the boundary
 * flag and the number of the face it writes.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheSolversRiemannSolverReceivesTheFace() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.newTimeStamp[0] = 0.3;
  face.faceLabel.newTimeStamp[1] = 0.7;
  userRiemannSolverCall          = RiemannSolverCall{};

  constexpr int UpperFaceOfTheFirstAxis = DIMENSIONS;
  solveFace<InteriorSolver, false, false, true>(face, tables, UpperFaceOfTheFirstAxis, false);

  validateEquals(userRiemannSolverCall.numberOfCalls, 1);
  validateNumericalEquals(userRiemannSolverCall.timeStamp, 0.7);
  validateNumericalEquals(userRiemannSolverCall.timeStepSize, TimeStepSize);
  validateEquals(userRiemannSolverCall.direction, 0);
  validateEquals(userRiemannSolverCall.isBoundary, false);
  validateEquals(userRiemannSolverCall.faceNumber, UpperFaceOfTheFirstAxis);
  for (int entry = 0; entry < 2 * FluxElementsPerFace; entry++) {
    validateNumericalEqualsWithParams1(face.faceFlux[entry], UserFlux, entry);
  }
}

/**
 * The linear Riemann solver of a system without flux writes the dissipation of the state jump
 * with opposite signs into the two flux halves.
 */
void exahype2::dg::ader::tests::CorrectTest::testTheLinearRiemannSolverWritesOppositeDissipations() {
  const Tables tables = makeTables();
  Face         face;
  fill(face);
  face.faceLabel.newTimeStamp[0] = 0.3;
  face.faceLabel.newTimeStamp[1] = 0.7;
  stampSeenByTheSolver           = 0.0;

  solveFace<InteriorSolver, false, true>(face, tables, 0, false);

  validateNumericalEquals(stampSeenByTheSolver, 0.7);
  for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
    const double dissipation = WaveSpeed * 0.5 * (1.0 - (2.0 + faceNode));
    validateNumericalEqualsWithParams1(face.faceFlux[faceNode], -dissipation, faceNode);
    validateNumericalEqualsWithParams1(face.faceFlux[FluxElementsPerFace + faceNode], dissipation, faceNode);
  }
}
