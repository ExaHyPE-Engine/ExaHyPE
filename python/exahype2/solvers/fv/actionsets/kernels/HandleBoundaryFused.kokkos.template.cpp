// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

#include "Kokkos_Core.hpp"

struct HandleBoundaryFusedFunctor {
  {{STORAGE_PRECISION}}*                      _facesFlattened;
  const uint64_t*                             _faceIndices;
  const uint8_t*                              _faceNumbers;
  const uint64_t*                             _cellIndices;
  const tarch::la::Vector<DIMENSIONS,double>* _cellCentres;
  const tarch::la::Vector<DIMENSIONS,double>* _cellSizes;
  double                                      _t;
  double                                      _dt;

  HandleBoundaryFusedFunctor(
    {{STORAGE_PRECISION}}*                      facesFlattened,
    const uint64_t*                             faceIndices,
    const uint8_t*                              faceNumbers,
    const uint64_t*                             cellIndices,
    const tarch::la::Vector<DIMENSIONS,double>* cellCentres,
    const tarch::la::Vector<DIMENSIONS,double>* cellSizes,
    double                                      t,
    double                                      dt
  ) : _facesFlattened(facesFlattened),
      _faceIndices(faceIndices),
      _faceNumbers(faceNumbers),
      _cellIndices(cellIndices),
      _cellCentres(cellCentres),
      _cellSizes(cellSizes),
      _t(t),
      _dt(dt) {}

  KOKKOS_INLINE_FUNCTION
  int serialisePatchIndex(const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) const {
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
  }

  KOKKOS_INLINE_FUNCTION
  void operator()(const int boundaryFaceIndex) const {
    const uint64_t faceIndex = _faceIndices[boundaryFaceIndex];
    const int faceNumber     = _faceNumbers[boundaryFaceIndex];
    const uint64_t cellIndex = _cellIndices[boundaryFaceIndex];
    tarch::la::Vector<DIMENSIONS,double> patchSize   = _cellSizes[cellIndex];
    tarch::la::Vector<DIMENSIONS,double> patchCentre = _cellCentres[cellIndex];
    double* Q = &_facesFlattened[faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %}];
    const double t  = _t;
    const double dt = _dt;
    {{ ApplyBoundaryCondition() | indent(2) }}
  }
};

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
  HandleBoundaryFusedFunctor functor(
    facesFlattened,
    faceIndices,
    faceNumbers,
    cellIndices,
    cellCentres,
    cellSizes,
    t,
    dt
  );
  Kokkos::parallel_for(
    "boundary-face-loop",
    Kokkos::RangePolicy<
      Kokkos::DefaultExecutionSpace,
      Kokkos::LaunchBounds<128, 1>
    >(0, numberOfFaces),
    functor
  );
}
