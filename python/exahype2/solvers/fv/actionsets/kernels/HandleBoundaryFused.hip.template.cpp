// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

namespace {
  __device__ inline int serialisePatchIndex(const tarch::la::Vector<DIMENSIONS, int>& overlapCell, int normal) {
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

__global__ static void
{% if DIMENSIONS==3 %} __launch_bounds__(32) {% else %} __launch_bounds__(32) {% endif %}
kernel(
  {{STORAGE_PRECISION}}* const                               facesFlattened,
  const std::uint64_t*              __restrict__             faceIndices,
  const std::uint8_t*               __restrict__             faceNumbers,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellCentres,
  const tarch::la::Vector<DIMENSIONS, double>* __restrict__  cellSizes,
  const std::uint64_t*              __restrict__             cellIndices,
  const double                                               t,
  const int                                                  numberOfFaces
) {
  if (blockIdx.x >= (unsigned)numberOfFaces) return;
  if (threadIdx.x | threadIdx.y | threadIdx.z) return;

  const auto boundaryFaceIndex = blockIdx.x;
  const auto faceIndex  = faceIndices[boundaryFaceIndex];
  const auto faceNumber = static_cast<int>(faceNumbers[boundaryFaceIndex]);
  const auto cellIndex  = cellIndices[boundaryFaceIndex];

  auto patchSize   = cellSizes[cellIndex];
  auto patchCentre = cellCentres[cellIndex];

  auto* Q = &facesFlattened[faceIndex * {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %}];

  {{ ApplyBoundaryCondition() | indent(2) }}
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
  if (numberOfFaces <= 0) {
    return;
  }

  const dim3 dimBlock(1, 1, 1);
  const dim3 dimGrid(numberOfFaces, 1, 1);

  CHECK_HIP_ERROR(hipSetDevice(targetDevice));
  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    facesFlattened, faceIndices, faceNumbers, cellCentres, cellSizes, cellIndices, t, numberOfFaces
  );
  CHECK_HIP_ERROR(hipStreamSynchronize(tarch::accelerator::deviceStreamGet()));
}
