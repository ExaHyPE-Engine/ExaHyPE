// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DataRepository.h"

#include "FVSolver.h"
#include "exahype2/RuntimeParameters.h"
#include "exahype2/fv/BoundaryConditions.h"
#include "peano4/Loop.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace benchmarks::exahype2::kernelbenchmarks {
  namespace {
    int getGridLength() {
      int result = 1;
      for (int level = 0; level < ::exahype2::RuntimeParameters::getInstance().getGridLevels(); ++level) {
        result *= 3;
      }
      return result;
    }
  } // namespace

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
    const int  gridLength = getGridLength();
    const auto cellCount  = std::pow(gridLength, DIMENSIONS);
    const auto faceCount  = std::pow(gridLength, DIMENSIONS - 1) * (gridLength + 1);
    const int  depth      = ::exahype2::RuntimeParameters::getInstance().getGridLevels();
    _depthLevels.resize(depth + 1); // For 1 AMR level
    _depthLevels[depth - 1].QNew.resize(cellCount);
    _depthLevels[depth - 1].QOld.resize(cellCount);
    for (int axis = 0; axis < DIMENSIONS; ++axis) {
      _depthLevels[depth - 1].facesQNew[axis].resize(faceCount);
      _depthLevels[depth - 1].facesQOld[axis].resize(faceCount);
    }

    if (UseAMR) {
      const auto cellCountAMR = std::pow(gridLength * 3, DIMENSIONS);
      const auto faceCountAMR = std::pow(gridLength * 3, DIMENSIONS - 1) * (gridLength * 3 + 1);
      _depthLevels[depth].QNew.resize(cellCountAMR);
      _depthLevels[depth].QOld.resize(cellCountAMR);
      for (int axis = 0; axis < DIMENSIONS; ++axis) {
        _depthLevels[depth].facesQNew[axis].resize(faceCountAMR);
        _depthLevels[depth].facesQOld[axis].resize(faceCountAMR);
      }
    }
  }

  DataRepository& DataRepository::getInstance() {
    static DataRepository instance;
    return instance;
  }

  int DataRepository::getFaceCardinality() const { return facedata::FVSolverQNew::Cardinality; }

  int DataRepository::getCellCardinality() const { return celldata::FVSolverQNew::Cardinality; }


  std::size_t DataRepository::getMemoryUsageInBytes() const {
    std::size_t totalStorageSize = 0;
    for (const auto& level : _depthLevels) {
      totalStorageSize += level.QNew.size() * sizeof(CellQNew);
      totalStorageSize += level.QOld.size() * sizeof(CellQOld);
      for (int axis = 0; axis < DIMENSIONS; ++axis) {
        totalStorageSize += level.facesQNew[axis].size() * sizeof(FaceQNew);
        totalStorageSize += level.facesQOld[axis].size() * sizeof(FaceQOld);
      }
    }
    return totalStorageSize;
  }

  double* DataRepository::getCellQNew(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    const double                          ratio = DomainSize[0] / cellSize[0];
    const int                             depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    int index = peano4::dLinearised(cellIndex, std::pow(3, depth));
    return _depthLevels[depth - 1].QNew[index].data();
  }

  double* DataRepository::getCellQOld(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize
  ) {
    const double                          ratio = DomainSize[0] / cellSize[0];
    const int                             depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    tarch::la::Vector<DIMENSIONS, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<DIMENSIONS, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    int index = peano4::dLinearised(cellIndex, std::pow(3, depth));
    return _depthLevels[depth - 1].QOld[index].data();
  }

  double* DataRepository::getFaceQNew(int index, int depth, int axis) {
    auto& faceData = _depthLevels[depth - 1].facesQNew[axis % DIMENSIONS];
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
    return _depthLevels[depth - 1].facesQNew[axis % DIMENSIONS][index].data();
  }

  double* DataRepository::getFaceQOld(int index, int depth, int axis) {
    auto& faceData = _depthLevels[depth - 1].facesQOld[axis % DIMENSIONS];
    if (axis >= DIMENSIONS) {
      // High face
      return faceData[index + 1].data();
    } else {
      // Low face
      return faceData[index].data();
    }
  }

  double* DataRepository::getFaceQOld(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          axis
  ) {
    const double ratio = DomainSize[0] / cellSize[0];
    const int    depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));
    int          index = Indexing::getFaceIndex(cellCentre, cellSize, axis);
    return _depthLevels[depth - 1].facesQOld[axis % DIMENSIONS][index].data();
  }
} // namespace benchmarks::exahype2::kernelbenchmarks
