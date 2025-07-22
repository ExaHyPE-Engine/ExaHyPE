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
    const tarch::la::Vector<Dimensions, int>&    cellIndex,
    const tarch::la::Vector<Dimensions, double>& cellSize
  ) {
    const double ratio = DomainSize[0] / cellSize[0];
    const int    depth = static_cast<int>(std::round(std::log(ratio) / std::log(3.0)));

    int index  = 0;
    int stride = 1;
    for (int i = 0; i < Dimensions; ++i) {
      index += cellIndex[i] * stride;

      int dimSize = (i == 0) ? (std::pow(3, depth) + 1) : std::pow(3, depth);
      stride *= dimSize;
    }
    return index;
  }

  tarch::la::Vector<Dimensions, int> DataRepository::Indexing::getFaceIndices(
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize
  ) {
    tarch::la::Vector<Dimensions, int>    faceIndices;
    tarch::la::Vector<Dimensions, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<Dimensions, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    // vector: x_1, x_2, x_3
    faceIndices[0] = linearise(cellIndex, cellSize);
    // vector: x_2, x_1, x_3
    std::swap(cellIndex[0], cellIndex[1]);
    faceIndices[1] = linearise(cellIndex, cellSize);
#if Dimensions == 3
    // vector: x_3, x_1, x_2
    std::swap(cellIndex[0], cellIndex[2]);
    faceIndices[2] = linearise(cellIndex, cellSize);
#endif
    return faceIndices;
  }

  int DataRepository::Indexing::getFaceIndex(
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize,
    int                                          axis
  ) {
    tarch::la::Vector<Dimensions, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<Dimensions, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    // permute as described in getFaceIndices()
    for (int i = 1; i <= axis % Dimensions; ++i) {
      std::swap(cellIndex[0], cellIndex[i]);
    }
    int faceIndex = linearise(cellIndex, cellSize);
    return axis >= Dimensions ? faceIndex + 1 : faceIndex;
  }

  DataRepository::DataRepository() {
    int cellCount = std::pow(GridLength, Dimensions);
    int faceCount = std::pow(GridLength, Dimensions - 1) * (GridLength + 1);
    _depthLevel.Q.resize(cellCount);
    for (int axis = 0; axis < Dimensions; ++axis) {
      _depthLevel.faces[axis].resize(faceCount);
    }
  }

  DataRepository& DataRepository::getInstance() {
    static DataRepository instance;
    return instance;
  }

  int DataRepository::getFaceCardinality() { return facedata::FVSolverQNew::Cardinality; }

  int DataRepository::getCellQCardinality() { return celldata::FVSolverQ::Cardinality; }

  int DataRepository::getCellQReconstructedCardinality() { return celldata::FVSolverQReconstructed::Cardinality; }

  double* DataRepository::getCellQ(int index) { return _depthLevel.Q[index].data(); }

  double* DataRepository::getCellQ(
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize
  ) {
    tarch::la::Vector<Dimensions, double> cellCentreOffset = cellCentre - DomainOffset; // remove Offset
    tarch::la::Vector<Dimensions, int>    cellIndex        = tarch::la::convertScalar<int>(
      tarch::la::floor(tarch::la::divideComponents(cellCentreOffset, cellSize))
    );
    int index = peano4::utils::dLinearised(cellIndex, GridLength);
    return getCellQ(index);
  }

  double* DataRepository::getFaceQNew(int index, int axis) {
    auto& faceData = _depthLevel.faces[axis % Dimensions];
    if (axis >= Dimensions) {
      // high face
      return faceData[index + 1].data();
    } else {
      // low face
      return faceData[index].data();
    }
  }

  double* DataRepository::getFaceQNew(
    const tarch::la::Vector<Dimensions, double>& cellCentre,
    const tarch::la::Vector<Dimensions, double>& cellSize,
    int                                          axis
  ) {
    int index = Indexing::getFaceIndex(cellCentre, cellSize, axis);
    return _depthLevel.faces[axis % Dimensions][index].data();
  }

} // namespace benchmarks::exahype2::kernelbenchmarks
