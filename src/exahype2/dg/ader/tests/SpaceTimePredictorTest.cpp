// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpaceTimePredictorTest.h"

#include "exahype2/dg/ader/SpaceTimePredictor.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr double TimeStamp                  = 0.5;
  constexpr double TimeStepSize               = 0.25;
  constexpr double PicardTolerance            = 1.0e-8;
  constexpr double Slope                      = 0.5;

  constexpr double FluxSpeed               = 0.5;
  constexpr double DiffusionSpeed          = 0.25;
  constexpr double ProductSpeed            = 0.75;
  constexpr double SourceStrength          = 1.5;
  constexpr double DiffusiveSourceStrength = 0.5;
  constexpr double DivergingSpeed          = 1.0e30;
  constexpr double LinearFluxSpeed         = 0.5;
  constexpr double LinearProductSpeed      = 0.25;
  constexpr double LinearSourceRate        = 0.75;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  using InertScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    false,
    false>;
  using TermScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    true,
    true>;
  using FluxScratch = exahype2::dg::ader::NonlinearPredictorScratch<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    false,
    false>;
  using InertLinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, false, false, false>;
  using TransportLinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, false, false>;
  using SourceLinearScratch = exahype2::dg::ader::
    LinearPredictorScratch<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, true, true, false>;

  constexpr int AveragedAxisStride = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;

  // Gauss-Legendre nodes and weights of three points on the unit interval.
  const double QuadratureNodes[NumberOfDofsPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
  const double QuadratureWeights[NumberOfDofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  // Lagrange basis function of the given node evaluated at x.
  double basisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < NumberOfDofsPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  // Derivative of the Lagrange basis function of the given node at x.
  double basisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < NumberOfDofsPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < NumberOfDofsPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  void invertMatrix(
    const double (&matrix)[NumberOfDofsPerAxis * NumberOfDofsPerAxis],
    double (&inverse)[NumberOfDofsPerAxis * NumberOfDofsPerAxis]
  ) {
    constexpr int WorkColumns = 2 * NumberOfDofsPerAxis;

    double work[NumberOfDofsPerAxis][WorkColumns];
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        work[row][column]                       = matrix[row * NumberOfDofsPerAxis + column];
        work[row][NumberOfDofsPerAxis + column] = (row == column) ? 1.0 : 0.0;
      }
    }

    for (int pivot = 0; pivot < NumberOfDofsPerAxis; pivot++) {
      int largestRow = pivot;
      for (int row = pivot + 1; row < NumberOfDofsPerAxis; row++) {
        if (std::abs(work[row][pivot]) > std::abs(work[largestRow][pivot])) {
          largestRow = row;
        }
      }
      for (int column = 0; column < WorkColumns; column++) {
        const double swapped     = work[pivot][column];
        work[pivot][column]      = work[largestRow][column];
        work[largestRow][column] = swapped;
      }
      const double scale = 1.0 / work[pivot][pivot];
      for (int column = 0; column < WorkColumns; column++) {
        work[pivot][column] *= scale;
      }
      for (int row = 0; row < NumberOfDofsPerAxis; row++) {
        if (row != pivot) {
          const double factor = work[row][pivot];
          for (int column = 0; column < WorkColumns; column++) {
            work[row][column] -= factor * work[pivot][column];
          }
        }
      }
    }

    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        inverse[row * NumberOfDofsPerAxis + column] = work[row][NumberOfDofsPerAxis + column];
      }
    }
  }

  struct Tables final {
    double quadraturePoints[NumberOfDofsPerAxis]{};
    double quadratureWeights[NumberOfDofsPerAxis]{};
    double inverseQuadratureWeights[NumberOfDofsPerAxis]{};
    double cellQuadratureWeights[CellLayout::NodesPerCell]{};
    double inverseCellQuadratureWeights[CellLayout::NodesPerCell]{};
    double stiffnessOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
    double stiffnessOperatorTransposed[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
    double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
    double inversePicardOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
    double leftFaceCoefficients[NumberOfDofsPerAxis]{};
    double rightFaceCoefficients[NumberOfDofsPerAxis]{};
  };

  tarch::la::Vector<DIMENSIONS, int> nodeIndexOf(const int cellNode) {
    tarch::la::Vector<DIMENSIONS, int> nodeIndex;
    int                                remainder = cellNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
      remainder /= NumberOfDofsPerAxis;
    }
    return nodeIndex;
  }

  Tables makeTables() {
    Tables tables;
    for (int node = 0; node < NumberOfDofsPerAxis; node++) {
      tables.quadraturePoints[node]         = QuadratureNodes[node];
      tables.quadratureWeights[node]        = QuadratureWeights[node];
      tables.inverseQuadratureWeights[node] = 1.0 / QuadratureWeights[node];
      tables.leftFaceCoefficients[node]     = basisValue(0.0, node);
      tables.rightFaceCoefficients[node]    = basisValue(1.0, node);
    }
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        const double stiffness = QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row);
        tables.stiffnessOperator[row * NumberOfDofsPerAxis + column]           = stiffness;
        tables.stiffnessOperatorTransposed[column * NumberOfDofsPerAxis + row] = stiffness;
        tables.derivativeOperator[row * NumberOfDofsPerAxis + column] = basisDerivative(QuadratureNodes[row], column);
      }
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const auto nodeIndex = nodeIndexOf(cellNode);
      double     weight    = 1.0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        weight *= QuadratureWeights[nodeIndex(axis)];
      }
      tables.cellQuadratureWeights[cellNode]        = weight;
      tables.inverseCellQuadratureWeights[cellNode] = 1.0 / weight;
    }

    // The Picard system couples the value at the end of the step with the temporal stiffness.
    double picardOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        const int entry       = row * NumberOfDofsPerAxis + column;
        picardOperator[entry] = basisValue(1.0, row) * basisValue(1.0, column) - tables.stiffnessOperator[entry];
      }
    }
    invertMatrix(picardOperator, tables.inversePicardOperator);
    return tables;
  }

  exahype2::dg::ader::BasisTables<double> collect(const Tables& tables) {
    return exahype2::dg::ader::BasisTables<double>{
      .quadraturePoints             = tables.quadraturePoints,
      .quadratureWeights            = tables.quadratureWeights,
      .inverseQuadratureWeights     = tables.inverseQuadratureWeights,
      .cellQuadratureWeights        = tables.cellQuadratureWeights,
      .inverseCellQuadratureWeights = tables.inverseCellQuadratureWeights,
      .stiffnessOperator            = tables.stiffnessOperator,
      .stiffnessOperatorTransposed  = tables.stiffnessOperatorTransposed,
      .derivativeOperator           = tables.derivativeOperator,
      .inversePicardOperator        = tables.inversePicardOperator,
      .leftFaceCoefficients         = tables.leftFaceCoefficients,
      .rightFaceCoefficients        = tables.rightFaceCoefficients
    };
  }

  struct FaceBuffers final {
    double state[peano4::FacesPerCell][CellLayout::NodesPerFace * CellLayout::NumberOfData]{};
    double flux[peano4::FacesPerCell][CellLayout::NodesPerFace * NumberOfUnknowns]{};
  };

  void collectFacePointers(
    FaceBuffers& buffers,
    double* (&faceState)[peano4::FacesPerCell],
    double* (&faceFlux)[peano4::FacesPerCell]
  ) {
    for (int face = 0; face < peano4::FacesPerCell; face++) {
      faceState[face] = buffers.state[face];
      faceFlux[face]  = buffers.flux[face];
    }
  }

  // A face node enumerates the axes other than the normal one, the lowest of them running fastest.
  int cellNodeOnFace(const int direction, const int faceNode, const int normalIndex) {
    int nodeIndex[DIMENSIONS];
    int remainder = faceNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (axis == direction) {
        nodeIndex[axis] = normalIndex;
      } else {
        nodeIndex[axis] = remainder % NumberOfDofsPerAxis;
        remainder /= NumberOfDofsPerAxis;
      }
    }
    int cellNode = 0;
    for (int axis = DIMENSIONS - 1; axis >= 0; axis--) {
      cellNode = cellNode * NumberOfDofsPerAxis + nodeIndex[axis];
    }
    return cellNode;
  }

  tarch::la::Vector<DIMENSIONS, double> makeCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  double baseValue(const int value) { return 1.0 + 0.25 * value; }

  double varyingCellStateEntry(const int cellNode, const int value) { return 1.0 + 0.125 * (cellNode + value); }

  void fillVaryingCellState(double (&cellState)[CellLayout::NodesPerCell * CellLayout::NumberOfData]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        cellState[cellNode * CellLayout::NumberOfData + value] = varyingCellStateEntry(cellNode, value);
      }
    }
  }

  double linearCellStateEntry(const int cellNode, const int value, const double cellExtent) {
    return baseValue(value) + Slope * cellExtent * QuadratureNodes[nodeIndexOf(cellNode)(0)];
  }

  void fillLinearCellState(
    double (&cellState)[CellLayout::NodesPerCell * CellLayout::NumberOfData],
    const double cellExtent
  ) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        cellState[cellNode * CellLayout::NumberOfData + value] = linearCellStateEntry(cellNode, value, cellExtent);
      }
    }
  }

  struct TermSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] +=
          FluxSpeed * (1.0 + direction) * state[unknown] + DiffusionSpeed * stateGradients[direction][unknown];
      }
    }

    static void sourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += SourceStrength;
      }
    }

    static void diffusiveSourceTerm(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double sumOverAxes = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          sumOverAxes += stateGradient[axis * NumberOfUnknowns + unknown];
        }
        values[unknown] += DiffusiveSourceStrength * sumOverAxes;
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += ProductSpeed * (1.0 + direction) * stateGradient[unknown];
      }
    }
  };

  struct DivergingSolver final {
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
        values[unknown] += DivergingSpeed * state[unknown];
      }
    }
  };

  struct LinearSystemSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += LinearFluxSpeed * (1.0 + direction) * state[unknown];
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += LinearProductSpeed * (1.0 + direction) * stateGradient[unknown];
      }
    }

    static void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& position,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] += LinearSourceRate * state[unknown];
      }
    }
  };

  // The rate at which the terms of TermSolver move a profile of the test's slope.
  double nonlinearStateRate() {
    return -FluxSpeed * Slope + SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope;
  }

  double nonlinearSourceAverage() { return SourceStrength + (DiffusiveSourceStrength - ProductSpeed) * Slope; }

  /**
   * Taylor coefficients of the linear system, one offset and one physical slope per order and
   * unknown, together with the factor that coefficient enters the time average with.
   */
  struct TaylorCoefficients final {
    double offset[NumberOfDofsPerAxis][NumberOfUnknowns]{};
    double slope[NumberOfDofsPerAxis][NumberOfUnknowns]{};
    double factor[NumberOfDofsPerAxis]{};
  };

  TaylorCoefficients makeTaylorCoefficients(const double transportSpeed, const double sourceRate) {
    TaylorCoefficients coefficients;
    coefficients.factor[0] = 1.0;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      coefficients.offset[0][unknown] = baseValue(unknown);
      coefficients.slope[0][unknown]  = Slope;
    }
    double factor = 1.0;
    for (int order = 1; order < NumberOfDofsPerAxis; order++) {
      factor *= TimeStepSize / (order + 1);
      coefficients.factor[order] = factor;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        coefficients.offset[order][unknown] = sourceRate * coefficients.offset[order - 1][unknown]
          - transportSpeed * coefficients.slope[order - 1][unknown];
        coefficients.slope[order][unknown] = sourceRate * coefficients.slope[order - 1][unknown];
      }
    }
    return coefficients;
  }
} // namespace

exahype2::dg::ader::tests::SpaceTimePredictorTest::SpaceTimePredictorTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::SpaceTimePredictorTest") {}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::run() {
  testMethod(testAnInertNonlinearSystemLeavesTheCellAtRest);
  testMethod(testTheNonlinearPredictorMovesALinearProfileByTheExactAmount);
  testMethod(testADivergingNonlinearPredictorReportsMinusOne);
  testMethod(testAnInertLinearSystemLeavesTheCellAtRest);
  testMethod(testTheLinearPredictorTranslatesALinearProfileByTheExactAmount);
  testMethod(testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem);
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::testAnInertNonlinearSystemLeavesTheCellAtRest() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillVaryingCellState(cellState);

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double timeAveragedScratch[InertScratch::TimeAveragedSize] = {};
  double picardScratch[InertScratch::PicardSize]             = {};

  const int numberOfSweeps = exahype2::dg::ader::spaceTimePredictor<
    TermSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    false,
    false,
    double,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    timeAveragedScratch,
    picardScratch,
    basisTables,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    PicardTolerance
  );

  validateEquals(numberOfSweeps, 1);

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + value],
        varyingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
      validateNumericalEqualsWithParams2(
        timeAveragedScratch[InertScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + value],
        varyingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const double nodeValue = varyingCellStateEntry(cellNodeOnFace(direction, faceNode, normalIndex), value);
          lower += basisValue(0.0, normalIndex) * nodeValue;
          upper += basisValue(1.0, normalIndex) * nodeValue;
        }
        const int entry = faceNode * CellLayout::NumberOfData + value;
        validateNumericalEqualsWithParams3(faceState[direction][entry], lower, direction, faceNode, value);
        validateNumericalEqualsWithParams3(faceState[direction + DIMENSIONS][entry], upper, direction, faceNode, value);
      }
    }
  }

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEqualsWithParams2(faceFlux[face][entry], 0.0, face, entry);
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::testTheNonlinearPredictorMovesALinearProfileByTheExactAmount() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillLinearCellState(cellState, cellSize(0));

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double timeAveragedScratch[TermScratch::TimeAveragedSize] = {};
  double picardScratch[TermScratch::PicardSize]             = {};

  const int numberOfSweeps = exahype2::dg::ader::spaceTimePredictor<
    TermSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    true,
    true,
    double,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    timeAveragedScratch,
    picardScratch,
    basisTables,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    PicardTolerance
  );

  // The first sweep reaches the solution and the second one confirms it.
  validateEquals(numberOfSweeps, 2);

  const double rate          = nonlinearStateRate();
  const double sourceAverage = nonlinearSourceAverage();

  double averagedState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      const double moved = (value < NumberOfUnknowns) ? 0.5 * TimeStepSize * rate : 0.0;
      averagedState[cellNode * CellLayout::NumberOfData + value] =
        linearCellStateEntry(cellNode, value, cellSize(0)) + moved;
    }
  }

  double averagedFlux[DIMENSIONS][CellLayout::NodesPerCell * NumberOfUnknowns];
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        const double diffusion = (axis == 0) ? DiffusionSpeed * Slope : 0.0;
        averagedFlux[axis][cellNode * NumberOfUnknowns + unknown] =
          FluxSpeed * (1.0 + axis) * averagedState[cellNode * CellLayout::NumberOfData + unknown] + diffusion;
      }
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        timeAveragedScratch[TermScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + value],
        averagedState[cellNode * CellLayout::NumberOfData + value],
        cellNode,
        value
      );
    }
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        timeAveragedScratch
          [TermScratch::TimeAveragedSourceOffset + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
        sourceAverage,
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        validateNumericalEqualsWithParams3(
          timeAveragedScratch
            [TermScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride
             + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
          averagedFlux[axis][cellNode * NumberOfUnknowns + unknown],
          axis,
          cellNode,
          unknown
        );
      }
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int    cellNode  = cellNodeOnFace(direction, faceNode, normalIndex);
          const double nodeValue = averagedState[cellNode * CellLayout::NumberOfData + value];
          lower += basisValue(0.0, normalIndex) * nodeValue;
          upper += basisValue(1.0, normalIndex) * nodeValue;
        }
        const int entry = faceNode * CellLayout::NumberOfData + value;
        validateNumericalEqualsWithParams3(faceState[direction][entry], lower, direction, faceNode, value);
        validateNumericalEqualsWithParams3(faceState[direction + DIMENSIONS][entry], upper, direction, faceNode, value);
      }
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const int    cellNode  = cellNodeOnFace(direction, faceNode, normalIndex);
          const double nodeValue = averagedFlux[direction][cellNode * NumberOfUnknowns + unknown];
          lower += basisValue(0.0, normalIndex) * nodeValue;
          upper += basisValue(1.0, normalIndex) * nodeValue;
        }
        const int entry = faceNode * NumberOfUnknowns + unknown;
        validateNumericalEqualsWithParams3(faceFlux[direction][entry], lower, direction, faceNode, unknown);
        validateNumericalEqualsWithParams3(
          faceFlux[direction + DIMENSIONS][entry],
          upper,
          direction,
          faceNode,
          unknown
        );
      }
    }
  }

  // The volume integral lifts the flux at the two ends of every axis and subtracts the exact
  // divergence of the flux over the time step.
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int    firstAxisNode = nodeIndex(0);
      const double lowerFlux = FluxSpeed * (baseValue(unknown) + 0.5 * TimeStepSize * rate) + DiffusionSpeed * Slope;
      const double upperFlux =
        FluxSpeed * (baseValue(unknown) + Slope * cellSize(0) + 0.5 * TimeStepSize * rate) + DiffusionSpeed * Slope;
      double expected = linearCellStateEntry(cellNode, unknown, cellSize(0)) + TimeStepSize * sourceAverage
        - TimeStepSize * FluxSpeed * Slope
        + TimeStepSize * (basisValue(1.0, firstAxisNode) * upperFlux - basisValue(0.0, firstAxisNode) * lowerFlux)
          / (QuadratureWeights[firstAxisNode] * cellSize(0));
      for (int axis = 1; axis < DIMENSIONS; axis++) {
        const int node = nodeIndex(axis);
        expected += TimeStepSize * averagedFlux[axis][cellNode * NumberOfUnknowns + unknown]
          * (basisValue(1.0, node) - basisValue(0.0, node)) / (QuadratureWeights[node] * cellSize(axis));
      }
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + unknown],
        expected,
        cellNode,
        unknown
      );
    }
    for (int value = NumberOfUnknowns; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + value],
        linearCellStateEntry(cellNode, value, cellSize(0)),
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::testADivergingNonlinearPredictorReportsMinusOne() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillLinearCellState(cellState, cellSize(0));

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double timeAveragedScratch[FluxScratch::TimeAveragedSize] = {};
  double picardScratch[FluxScratch::PicardSize]             = {};

  const int numberOfSweeps = exahype2::dg::ader::spaceTimePredictor<
    DivergingSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    false,
    false,
    false,
    double,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    timeAveragedScratch,
    picardScratch,
    basisTables,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize,
    PicardTolerance
  );

  validateEquals(numberOfSweeps, -1);
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::testAnInertLinearSystemLeavesTheCellAtRest() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillVaryingCellState(cellState);

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double scratch[InertLinearScratch::Size] = {};

  exahype2::dg::ader::linearSpaceTimePredictor<
    LinearSystemSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    false,
    false,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int value = 0; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + value],
        varyingCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }

  for (int direction = 0; direction < DIMENSIONS; direction++) {
    for (int faceNode = 0; faceNode < CellLayout::NodesPerFace; faceNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        double lower = 0.0;
        double upper = 0.0;
        for (int normalIndex = 0; normalIndex < NumberOfDofsPerAxis; normalIndex++) {
          const double nodeValue = varyingCellStateEntry(cellNodeOnFace(direction, faceNode, normalIndex), value);
          lower += basisValue(0.0, normalIndex) * nodeValue;
          upper += basisValue(1.0, normalIndex) * nodeValue;
        }
        const int entry = faceNode * CellLayout::NumberOfData + value;
        validateNumericalEqualsWithParams3(faceState[direction][entry], lower, direction, faceNode, value);
        validateNumericalEqualsWithParams3(faceState[direction + DIMENSIONS][entry], upper, direction, faceNode, value);
      }
    }
  }

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    for (int entry = 0; entry < CellLayout::NodesPerFace * NumberOfUnknowns; entry++) {
      validateNumericalEqualsWithParams2(faceFlux[face][entry], 0.0, face, entry);
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::
  testTheLinearPredictorTranslatesALinearProfileByTheExactAmount() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillLinearCellState(cellState, cellSize(0));

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double scratch[TransportLinearScratch::Size] = {};

  exahype2::dg::ader::linearSpaceTimePredictor<
    LinearSystemSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    false,
    false,
    false,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  const double transportSpeed = LinearFluxSpeed + LinearProductSpeed;

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double expected =
        linearCellStateEntry(cellNode, unknown, cellSize(0)) - TimeStepSize * transportSpeed * Slope;
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + unknown],
        expected,
        cellNode,
        unknown
      );
    }
    for (int value = NumberOfUnknowns; value < CellLayout::NumberOfData; value++) {
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + value],
        linearCellStateEntry(cellNode, value, cellSize(0)),
        cellNode,
        value
      );
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double expected = (axis == 0) ? transportSpeed * Slope : 0.0;
        validateNumericalEqualsWithParams3(
          scratch
            [TransportLinearScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride
             + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
          expected,
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::SpaceTimePredictorTest::testTheLinearPredictorFollowsTheTaylorSeriesOfTheSystem() {
  const Tables                                tables      = makeTables();
  const auto                                  basisTables = collect(tables);
  const auto                                  cellSize    = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillLinearCellState(cellState, cellSize(0));

  FaceBuffers buffers;
  double*     faceState[peano4::FacesPerCell];
  double*     faceFlux[peano4::FacesPerCell];
  collectFacePointers(buffers, faceState, faceFlux);

  double scratch[SourceLinearScratch::Size] = {};

  exahype2::dg::ader::linearSpaceTimePredictor<
    LinearSystemSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    true,
    false,
    false,
    double,
    double,
    double>(
    faceState,
    faceFlux,
    cellState,
    scratch,
    nullptr,
    0,
    nullptr,
    basisTables,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  const double             transportSpeed = LinearFluxSpeed + LinearProductSpeed;
  const TaylorCoefficients coefficients   = makeTaylorCoefficients(transportSpeed, LinearSourceRate);

  double averagedState[CellLayout::NodesPerCell * NumberOfUnknowns];
  double averagedFlux[NumberOfUnknowns] = {};
  for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
    for (int order = 0; order < NumberOfDofsPerAxis; order++) {
      averagedFlux[unknown] += coefficients.factor[order] * transportSpeed * coefficients.slope[order][unknown];
    }
  }
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const double reference = QuadratureNodes[nodeIndexOf(cellNode)(0)];
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double average = 0.0;
      for (int order = 0; order < NumberOfDofsPerAxis; order++) {
        average += coefficients.factor[order]
          * (coefficients.offset[order][unknown] + coefficients.slope[order][unknown] * cellSize(0) * reference);
      }
      averagedState[cellNode * NumberOfUnknowns + unknown] = average;
    }
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double average = averagedState[cellNode * NumberOfUnknowns + unknown];
      validateNumericalEqualsWithParams2(
        scratch[SourceLinearScratch::TimeAveragedStateOffset + cellNode * CellLayout::PaddedNumberOfData + unknown],
        average,
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(
        scratch
          [SourceLinearScratch::TimeAveragedSourceOffset + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
        LinearSourceRate * average,
        cellNode,
        unknown
      );
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const double expectedFlux = (axis == 0) ? averagedFlux[unknown] : 0.0;
        validateNumericalEqualsWithParams3(
          scratch
            [SourceLinearScratch::TimeAveragedFluxOffset + axis * AveragedAxisStride
             + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown],
          expectedFlux,
          axis,
          cellNode,
          unknown
        );
      }
      const double expectedState = linearCellStateEntry(cellNode, unknown, cellSize(0))
        + TimeStepSize * (LinearSourceRate * average - averagedFlux[unknown]);
      validateNumericalEqualsWithParams2(
        cellState[cellNode * CellLayout::NumberOfData + unknown],
        expectedState,
        cellNode,
        unknown
      );
    }
  }
}
