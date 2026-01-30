// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

#include "tarch/accelerator/sycl/Device.h"
#include <sycl/sycl.hpp>

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
  sycl::queue& queue = *static_cast<sycl::queue*>(tarch::accelerator::getSYCLQueue(targetDevice));

  sycl::event event = queue.submit([&](sycl::handler& h) {
    h.parallel_for(sycl::range<1>(numberOfFaces), [=](sycl::id<1> gid) {
      auto boundaryFaceIndex = gid[0];

      uint64_t faceIndex  = faceIndices[boundaryFaceIndex];
      int      faceNumber = faceNumbers[boundaryFaceIndex];
      uint64_t cellIndex  = cellIndices[boundaryFaceIndex];

      tarch::la::Vector<DIMENSIONS,double> patchSize   = cellSizes[cellIndex];
      tarch::la::Vector<DIMENSIONS,double> patchCentre = cellCentres[cellIndex];

      double* Q = &facesFlattened[
        faceIndex * ({% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %})
      ];

      {{ ApplyBoundaryCondition() | indent(6) }}
    });
  });

  event.wait();
}
