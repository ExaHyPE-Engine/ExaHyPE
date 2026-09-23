// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LinearPredictorTermsTest.h"

#include "exahype2/dg/ader/LinearPredictorTerms.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr double TimeStamp                  = 0.5;
  constexpr double TimeStepSize               = 0.25;
  constexpr double Slope                      = 3.0;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int AxisStride   = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;
  constexpr int StateSize    = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfData;
  constexpr int GradientSize = NumberOfDofsPerAxis * CellLayout::PaddedNumberOfUnknowns;

  // Gauss-Legendre nodes of three points on the unit interval.
  const double QuadratureNodes[NumberOfDofsPerAxis] = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};

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

  void makeDerivativeOperator(double (&derivativeOperator)[NumberOfDofsPerAxis * NumberOfDofsPerAxis]) {
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        derivativeOperator[row * NumberOfDofsPerAxis + column] = basisDerivative(QuadratureNodes[row], column);
      }
    }
  }

  tarch::la::Vector<DIMENSIONS, double> makeInverseCellSize() {
    tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      inverseCellSize(axis) = 1.0 / (0.5 + 0.25 * axis);
    }
    return inverseCellSize;
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

  struct MarkerSolver final {
    static void flux(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         fluxValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        fluxValues[unknown] += (1.0 + direction) * state[unknown];
      }
    }

    static void nonconservativeProduct(
      [[maybe_unused]] const double* NOALIAS const                  state,
      const double* NOALIAS const                                   stateGradient,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         productValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        productValues[unknown] += (1.0 + direction) * stateGradient[unknown];
      }
    }

    static void multiplyMaterialParameterMatrix(
      [[maybe_unused]] const double* NOALIAS const                  state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      const int                                                     direction,
      double* NOALIAS const                                         values
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        values[unknown] *= 2.0 + direction;
      }
    }

    static void sourceTerm(
      const double* NOALIAS const                                   state,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      [[maybe_unused]] const double                                 timeStamp,
      [[maybe_unused]] const double                                 timeStepSize,
      double* NOALIAS const                                         sourceValues
    ) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        sourceValues[unknown] += 3.0 * state[unknown];
      }
    }
  };

  double varyingStateEntry(const int cellNode, const int value) { return 1.0 + 0.125 * (cellNode + value); }

  void fillVaryingState(double (&state)[StateSize]) {
    for (int entry = 0; entry < StateSize; entry++) {
      state[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        state[cellNode * CellLayout::PaddedNumberOfData + value] = varyingStateEntry(cellNode, value);
      }
    }
  }

  void fillStateLinearAlongAxis(double (&state)[StateSize], const int markedAxis) {
    for (int entry = 0; entry < StateSize; entry++) {
      state[entry] = 0.0;
    }
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const double reference = QuadratureNodes[nodeIndexOf(cellNode)(markedAxis)];
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        state[cellNode * CellLayout::PaddedNumberOfData + value] = 1.0 + value + Slope * reference;
      }
    }
  }
} // namespace

exahype2::dg::ader::tests::LinearPredictorTermsTest::LinearPredictorTermsTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::LinearPredictorTermsTest") {}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::run() {
  testMethod(testTheFluxFillsEveryNodeAndAxis);
  testMethod(testAConstantStateLeavesTheNonconservativeProductAtZero);
  testMethod(testALinearStateGivesTheExactNonconservativeProduct);
  testMethod(testTheMaterialParameterMatrixActsOnEveryAxis);
  testMethod(testTheSourceFillsEveryNode);
}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::testTheFluxFillsEveryNodeAndAxis() {
  double state[StateSize];
  fillVaryingState(state);

  double flux[DIMENSIONS * AxisStride] = {};

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  exahype2::dg::ader::
    evaluateLinearFlux<MarkerSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      flux,
      state,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize
    );

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int base = axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          (1.0 + axis) * varyingStateEntry(cellNode, unknown),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < CellLayout::PaddedNumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, axis, cellNode, unknown);
      }
    }
  }
}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::testAConstantStateLeavesTheNonconservativeProductAtZero() {
  double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeDerivativeOperator(derivativeOperator);

  double state[StateSize];
  for (int entry = 0; entry < StateSize; entry++) {
    state[entry] = 4.0;
  }

  double stateGradient[GradientSize]   = {};
  double flux[DIMENSIONS * AxisStride] = {};

  const auto                                  inverseCellSize = makeInverseCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  exahype2::dg::ader::
    addNonconservativeProduct<MarkerSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      flux,
      stateGradient,
      state,
      derivativeOperator,
      inverseCellSize,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize
    );

  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    validateNumericalEqualsWithParams1(flux[entry], 0.0, entry);
  }
}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::testALinearStateGivesTheExactNonconservativeProduct() {
  double derivativeOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeDerivativeOperator(derivativeOperator);

  const auto                                  inverseCellSize = makeInverseCellSize();
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  for (int markedAxis = 0; markedAxis < DIMENSIONS; markedAxis++) {
    double state[StateSize];
    fillStateLinearAlongAxis(state, markedAxis);

    double stateGradient[GradientSize]   = {};
    double flux[DIMENSIONS * AxisStride] = {};

    exahype2::dg::ader::addNonconservativeProduct<
      MarkerSolver,
      NumberOfDofsPerAxis,
      NumberOfUnknowns,
      NumberOfAuxiliaryVariables,
      double>(
      flux,
      stateGradient,
      state,
      derivativeOperator,
      inverseCellSize,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize
    );

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const double expected = (axis == markedAxis) ? (1.0 + axis) * Slope * inverseCellSize(axis) : 0.0;
      for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
        const int base = axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns;
        for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams3(flux[base + unknown], expected, markedAxis, axis, cellNode);
        }
        for (int unknown = NumberOfUnknowns; unknown < CellLayout::PaddedNumberOfUnknowns; unknown++) {
          validateNumericalEqualsWithParams3(flux[base + unknown], 0.0, markedAxis, axis, cellNode);
        }
      }
    }
  }
}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::testTheMaterialParameterMatrixActsOnEveryAxis() {
  double state[StateSize];
  fillVaryingState(state);

  double flux[DIMENSIONS * AxisStride];
  for (int entry = 0; entry < DIMENSIONS * AxisStride; entry++) {
    flux[entry] = 1.0 + 0.125 * entry;
  }

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  exahype2::dg::ader::applyMaterialParameterMatrix<
    MarkerSolver,
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    double>(flux, state, cellCentre, cellSize, TimeStamp, TimeStepSize);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      const int base = axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns;
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          (2.0 + axis) * (1.0 + 0.125 * (base + unknown)),
          axis,
          cellNode,
          unknown
        );
      }
      for (int unknown = NumberOfUnknowns; unknown < CellLayout::PaddedNumberOfUnknowns; unknown++) {
        validateNumericalEqualsWithParams3(
          flux[base + unknown],
          1.0 + 0.125 * (base + unknown),
          axis,
          cellNode,
          unknown
        );
      }
    }
  }
}

void exahype2::dg::ader::tests::LinearPredictorTermsTest::testTheSourceFillsEveryNode() {
  double state[StateSize];
  fillVaryingState(state);

  double source[AxisStride] = {};

  const tarch::la::Vector<DIMENSIONS, double> cellCentre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.5);

  exahype2::dg::ader::
    evaluateLinearSource<MarkerSolver, NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double>(
      source,
      state,
      cellCentre,
      cellSize,
      TimeStamp,
      TimeStepSize
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const int base = cellNode * CellLayout::PaddedNumberOfUnknowns;
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(
        source[base + unknown],
        3.0 * varyingStateEntry(cellNode, unknown),
        cellNode,
        unknown
      );
    }
    for (int unknown = NumberOfUnknowns; unknown < CellLayout::PaddedNumberOfUnknowns; unknown++) {
      validateNumericalEqualsWithParams2(source[base + unknown], 0.0, cellNode, unknown);
    }
  }
}
