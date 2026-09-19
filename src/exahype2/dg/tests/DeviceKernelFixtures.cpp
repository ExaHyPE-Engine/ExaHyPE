// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceKernelFixtures.h"

void exahype2::dg::tests::DeviceDeleter::operator()(void* const data) const {
  tarch::freeMemory(data, tarch::MemoryLocation::Device);
}

/**
 * Returns count values that differ per entry, so an offset that is off by any
 * amount changes the result.
 */
std::vector<double> exahype2::dg::tests::makeRamp(
  const std::size_t count,
  const double      offset,
  const double      increment
) {
  std::vector<double> result(count);
  for (std::size_t entry = 0; entry < count; entry++) {
    result[entry] = offset + increment * static_cast<double>(entry);
  }
  return result;
}

exahype2::dg::tests::KernelOperators::KernelOperators() {
  for (int node = 0; node < DeviceKernelSetup::DofsPerAxis; node++) {
    quadratureNodes[node]       = (node + 0.5) / DeviceKernelSetup::DofsPerAxis;
    massMatrixDiagonal[node]    = 0.5 + 0.25 * node;
    leftFaceCoefficients[node]  = 0.7 - 0.2 * node;
    rightFaceCoefficients[node] = 0.3 + 0.2 * node;
  }
  for (int row = 0; row < DeviceKernelSetup::DofsPerAxis; row++) {
    for (int column = 0; column < DeviceKernelSetup::DofsPerAxis; column++) {
      const int entry           = row * DeviceKernelSetup::DofsPerAxis + column;
      stiffnessMatrix[entry]    = 0.1 * (row + 1) - 0.05 * (column + 1);
      derivativeOperator[entry] = 0.4 * (column - row);
    }
  }
  double weightSum = 0.0;
  for (int faceNode = 0; faceNode < DeviceKernelSetup::NodesPerFace; faceNode++) {
    faceQuadratureWeights[faceNode] = 1.0 + faceNode;
    weightSum += faceQuadratureWeights[faceNode];
  }
  for (double& weight : faceQuadratureWeights) {
    weight /= weightSum;
  }
  for (int cellNode = 0; cellNode < DeviceKernelSetup::NodesPerCell; cellNode++) {
    inverseCellQuadratureWeights[cellNode] = 1.0 / (0.5 + 0.05 * cellNode);
  }
  for (int entry = 0; entry < DeviceKernelSetup::RungeKuttaSteps * DeviceKernelSetup::RungeKuttaSteps; entry++) {
    rungeKuttaCoefficients[entry] = 0.25 - 0.1 * entry;
  }
}

exahype2::dg::tests::CellChain::CellChain() {
  for (int cell = 0; cell < DeviceKernelSetup::NumberOfCells; cell++) {
    const int firstCoordinate = cell * DIMENSIONS;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellSizes[firstCoordinate + axis] = (0.3 + 0.1 * axis) * (1.0 + 0.05 * cell);
    }
    cellCentres[firstCoordinate] = (cell + 0.5) * cellSizes[firstCoordinate];
    for (int axis = 1; axis < DIMENSIONS; axis++) {
      cellCentres[firstCoordinate + axis] = 0.5 * cellSizes[firstCoordinate + axis];
    }
  }

  int nextFaceIndex = DeviceKernelSetup::NumberOfCells + 1;
  for (int cell = 0; cell < DeviceKernelSetup::NumberOfCells; cell++) {
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
  for (int cell = 0; cell < DeviceKernelSetup::NumberOfCells; cell++) {
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

int exahype2::dg::tests::CellChain::numberOfWorkFaces() const { return static_cast<int>(workFaceIndices.size()); }

int exahype2::dg::tests::CellChain::faceStateEntries() const {
  return numberOfUniqueFaces * DeviceKernelSetup::FaceStateCardinality;
}

int exahype2::dg::tests::CellChain::faceFluxEntries() const {
  return numberOfUniqueFaces * DeviceKernelSetup::FaceFluxCardinality;
}
