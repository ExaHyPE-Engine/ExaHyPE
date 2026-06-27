// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/ParallelFor.h"

{% from "ApplyBoundaryCondition.macro.h" import ApplyBoundaryCondition with context %}

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
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}& solver,
  {{STORAGE_PRECISION}}*                      facesFlattened,
  const std::uint64_t*                        faceIndices,
  const std::uint8_t*                         faceNumbers,
  const std::uint64_t*                        cellIndices,
  const tarch::la::Vector<DIMENSIONS,double>* cellCentres,
  const tarch::la::Vector<DIMENSIONS,double>* cellSizes,
  const double                                t,
  [[maybe_unused]] const double               dt,
  const int                                   numberOfFaces,
  [[maybe_unused]] const int                  numberOfCells,
  [[maybe_unused]] const int                  targetDevice
) {
  traceEnter();
  traceEnter("Boundary handling");
  ::tarch::accelerator::setCurrentDevice(targetDevice);

  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

  ::tarch::parallelFor(
    ::tarch::Range(numberOfFaces),
    [=] GPU_CALLABLE_LAMBDA (int boundaryFaceIndex) {
      const auto faceIndex  = faceIndices[boundaryFaceIndex];
      const auto faceNumber = static_cast<int>(faceNumbers[boundaryFaceIndex]);
      const auto cellIndex  = cellIndices[boundaryFaceIndex];

      auto patchSize   = cellSizes[cellIndex];
      auto patchCentre = cellCentres[cellIndex];
      auto* Q = &facesFlattened[faceIndex * FaceCardinality];

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

      {{ ApplyBoundaryCondition() | indent(6) }}
    }
  );
  traceLeave("Boundary handling");
  traceLeave();
}
