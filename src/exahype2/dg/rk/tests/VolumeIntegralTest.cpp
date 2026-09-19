// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VolumeIntegralTest.h"

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/rk/VolumeIntegral.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr int    NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int    CellNodes                  = exahype2::dg::NodesPerCell<NumberOfDofsPerAxis>;
  constexpr double TimeStamp                  = 0.5;
  constexpr double TimeStepSize               = 0.125;

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

  struct Operators final {
    double massMatrixDiagonal[NumberOfDofsPerAxis]{};
    double stiffnessMatrix[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
    double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
  };

  // The stiffness entry (i, j) is the weight of node j times the derivative of
  // basis function i at node j, and the derivative entry (i, j) is the
  // derivative of basis function j at node i.
  Operators makeOperators() {
    Operators operators;
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      operators.massMatrixDiagonal[row] = QuadratureWeights[row];
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        operators.stiffnessMatrix[row * NumberOfDofsPerAxis + column] =
          QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row);
        operators.derivativeOperator[row * NumberOfDofsPerAxis + column] =
          basisDerivative(QuadratureNodes[row], column);
      }
    }
    return operators;
  }

  tarch::la::Vector<DIMENSIONS, double> makeCellSize() {
    tarch::la::Vector<DIMENSIONS, double> cellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSize(axis) = 0.5 + 0.25 * axis;
    }
    return cellSize;
  }

  tarch::la::Vector<DIMENSIONS, int> nodeIndexOf(const int cellNode) {
    tarch::la::Vector<DIMENSIONS, int> nodeIndex;
    int                                remainder = cellNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      nodeIndex(axis) = remainder % NumberOfDofsPerAxis;
      remainder /= NumberOfDofsPerAxis;
    }
    return nodeIndex;
  }

  // The flux along an axis is the state times one plus the axis number, the
  // source is the state times one plus the unknown number, and the
  // nonconservative product returns the gradient it is handed.
  struct StubSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      int                                                           direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = (1.0 + direction) * state[unknown];
      }
    }

    static void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] = (1.0 + unknown) * state[unknown];
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      [[maybe_unused]] int                                          direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] = stateGradient[unknown];
      }
    }
  };

  // The diffusive flux along every axis is the sum over the axes of one plus
  // the axis number times the gradient along that axis.
  struct DiffusiveStubSolver final {
    static void flux(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* const NOALIAS                                   stateGradients[DIMENSIONS],
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] double                                       timeStamp,
      [[maybe_unused]] double                                       timeStepSize,
      [[maybe_unused]] int                                          direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] = 0.0;
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          fluxValues[unknown] += (1.0 + axis) * stateGradients[axis][unknown];
        }
      }
    }
  };
} // namespace

exahype2::dg::rk::tests::VolumeIntegralTest::VolumeIntegralTest() :
  tarch::tests::TestCase("exahype2::dg::rk::tests::VolumeIntegralTest") {}

void exahype2::dg::rk::tests::VolumeIntegralTest::run() {
  testMethod(testWeakFormOfAConstantFluxLiftsTheBasisEndpoints);
  testMethod(testLinearFormDifferentiatesTheFlux);
  testMethod(testSourceTermIsScaledByTheTimeStep);
  testMethod(testNonconservativeProductSeesTheNodalGradient);
  testMethod(testDiffusiveFluxSeesTheNodalGradients);
}

void exahype2::dg::rk::tests::VolumeIntegralTest::testWeakFormOfAConstantFluxLiftsTheBasisEndpoints() {
  const Operators                             operators = makeOperators();
  const auto                                  cellSize  = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 2.0 + value;
    }
  }
  double cellUpdate[CellNodes * NumberOfUnknowns];
  for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
    cellUpdate[entry] = 9.0;
  }

  exahype2::dg::rk::volumeIntegral<
    StubSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    false,
    false,
    false>(
    cellState,
    cellUpdate,
    QuadratureNodes,
    operators.massMatrixDiagonal,
    operators.stiffnessMatrix,
    operators.derivativeOperator,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // The weak form integrates a constant flux against the basis derivative, which
  // leaves the difference of the basis function at the two ends of the axis,
  // divided by the node's quadrature weight and the cell extent.
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        const int    node = nodeIndex(direction);
        const double flux = (1.0 + direction) * (2.0 + unknown);
        expected += TimeStepSize * flux * (basisValue(1.0, node) - basisValue(0.0, node))
          / (QuadratureWeights[node] * cellSize(direction));
      }
      validateNumericalEqualsWithParams2(
        cellUpdate[cellNode * NumberOfUnknowns + unknown],
        expected,
        cellNode,
        unknown
      );
    }
  }
}

void exahype2::dg::rk::tests::VolumeIntegralTest::testLinearFormDifferentiatesTheFlux() {
  constexpr double Slope = 3.0;

  const Operators                             operators = makeOperators();
  const auto                                  cellSize  = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  // The state grows linearly along the first axis and is constant along the others.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const double reference = QuadratureNodes[nodeIndexOf(cellNode)(0)];
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 + value + Slope * reference;
    }
  }
  double cellUpdate[CellNodes * NumberOfUnknowns] = {};

  exahype2::dg::rk::volumeIntegral<
    StubSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    false,
    false,
    false>(
    cellState,
    cellUpdate,
    QuadratureNodes,
    operators.massMatrixDiagonal,
    operators.stiffnessMatrix,
    operators.derivativeOperator,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // The linear form subtracts the time step times the flux divergence, and only
  // the flux along the first axis varies, with the physical slope of the state
  // being the reference slope over the cell extent.
  const double expected = -TimeStepSize * Slope / cellSize(0);
  for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
    validateNumericalEqualsWithParams1(cellUpdate[entry], expected, entry);
  }
}

void exahype2::dg::rk::tests::VolumeIntegralTest::testSourceTermIsScaledByTheTimeStep() {
  const Operators                             operators = makeOperators();
  const auto                                  cellSize  = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  double cellState[CellNodes * NumberOfData];
  for (int entry = 0; entry < CellNodes * NumberOfData; entry++) {
    cellState[entry] = 0.5 + 0.25 * entry;
  }
  double cellUpdate[CellNodes * NumberOfUnknowns];
  for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
    cellUpdate[entry] = 9.0;
  }

  exahype2::dg::rk::volumeIntegral<
    StubSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    true,
    false>(
    cellState,
    cellUpdate,
    QuadratureNodes,
    operators.massMatrixDiagonal,
    operators.stiffnessMatrix,
    operators.derivativeOperator,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // Without a flux the update starts from zero, so it holds the source alone.
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double expected = TimeStepSize * (1.0 + unknown) * cellState[cellNode * NumberOfData + unknown];
      validateNumericalEqualsWithParams2(
        cellUpdate[cellNode * NumberOfUnknowns + unknown],
        expected,
        cellNode,
        unknown
      );
    }
  }
}

void exahype2::dg::rk::tests::VolumeIntegralTest::testNonconservativeProductSeesTheNodalGradient() {
  constexpr double Slope = 3.0;

  const Operators                             operators = makeOperators();
  const auto                                  cellSize  = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  // The state grows linearly along the last axis and is constant along the others.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const double reference = QuadratureNodes[nodeIndexOf(cellNode)(DIMENSIONS - 1)];
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 + value + Slope * reference;
    }
  }
  double cellUpdate[CellNodes * NumberOfUnknowns] = {};

  exahype2::dg::rk::volumeIntegral<
    StubSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    false,
    false,
    false,
    true>(
    cellState,
    cellUpdate,
    QuadratureNodes,
    operators.massMatrixDiagonal,
    operators.stiffnessMatrix,
    operators.derivativeOperator,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // The stub returns the gradient, which is the physical slope along the last
  // axis and zero along the others, and the kernel subtracts it times the time step.
  const double expected = -TimeStepSize * Slope / cellSize(DIMENSIONS - 1);
  for (int entry = 0; entry < CellNodes * NumberOfUnknowns; entry++) {
    validateNumericalEqualsWithParams1(cellUpdate[entry], expected, entry);
  }
}

void exahype2::dg::rk::tests::VolumeIntegralTest::testDiffusiveFluxSeesTheNodalGradients() {
  constexpr double Slope = 3.0;

  const Operators                             operators = makeOperators();
  const auto                                  cellSize  = makeCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);

  // The state grows linearly along the last axis and is constant along the others.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const double reference = QuadratureNodes[nodeIndexOf(cellNode)(DIMENSIONS - 1)];
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = 1.0 + value + Slope * reference;
    }
  }
  double cellUpdate[CellNodes * NumberOfUnknowns] = {};

  exahype2::dg::rk::volumeIntegral<
    DiffusiveStubSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    true,
    false,
    false>(
    cellState,
    cellUpdate,
    QuadratureNodes,
    operators.massMatrixDiagonal,
    operators.stiffnessMatrix,
    operators.derivativeOperator,
    cellCentre,
    cellSize,
    TimeStamp,
    TimeStepSize
  );

  // Only the gradient along the last axis is non-zero, so the stub returns the
  // same constant flux along every axis, and the weak form lifts the basis
  // endpoints of every axis with it.
  const double flux = DIMENSIONS * Slope / cellSize(DIMENSIONS - 1);
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = 0.0;
      for (int direction = 0; direction < DIMENSIONS; direction++) {
        const int node = nodeIndex(direction);
        expected += TimeStepSize * flux * (basisValue(1.0, node) - basisValue(0.0, node))
          / (QuadratureWeights[node] * cellSize(direction));
      }
      validateNumericalEqualsWithParams2(
        cellUpdate[cellNode * NumberOfUnknowns + unknown],
        expected,
        cellNode,
        unknown
      );
    }
  }
}
