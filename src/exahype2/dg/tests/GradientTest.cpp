// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GradientTest.h"

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/Gradient.h"

namespace {
  constexpr int NumberOfDofsPerAxis        = 3;
  constexpr int NumberOfUnknowns           = 2;
  constexpr int NumberOfAuxiliaryVariables = 1;
  constexpr int NumberOfData               = NumberOfUnknowns + NumberOfAuxiliaryVariables;
  constexpr int CellNodes                  = exahype2::dg::NodesPerCell<NumberOfDofsPerAxis>;
  constexpr int EntriesPerAxis             = CellNodes * NumberOfData;

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

  struct DerivativeOperator final {
    double entries[NumberOfDofsPerAxis * NumberOfDofsPerAxis]{};
  };

  // The entry (i, j) is the derivative of basis function j at node i.
  DerivativeOperator makeDerivativeOperator() {
    DerivativeOperator result;
    for (int row = 0; row < NumberOfDofsPerAxis; row++) {
      for (int column = 0; column < NumberOfDofsPerAxis; column++) {
        result.entries[row * NumberOfDofsPerAxis + column] = basisDerivative(QuadratureNodes[row], column);
      }
    }
    return result;
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
} // namespace

exahype2::dg::tests::GradientTest::GradientTest() :
  tarch::tests::TestCase("exahype2::dg::tests::GradientTest") {}

void exahype2::dg::tests::GradientTest::run() {
  testMethod(testGradientOfALinearPolynomialIsItsSlope);
  testMethod(testEachBlockHoldsTheDerivativeAlongItsOwnAxis);
  testMethod(testNarrowStorageIsDifferentiatedInTheComputePrecision);
}

void exahype2::dg::tests::GradientTest::testGradientOfALinearPolynomialIsItsSlope() {
  const DerivativeOperator derivativeOperator = makeDerivativeOperator();
  const auto               cellSize           = makeCellSize();

  // Every value grows with its own slope along every axis.
  double cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const auto nodeIndex = nodeIndexOf(cellNode);
    for (int value = 0; value < NumberOfData; value++) {
      double entry = 1.0 + value;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        entry += (2.0 + value + axis) * QuadratureNodes[nodeIndex(axis)];
      }
      cellState[cellNode * NumberOfData + value] = entry;
    }
  }

  double gradients[DIMENSIONS * EntriesPerAxis];
  exahype2::dg::
    computeNodalGradients<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
      gradients,
      cellState,
      derivativeOperator.entries,
      cellSize
    );

  // The reference slope over the cell extent is the physical slope.
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        const double expected = (2.0 + value + axis) / cellSize(axis);
        validateNumericalEqualsWithParams3(
          gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value],
          expected,
          axis,
          cellNode,
          value
        );
      }
    }
  }
}

void exahype2::dg::tests::GradientTest::testEachBlockHoldsTheDerivativeAlongItsOwnAxis() {
  const DerivativeOperator derivativeOperator = makeDerivativeOperator();
  const auto               cellSize           = makeCellSize();

  for (int varyingAxis = 0; varyingAxis < DIMENSIONS; varyingAxis++) {
    double cellState[CellNodes * NumberOfData];
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      const double reference = QuadratureNodes[nodeIndexOf(cellNode)(varyingAxis)];
      for (int value = 0; value < NumberOfData; value++) {
        cellState[cellNode * NumberOfData + value] = 3.0 * reference * reference;
      }
    }

    double gradients[DIMENSIONS * EntriesPerAxis];
    exahype2::dg::
      computeNodalGradients<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, double, double>(
        gradients,
        cellState,
        derivativeOperator.entries,
        cellSize
      );

    for (int axis = 0; axis < DIMENSIONS; axis++) {
      for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
        const double reference = QuadratureNodes[nodeIndexOf(cellNode)(varyingAxis)];
        const double expected  = axis == varyingAxis ? 6.0 * reference / cellSize(axis) : 0.0;
        for (int value = 0; value < NumberOfData; value++) {
          validateNumericalEqualsWithParams3(
            gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value],
            expected,
            varyingAxis,
            axis,
            cellNode
          );
        }
      }
    }
  }
}

void exahype2::dg::tests::GradientTest::testNarrowStorageIsDifferentiatedInTheComputePrecision() {
  const DerivativeOperator derivativeOperator = makeDerivativeOperator();
  const auto               cellSize           = makeCellSize();

  float cellState[CellNodes * NumberOfData];
  for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
    const double reference = QuadratureNodes[nodeIndexOf(cellNode)(0)];
    for (int value = 0; value < NumberOfData; value++) {
      cellState[cellNode * NumberOfData + value] = static_cast<float>(0.5 + 2.0 * reference);
    }
  }

  double gradients[DIMENSIONS * EntriesPerAxis];
  exahype2::dg::computeNodalGradients<NumberOfDofsPerAxis, NumberOfUnknowns, NumberOfAuxiliaryVariables, float, double>(
    gradients,
    cellState,
    derivativeOperator.entries,
    cellSize
  );

  // The stored values carry single-precision rounding, so the slope is met to
  // that accuracy only. The state varies along the first axis alone, so the
  // blocks of the other axes hold zero.
  constexpr double Tolerance = 1.0e-5;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double expected = (axis == 0) ? 2.0 / cellSize(0) : 0.0;
    for (int cellNode = 0; cellNode < CellNodes; cellNode++) {
      for (int value = 0; value < NumberOfData; value++) {
        const double slope = gradients[axis * EntriesPerAxis + cellNode * NumberOfData + value];
        validateWithParams3(std::abs(slope - expected) < Tolerance, slope, axis, cellNode);
      }
    }
  }
}
