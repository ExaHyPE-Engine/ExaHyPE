// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

#include "tarch/accelerator/sycl/Device.h"
#include <sycl/sycl.hpp>

namespace {
  const auto serialisePatchIndex = [](const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
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
  const std::uint64_t*                        faceIndices,
  const std::uint8_t*                         faceNumbers,
  const std::uint64_t*                        cellIndices,
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
      const auto boundaryFaceIndex = gid[0];

      const auto faceIndex  = faceIndices[boundaryFaceIndex];
      const auto faceNumber = static_cast<int>(faceNumbers[boundaryFaceIndex]);
      const auto cellIndex  = cellIndices[boundaryFaceIndex];

      auto patchSize   = cellSizes[cellIndex];
      auto patchCentre = cellCentres[cellIndex];

      auto* Q = &facesFlattened[
        faceIndex * ({% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %})
      ];

      {{ ApplyBoundaryCondition() | indent(6) }}
    });
  });

  event.wait();
}
