// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceKernelFixtures.h"

namespace {
  constexpr int DofsPerAxis = exahype2::dg::ader::tests::DeviceKernelSetup::DofsPerAxis;

  // Three-point Gauss-Legendre rule on the unit interval.
  const double QuadratureNodes[DofsPerAxis]   = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
  const double QuadratureWeights[DofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  double basisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < DofsPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  double basisDerivative(const double x, const int node) {
    double result = 0.0;
    for (int omitted = 0; omitted < DofsPerAxis; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (QuadratureNodes[node] - QuadratureNodes[omitted]);
        for (int other = 0; other < DofsPerAxis; other++) {
          if (other != node and other != omitted) {
            term *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  void invertMatrix(const std::vector<double>& matrix, std::vector<double>& inverse) {
    constexpr int WorkColumns = 2 * DofsPerAxis;

    double work[DofsPerAxis][WorkColumns];
    for (int row = 0; row < DofsPerAxis; row++) {
      for (int column = 0; column < DofsPerAxis; column++) {
        work[row][column]               = matrix[row * DofsPerAxis + column];
        work[row][DofsPerAxis + column] = (row == column) ? 1.0 : 0.0;
      }
    }

    for (int pivot = 0; pivot < DofsPerAxis; pivot++) {
      int largestRow = pivot;
      for (int row = pivot + 1; row < DofsPerAxis; row++) {
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
      for (int row = 0; row < DofsPerAxis; row++) {
        if (row != pivot) {
          const double factor = work[row][pivot];
          for (int column = 0; column < WorkColumns; column++) {
            work[row][column] -= factor * work[pivot][column];
          }
        }
      }
    }

    for (int row = 0; row < DofsPerAxis; row++) {
      for (int column = 0; column < DofsPerAxis; column++) {
        inverse[row * DofsPerAxis + column] = work[row][DofsPerAxis + column];
      }
    }
  }
} // namespace

exahype2::dg::ader::tests::BasisTableSet::BasisTableSet() {
  for (int node = 0; node < DofsPerAxis; node++) {
    quadraturePoints[node]         = QuadratureNodes[node];
    quadratureWeights[node]        = QuadratureWeights[node];
    inverseQuadratureWeights[node] = 1.0 / QuadratureWeights[node];
    leftFaceCoefficients[node]     = basisValue(0.0, node);
    rightFaceCoefficients[node]    = basisValue(1.0, node);
  }
  for (int row = 0; row < DofsPerAxis; row++) {
    for (int column = 0; column < DofsPerAxis; column++) {
      const double stiffness = QuadratureWeights[column] * basisDerivative(QuadratureNodes[column], row);
      stiffnessOperator[row * DofsPerAxis + column]           = stiffness;
      stiffnessOperatorTransposed[column * DofsPerAxis + row] = stiffness;
      derivativeOperator[row * DofsPerAxis + column]          = basisDerivative(QuadratureNodes[row], column);
    }
  }
  for (int cellNode = 0; cellNode < DeviceKernelSetup::CellLayout::NodesPerCell; cellNode++) {
    double weight    = 1.0;
    int    remainder = cellNode;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      weight *= QuadratureWeights[remainder % DofsPerAxis];
      remainder /= DofsPerAxis;
    }
    cellQuadratureWeights[cellNode]        = weight;
    inverseCellQuadratureWeights[cellNode] = 1.0 / weight;
  }

  // Picard operator, the end-of-step value minus the temporal stiffness.
  std::vector<double> picardOperator(DeviceKernelSetup::OperatorCardinality);
  for (int row = 0; row < DofsPerAxis; row++) {
    for (int column = 0; column < DofsPerAxis; column++) {
      const int entry       = row * DofsPerAxis + column;
      picardOperator[entry] = basisValue(1.0, row) * basisValue(1.0, column) - stiffnessOperator[entry];
    }
  }
  invertMatrix(picardOperator, inversePicardOperator);
}

exahype2::dg::ader::BasisTables<double> exahype2::dg::ader::tests::BasisTableSet::view() const {
  return exahype2::dg::ader::BasisTables<double>{
    .quadraturePoints             = quadraturePoints.data(),
    .quadratureWeights            = quadratureWeights.data(),
    .inverseQuadratureWeights     = inverseQuadratureWeights.data(),
    .cellQuadratureWeights        = cellQuadratureWeights.data(),
    .inverseCellQuadratureWeights = inverseCellQuadratureWeights.data(),
    .stiffnessOperator            = stiffnessOperator.data(),
    .stiffnessOperatorTransposed  = stiffnessOperatorTransposed.data(),
    .derivativeOperator           = derivativeOperator.data(),
    .inversePicardOperator        = inversePicardOperator.data(),
    .leftFaceCoefficients         = leftFaceCoefficients.data(),
    .rightFaceCoefficients        = rightFaceCoefficients.data()
  };
}

exahype2::dg::ader::tests::DeviceBasisTables::DeviceBasisTables(const BasisTableSet& tables) :
  _quadraturePoints(tables.quadraturePoints),
  _quadratureWeights(tables.quadratureWeights),
  _inverseQuadratureWeights(tables.inverseQuadratureWeights),
  _cellQuadratureWeights(tables.cellQuadratureWeights),
  _inverseCellQuadratureWeights(tables.inverseCellQuadratureWeights),
  _stiffnessOperator(tables.stiffnessOperator),
  _stiffnessOperatorTransposed(tables.stiffnessOperatorTransposed),
  _derivativeOperator(tables.derivativeOperator),
  _inversePicardOperator(tables.inversePicardOperator),
  _leftFaceCoefficients(tables.leftFaceCoefficients),
  _rightFaceCoefficients(tables.rightFaceCoefficients) {}

exahype2::dg::ader::BasisTables<double> exahype2::dg::ader::tests::DeviceBasisTables::view() const {
  return exahype2::dg::ader::BasisTables<double>{
    .quadraturePoints             = _quadraturePoints.data(),
    .quadratureWeights            = _quadratureWeights.data(),
    .inverseQuadratureWeights     = _inverseQuadratureWeights.data(),
    .cellQuadratureWeights        = _cellQuadratureWeights.data(),
    .inverseCellQuadratureWeights = _inverseCellQuadratureWeights.data(),
    .stiffnessOperator            = _stiffnessOperator.data(),
    .stiffnessOperatorTransposed  = _stiffnessOperatorTransposed.data(),
    .derivativeOperator           = _derivativeOperator.data(),
    .inversePicardOperator        = _inversePicardOperator.data(),
    .leftFaceCoefficients         = _leftFaceCoefficients.data(),
    .rightFaceCoefficients        = _rightFaceCoefficients.data()
  };
}
