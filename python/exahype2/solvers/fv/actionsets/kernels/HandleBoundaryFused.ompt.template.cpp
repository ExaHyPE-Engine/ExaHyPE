// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

namespace {
  auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
    int base   = 1;
    int result = 0;
    for (int d = 0; d < DIMENSIONS; d++) {
      result += overlapCell(d) * base;
      if (d==normal) {
        base *= {{HALO_SIZE}} * 2;
      } else {
        base *= {{NUMBER_OF_VOLUMES_PER_AXIS}};
      }
    }
    return result;
  };
}

void {{FULL_QUALIFIED_NAMESPACE}}::handleBoundaryFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                        solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  const int                                                              targetDevice
) {
  handleBoundaryFused(
    solver,
    patchData.facesFlattened,
    patchData.boundaryFaceIndices,
    patchData.boundaryFaceNumbers,
    patchData.patchIndicesOfBoundaryFaces,
    patchData.cellCentre,
    patchData.cellSize,
    patchData.t,
    patchData.dt,
    patchData.numberOfBoundaryFaces,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::handleBoundaryFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&             solver,
  {{STORAGE_PRECISION}}*                      facesFlattened,
  const uint64_t*                             faceIndices,
  const uint8_t*                              faceNumbers,
  const uint64_t*                             cellIndices,
  const tarch::la::Vector<DIMENSIONS,double>* cellCentres,
  const tarch::la::Vector<DIMENSIONS,double>* cellSizes,
  const double                                t,
  const double                                dt,
  const int                                   numberOfFaces,
  const int                                   numberOfCells,
  const int                                   targetDevice
) {
  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

  // #pragma omp target teams distribute parallel for device(targetDevice) \
  //   map(to:   faceIndices[0:numberOfFaces],                             \
  //             faceNumbers[0:numberOfFaces],                             \
  //             cellIndices[0:numberOfFaces],                             \
  //             cellCentres[0:numberOfCells],                             \
  //             cellSizes[0:numberOfCells])                               \
  //   map(tofrom: facesFlattened[0:numberOfFaces*FaceCardinality])
  #pragma omp target teams distribute parallel for device(targetDevice) \
    is_device_ptr(faceIndices, faceNumbers, cellIndices, cellCentres, cellSizes, facesFlattened)
  for (int boundaryFaceIndex = 0; boundaryFaceIndex < numberOfFaces; ++boundaryFaceIndex) {
    const uint64_t faceIndex  = faceIndices[boundaryFaceIndex];
    const int      faceNumber = faceNumbers[boundaryFaceIndex];
    const uint64_t cellIndex  = cellIndices[boundaryFaceIndex];

    tarch::la::Vector<DIMENSIONS, double> patchSize   = cellSizes[cellIndex];
    tarch::la::Vector<DIMENSIONS, double> patchCentre = cellCentres[cellIndex];
    double* Q = &facesFlattened[faceIndex * FaceCardinality];

    {{ ApplyBoundaryCondition() | indent(2) }}
  }
}
