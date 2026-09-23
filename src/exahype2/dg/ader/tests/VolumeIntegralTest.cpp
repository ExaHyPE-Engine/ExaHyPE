// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VolumeIntegralTest.h"

#include "exahype2/dg/ader/VolumeIntegral.h"

namespace {
  constexpr int    NumberOfDofsPerAxis        = 3;
  constexpr int    NumberOfUnknowns           = 2;
  constexpr int    NumberOfAuxiliaryVariables = 1;
  constexpr double TimeStepSize               = 0.25;

  using CellLayout = exahype2::dg::ader::Layout<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables>;

  constexpr int AxisStride = CellLayout::NodesPerCell * CellLayout::PaddedNumberOfUnknowns;

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

  void makeStiffnessOperator(double (&stiffnessOperator)[NumberOfDofsPerAxis * NumberOfDofsPerAxis]) {
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        stiffnessOperator[row * NumberOfDofsPerAxis + column] =
          QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row);
      }
    }
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

  double initialCellStateEntry(const int cellNode, const int value) { return 2.0 + 0.125 * (cellNode + value); }

  void fillCellState(double (&cellState)[CellLayout::NodesPerCell * CellLayout::NumberOfData]) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int value = 0; value < CellLayout::NumberOfData; value++) {
        cellState[cellNode * CellLayout::NumberOfData + value] = initialCellStateEntry(cellNode, value);
      }
    }
  }
} // namespace

exahype2::dg::ader::tests::VolumeIntegralTest::VolumeIntegralTest() :
  tarch::tests::TestCase("exahype2::dg::ader::tests::VolumeIntegralTest") {}

void exahype2::dg::ader::tests::VolumeIntegralTest::run() {
  testMethod(testTheCoefficientsCarryTheTimeStepAndTheRowWeight);
  testMethod(testAConstantFluxLiftsTheBasisEndpoints);
  testMethod(testTheSourceScalesWithTheTimeStep);
  testMethod(testTheLinearIntegralSubtractsTheSumOverTheAxes);
}

void exahype2::dg::ader::tests::VolumeIntegralTest::testTheCoefficientsCarryTheTimeStepAndTheRowWeight() {
  double stiffnessOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeStiffnessOperator(stiffnessOperator);

  double inverseQuadratureWeights[NumberOfDofsPerAxis];
  for (int row = 0; row < NumberOfDofsPerAxis; row++) {
    inverseQuadratureWeights[row] = 1.0 / QuadratureWeights[row];
  }

  double volumeCoefficients[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  for (int entry = 0; entry < NumberOfDofsPerAxis * NumberOfDofsPerAxis; entry++) {
    volumeCoefficients[entry] = 9.0;
  }

  exahype2::dg::ader::buildVolumeCoefficients<NumberOfDofsPerAxis, double>(
    volumeCoefficients,
    stiffnessOperator,
    inverseQuadratureWeights,
    TimeStepSize
  );

  for (int row = 0; row < NumberOfDofsPerAxis; row++) {
    double rowSum = 0.0;
    for (int column = 0; column < NumberOfDofsPerAxis; column++) {
      const double expected = TimeStepSize * QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row)
        / QuadratureWeights[row];
      validateNumericalEqualsWithParams2(volumeCoefficients[row * NumberOfDofsPerAxis + column], expected, row, column);
      rowSum += volumeCoefficients[row * NumberOfDofsPerAxis + column];
    }
    // The row applied to a constant flux leaves the difference of the basis function at the two
    // ends of the axis.
    const double expectedRowSum = TimeStepSize * (basisValue(1.0, row) - basisValue(0.0, row)) / QuadratureWeights[row];
    validateNumericalEqualsWithParams1(rowSum, expectedRowSum, row);
  }
}

void exahype2::dg::ader::tests::VolumeIntegralTest::testAConstantFluxLiftsTheBasisEndpoints() {
  double stiffnessOperator[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  makeStiffnessOperator(stiffnessOperator);

  double inverseQuadratureWeights[NumberOfDofsPerAxis];
  for (int row = 0; row < NumberOfDofsPerAxis; row++) {
    inverseQuadratureWeights[row] = 1.0 / QuadratureWeights[row];
  }

  double volumeCoefficients[NumberOfDofsPerAxis * NumberOfDofsPerAxis];
  exahype2::dg::ader::buildVolumeCoefficients<NumberOfDofsPerAxis, double>(
    volumeCoefficients,
    stiffnessOperator,
    inverseQuadratureWeights,
    TimeStepSize
  );

  double timeAveragedFlux[DIMENSIONS * AxisStride] = {};
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
      for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
        timeAveragedFlux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] =
          1.0 + axis + 0.5 * unknown;
      }
    }
  }

  const auto                            cellSize = makeCellSize();
  tarch::la::Vector<DIMENSIONS, double> inverseCellSize;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    inverseCellSize(axis) = 1.0 / cellSize(axis);
  }

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillCellState(cellState);

  exahype2::dg::ader::
    addFluxVolumeIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      cellState,
      timeAveragedFlux,
      volumeCoefficients,
      inverseCellSize
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      double expected = initialCellStateEntry(cellNode, unknown);
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        const int    node = nodeIndex(axis);
        const double flux = 1.0 + axis + 0.5 * unknown;
        expected += TimeStepSize * flux * (basisValue(1.0, node) - basisValue(0.0, node))
          / (QuadratureWeights[node] * cellSize(axis));
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
        initialCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::VolumeIntegralTest::testTheSourceScalesWithTheTimeStep() {
  double timeAveragedSource[AxisStride] = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      timeAveragedSource[cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 0.5 + 0.25 * (cellNode + unknown);
    }
  }

  double cellState[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillCellState(cellState);

  exahype2::dg::ader::
    addSourceVolumeIntegral<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      cellState,
      timeAveragedSource,
      TimeStepSize
    );

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const double expected =
        initialCellStateEntry(cellNode, unknown) + TimeStepSize * (0.5 + 0.25 * (cellNode + unknown));
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
        initialCellStateEntry(cellNode, value),
        cellNode,
        value
      );
    }
  }
}

void exahype2::dg::ader::tests::VolumeIntegralTest::testTheLinearIntegralSubtractsTheSumOverTheAxes() {
  double timeAveragedFlux[DIMENSIONS * AxisStride] = {};
  double timeAveragedSource[AxisStride]            = {};
  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        timeAveragedFlux[axis * AxisStride + cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 1.0 + axis;
      }
      timeAveragedSource[cellNode * CellLayout::PaddedNumberOfUnknowns + unknown] = 4.0;
    }
  }

  double withBothTerms[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  double withoutFlux[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  double withoutSource[CellLayout::NodesPerCell * CellLayout::NumberOfData];
  fillCellState(withBothTerms);
  fillCellState(withoutFlux);
  fillCellState(withoutSource);

  exahype2::dg::ader::subtractTimeAveragedVolumeIntegral<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    true,
    double,
    double>(withBothTerms, timeAveragedFlux, timeAveragedSource, TimeStepSize);
  exahype2::dg::ader::subtractTimeAveragedVolumeIntegral<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    false,
    true,
    double,
    double>(withoutFlux, timeAveragedFlux, timeAveragedSource, TimeStepSize);
  exahype2::dg::ader::subtractTimeAveragedVolumeIntegral<
    NumberOfDofsPerAxis,
    NumberOfUnknowns,
    NumberOfAuxiliaryVariables,
    true,
    false,
    double,
    double>(withoutSource, timeAveragedFlux, timeAveragedSource, TimeStepSize);

  double sumOverAxes = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    sumOverAxes += 1.0 + axis;
  }

  for (int cellNode = 0; cellNode < CellLayout::NodesPerCell; cellNode++) {
    for (int unknown = 0; unknown < NumberOfUnknowns; unknown++) {
      const int    entry   = cellNode * CellLayout::NumberOfData + unknown;
      const double initial = initialCellStateEntry(cellNode, unknown);
      validateNumericalEqualsWithParams2(
        withBothTerms[entry],
        initial + TimeStepSize * (4.0 - sumOverAxes),
        cellNode,
        unknown
      );
      validateNumericalEqualsWithParams2(withoutFlux[entry], initial + TimeStepSize * 4.0, cellNode, unknown);
      validateNumericalEqualsWithParams2(withoutSource[entry], initial - TimeStepSize * sumOverAxes, cellNode, unknown);
    }
    for (int value = NumberOfUnknowns; value < CellLayout::NumberOfData; value++) {
      const int entry = cellNode * CellLayout::NumberOfData + value;
      validateNumericalEqualsWithParams2(withBothTerms[entry], initialCellStateEntry(cellNode, value), cellNode, value);
    }
  }
}
