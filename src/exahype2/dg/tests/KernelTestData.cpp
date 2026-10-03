// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "KernelTestData.h"

#include "peano4/Loop.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"

namespace {
  constexpr int DofsPerAxis = exahype2::dg::tests::KernelTestSetup::DofsPerAxis;

  // The three-point Gauss-Legendre rule on the unit interval.
  const double QuadratureNodes[DofsPerAxis]   = {0.5 * (1.0 - std::sqrt(0.6)), 0.5, 0.5 * (1.0 + std::sqrt(0.6))};
  const double QuadratureWeights[DofsPerAxis] = {5.0 / 18.0, 8.0 / 18.0, 5.0 / 18.0};

  double getBasisValue(const double x, const int node) {
    double result = 1.0;
    for (int other = 0; other < DofsPerAxis; other++) {
      if (other != node) {
        result *= (x - QuadratureNodes[other]) / (QuadratureNodes[node] - QuadratureNodes[other]);
      }
    }
    return result;
  }

  double getBasisDerivative(const double x, const int node) {
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

void
exahype2::dg::tests::DeviceDeleter::operator()(void* const data) const {
  tarch::freeMemory(data, tarch::MemoryLocation::Device);
}

/**
 * Returns count values that differ per entry, so an offset that is off by any
 * amount changes the result.
 */
std::vector<double>
exahype2::dg::tests::getRamp(const std::size_t count, const double offset, const double increment) {
  std::vector<double> result(count);
  for (std::size_t entry = 0; entry < count; entry++) {
    result[entry] = offset + increment * static_cast<double>(entry);
  }
  return result;
}

exahype2::dg::tests::KernelOperators::KernelOperators() {
  for (int node = 0; node < KernelTestSetup::DofsPerAxis; node++) {
    quadratureNodes[node]       = (node + 0.5) / KernelTestSetup::DofsPerAxis;
    leftFaceCoefficients[node]  = 0.7 - 0.2 * node;
    rightFaceCoefficients[node] = 0.3 + 0.2 * node;
  }
  for (int row = 0; row < KernelTestSetup::DofsPerAxis; row++) {
    for (int column = 0; column < KernelTestSetup::DofsPerAxis; column++) {
      derivativeOperator[row * KernelTestSetup::DofsPerAxis + column] = 0.4 * (column - row);
    }
  }
  double weightSum = 0.0;
  for (int faceNode = 0; faceNode < KernelTestSetup::NodesPerFace; faceNode++) {
    faceQuadratureWeights[faceNode] = 1.0 + faceNode;
    weightSum += faceQuadratureWeights[faceNode];
  }
  for (double& weight : faceQuadratureWeights) {
    weight /= weightSum;
  }
  for (int cellNode = 0; cellNode < KernelTestSetup::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0 / (0.5 + 0.05 * cellNode);
  }
}

exahype2::dg::tests::BasisTableSet::BasisTableSet() {
  for (int node = 0; node < DofsPerAxis; node++) {
    quadraturePoints[node]         = QuadratureNodes[node];
    quadratureWeights[node]        = QuadratureWeights[node];
    inverseQuadratureWeights[node] = 1.0 / QuadratureWeights[node];
    leftFaceCoefficients[node]     = getBasisValue(0.0, node);
    rightFaceCoefficients[node]    = getBasisValue(1.0, node);
  }
  for (int row = 0; row < DofsPerAxis; row++) {
    for (int column = 0; column < DofsPerAxis; column++) {
      const double stiffness = QuadratureWeights[column] * getBasisDerivative(QuadratureNodes[column], row);
      stiffnessOperator[row * DofsPerAxis + column]           = stiffness;
      stiffnessOperatorTransposed[column * DofsPerAxis + row] = stiffness;
      derivativeOperator[row * DofsPerAxis + column]          = getBasisDerivative(QuadratureNodes[row], column);
    }
  }
  peano4::dForEach(DofsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& nodeIndex) {
    const int cellNode = peano4::dLinearised(nodeIndex, DofsPerAxis);
    double    weight   = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      weight *= QuadratureWeights[nodeIndex(axis)];
    }
    cellQuadratureWeights[cellNode]        = weight;
    inverseCellQuadratureWeights[cellNode] = 1.0 / weight;
  });

  // The Picard operator is the end-of-step value minus the temporal stiffness.
  std::vector<double> picardOperator(KernelTestSetup::OperatorCardinality);
  for (int row = 0; row < DofsPerAxis; row++) {
    for (int column = 0; column < DofsPerAxis; column++) {
      const int entry       = row * DofsPerAxis + column;
      picardOperator[entry] = getBasisValue(1.0, row) * getBasisValue(1.0, column) - stiffnessOperator[entry];
    }
  }
  invertMatrix(picardOperator, inversePicardOperator);
}

exahype2::dg::BasisTables<double>
exahype2::dg::tests::BasisTableSet::getView() const {
  return exahype2::dg::BasisTables<double>{
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

exahype2::dg::tests::DeviceBasisTables::DeviceBasisTables(const BasisTableSet& tables) :
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

exahype2::dg::BasisTables<double>
exahype2::dg::tests::DeviceBasisTables::getView() const {
  return exahype2::dg::BasisTables<double>{
    .quadraturePoints             = _quadraturePoints.getData(),
    .quadratureWeights            = _quadratureWeights.getData(),
    .inverseQuadratureWeights     = _inverseQuadratureWeights.getData(),
    .cellQuadratureWeights        = _cellQuadratureWeights.getData(),
    .inverseCellQuadratureWeights = _inverseCellQuadratureWeights.getData(),
    .stiffnessOperator            = _stiffnessOperator.getData(),
    .stiffnessOperatorTransposed  = _stiffnessOperatorTransposed.getData(),
    .derivativeOperator           = _derivativeOperator.getData(),
    .inversePicardOperator        = _inversePicardOperator.getData(),
    .leftFaceCoefficients         = _leftFaceCoefficients.getData(),
    .rightFaceCoefficients        = _rightFaceCoefficients.getData()
  };
}

exahype2::dg::tests::CellChain::CellChain() {
  for (int cell = 0; cell < KernelTestSetup::NumberOfCells; cell++) {
    const int firstCoordinate = cell * DIMENSIONS;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSizes[firstCoordinate + axis] = (0.3 + 0.1 * axis) * (1.0 + 0.05 * cell);
    }
    cellCentres[firstCoordinate] = (cell + 0.5) * cellSizes[firstCoordinate];
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      cellCentres[firstCoordinate + axis] = 0.5 * cellSizes[firstCoordinate + axis];
    }
  }

  int nextFaceIndex = KernelTestSetup::NumberOfCells + 1;
  for (int cell = 0; cell < KernelTestSetup::NumberOfCells; cell++) {
    const int firstFace                 = cell * peano4::FacesPerCell;
    faceIndices[firstFace]              = cell;
    faceIndices[firstFace + DIMENSIONS] = cell + 1;
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      faceIndices[firstFace + axis]              = nextFaceIndex++;
      faceIndices[firstFace + axis + DIMENSIONS] = nextFaceIndex++;
    }
  }
  numberOfUniqueFaces = nextFaceIndex;

  std::vector<bool> alreadyListed(numberOfUniqueFaces, false);
  for (int cell = 0; cell < KernelTestSetup::NumberOfCells; cell++) {
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const std::uint64_t faceIndex = faceIndices[cell * peano4::FacesPerCell + faceNumber];
      if (not alreadyListed[faceIndex]) {
        alreadyListed[faceIndex] = true;
        workFaceIndices.push_back(faceIndex);
        workFaceNumbers.push_back(static_cast<std::uint8_t>(faceNumber));
        workCellIndices.push_back(cell);
      }
    }
  }
}

int
exahype2::dg::tests::CellChain::getNumberOfWorkFaces() const {
  return static_cast<int>(workFaceIndices.size());
}

int
exahype2::dg::tests::CellChain::getNumberOfFaceStateEntries() const {
  return numberOfUniqueFaces * KernelTestSetup::FaceStateCardinality;
}

int
exahype2::dg::tests::CellChain::getNumberOfFaceFluxEntries() const {
  return numberOfUniqueFaces * KernelTestSetup::FaceFluxCardinality;
}
