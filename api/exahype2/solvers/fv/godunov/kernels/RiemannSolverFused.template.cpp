// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{STORAGE_PRECISION}}* const                  QInFlattened,
  {{STORAGE_PRECISION}}* const                        QOutFlattened,
  const {{STORAGE_PRECISION}}* const                  QInFaces,
  {{STORAGE_PRECISION}}* const                        QOutFaces,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
  traceEnter();
  ::tarch::accelerator::setCurrentDevice(targetDevice);

  constexpr int PatchCardinality = {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %};
  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

  traceEnter("Patch updates");
{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;

  ::tarch::parallelReduce(
    ::tarch::TeamRange(numberOfPatches),
    [=] GPU_CALLABLE_LAMBDA (int patchIndex, ::tarch::accelerator::TeamHandle team, double& _outerMax) {
      const {{STORAGE_PRECISION}}* const QIn = &QInFlattened[patchIndex * PatchCardinality];
      {{STORAGE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * PatchCardinality];

      const tarch::la::Vector<DIMENSIONS, double> patchCentre(patchCentres + patchIndex * DIMENSIONS);
      const tarch::la::Vector<DIMENSIONS, double> patchSize  (patchSizes   + patchIndex * DIMENSIONS);

      const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
      {{STORAGE_PRECISION}}*       faceQOut[TWO_TIMES_D];
      for (int f = 0; f < TWO_TIMES_D; f++) {
        auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + f];
        faceQIn[f]  = &QInFaces[faceIndex * FaceCardinality];
        faceQOut[f] = &QOutFaces[faceIndex * FaceCardinality];
      }

      ::tarch::parallelReduce(
        team,
        ::tarch::RangeMD<{{DIMENSIONS}}>(
          {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}{% if DIMENSIONS==3 %}, {{NUMBER_OF_VOLUMES_PER_AXIS}}{% endif %}
        ),
        [=](int x, int y, {% if DIMENSIONS==3 %}int z, {% endif %}double& maxEigenvalueAfterTimeStep) {
          {{ ComputeFluxesAndPerformTimeStep() | indent(10) }}
        },
        ::tarch::Max<double>{_outerMax}
      );
    },
    ::tarch::Max<double>{maxEigenvalueAfterTimeStep}
  );
  traceLeave("Patch updates");
  traceLeave();
  return maxEigenvalueAfterTimeStep;
{% else %}
  ::tarch::parallelFor(
    ::tarch::TeamRange(numberOfPatches),
    [=] GPU_CALLABLE_LAMBDA (int patchIndex, ::tarch::accelerator::TeamHandle team) {
      const {{STORAGE_PRECISION}}* const QIn = &QInFlattened[patchIndex * PatchCardinality];
      {{STORAGE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * PatchCardinality];

      const tarch::la::Vector<DIMENSIONS, double> patchCentre(patchCentres + patchIndex * DIMENSIONS);
      const tarch::la::Vector<DIMENSIONS, double> patchSize  (patchSizes   + patchIndex * DIMENSIONS);

      const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
      {{STORAGE_PRECISION}}*       faceQOut[TWO_TIMES_D];
      for (int f = 0; f < TWO_TIMES_D; f++) {
        auto faceIndex = faceIndices[patchIndex * TWO_TIMES_D + f];
        faceQIn[f]  = &QInFaces[faceIndex * FaceCardinality];
        faceQOut[f] = &QOutFaces[faceIndex * FaceCardinality];
      }

      ::tarch::parallelFor(
        team,
        ::tarch::RangeMD<{{DIMENSIONS}}>(
          {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}{% if DIMENSIONS==3 %}, {{NUMBER_OF_VOLUMES_PER_AXIS}}{% endif %}
        ),
        [=](int x, int y{% if DIMENSIONS==3 %}, int z{% endif %}) {
          {{ ComputeFluxesAndPerformTimeStep() | indent(10) }}
        }
      );
    }
  );
  traceLeave("Patch updates");
  traceLeave();
  return 0.0;
{% endif %}
}
