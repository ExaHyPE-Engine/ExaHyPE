// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "FVSolver.h"
#include "exahype2/fv/BoundaryConditions.h"
#include "peano4/utils/Linearised.h"
#include "peano4/utils/Loop.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace benchmarks::exahype2::kernelbenchmarks {
  int DataRepository::Indexing::linearise(
    const tarch::la::Vector<DIMENSIONS, int>&    cellIndex,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    const double ratio = DomainSize[0] / cellSize[0];
    const int    depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));

    int index  = 0;
    int stride = 1;
    for (int i = 0; i < DIMENSIONS; ++i) {
      index += cellIndex[i] * stride;

      int dimSize = (i == 0) ? (std::pow(3, depth) + 1) : std::pow(3, depth);
      stride *= dimSize;
    }
    return index;
  }

  tarch::la::Vector<DIMENSIONS, int> DataRepository::Indexing::getFaceIndices(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    tarch::la::Vector<DIMENSIONS, int>    faceIndices;
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    // vector: x_1, x_2, x_3
    faceIndices[0] = linearise(cellIndex, cellSize);
    // vector: x_2, x_1, x_3
    std::swap(cellIndex[0], cellIndex[1]);
    faceIndices[1] = linearise(cellIndex, cellSize);
#if DIMENSIONS == 3
    // vector: x_3, x_1, x_2
    std::swap(cellIndex[0], cellIndex[2]);
    faceIndices[2] = linearise(cellIndex, cellSize);
#endif
    return faceIndices;
  }

  int DataRepository::Indexing::getFaceIndex(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          axis
  ) {
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    // Permute as described in getFaceIndices()
    for (int i = 1; i <= axis % DIMENSIONS; ++i) {
      std::swap(cellIndex[0], cellIndex[i]);
    }
    int faceIndex = linearise(cellIndex, cellSize);
    return axis >= DIMENSIONS ? faceIndex + 1 : faceIndex;
  }

  DataRepository::DataRepository() {
    const auto cellCount = std::pow(GridLength, DIMENSIONS);
    const auto faceCount = std::pow(GridLength, DIMENSIONS - 1) * (GridLength + 1);
    const int  depth     = static_cast<int>(std::round(std::log(GridLength) / std::log(3.0)));
    _depthLevels.resize(depth + 1); // For 1 AMR level
    _depthLevels[depth - 1].Q.resize(cellCount);
    _depthLevels[depth - 1].QReconstructed.resize(cellCount);
    for (int axis = 0; axis < DIMENSIONS; ++axis) {
      _depthLevels[depth - 1].faces[axis].resize(faceCount);
    }

    if (UseAMR) {
      const auto cellCountAMR = std::pow(GridLength * 3, DIMENSIONS);
      const auto faceCountAMR = std::pow(GridLength * 3, DIMENSIONS - 1) * (GridLength * 3 + 1);
      _depthLevels[depth].Q.resize(cellCountAMR);
      _depthLevels[depth].QReconstructed.resize(cellCountAMR);
      for (int axis = 0; axis < DIMENSIONS; ++axis) {
        _depthLevels[depth].faces[axis].resize(faceCountAMR);
      }
    }
  }

  DataRepository& DataRepository::getInstance() {
    static DataRepository instance;
    return instance;
  }

  int DataRepository::getFaceCardinality() const { return facedata::FVSolverQNew::Cardinality; }

  int DataRepository::getCellQCardinality() const { return celldata::FVSolverQ::Cardinality; }

  int DataRepository::getCellQReconstructedCardinality() const { return celldata::FVSolverQReconstructed::Cardinality; }

  std::size_t DataRepository::getMemoryUsageInBytes() const {
    const auto cellCount   = std::pow(GridLength, DIMENSIONS);
    const auto faceCount   = std::pow(GridLength, DIMENSIONS - 1) * (GridLength + 1);
    const auto volumeCount = cellCount * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS);
    const auto volumeReconstructedCount = cellCount
                                          * std::
                                            pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch + 2, DIMENSIONS);
    const auto facesVolumesCount = faceCount
                                   * std::pow(AbstractFVSolver::NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS - 1) * 2;
    std::size_t totalStorageSize = (volumeCount + volumeReconstructedCount + facesVolumesCount)
                                   * (AbstractFVSolver::NumberOfUnknowns + AbstractFVSolver::NumberOfAuxiliaryVariables)
                                   * sizeof(double);
    return totalStorageSize;
  }

  double* DataRepository::getCellQ(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    const double                          ratio = DomainSize[0] / cellSize[0];
    const int                             depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    int index = peano4::utils::dLinearised(cellIndex, std::pow(3, depth));
    return _depthLevels[depth - 1].Q[index].data();
  }

  double* DataRepository::getCellQReconstructed(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    const double                          ratio = DomainSize[0] / cellSize[0];
    const int                             depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    int index = peano4::utils::dLinearised(cellIndex, std::pow(3, depth));
    return _depthLevels[depth - 1].QReconstructed[index].data();
  }

  double* DataRepository::getFaceQNew(int index, int depth, int axis) {
    auto& faceData = _depthLevels[depth - 1].faces[axis % DIMENSIONS];
    if (axis >= DIMENSIONS) {
      // High face
      return faceData[index + 1].data();
    } else {
      // Low face
      return faceData[index].data();
    }
  }

  double* DataRepository::getFaceQNew(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          axis
  ) {
    const double ratio = DomainSize[0] / cellSize[0];
    const int    depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    int          index = Indexing::getFaceIndex(cellCentre, cellSize, axis);
    return _depthLevels[depth - 1].faces[axis % DIMENSIONS][index].data();
  }
} // namespace benchmarks::exahype2::kernelbenchmarks
