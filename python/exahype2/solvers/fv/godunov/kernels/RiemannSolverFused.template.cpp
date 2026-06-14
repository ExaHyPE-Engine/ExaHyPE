// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "tarch/instrumentation/Trace.h"

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

  constexpr int PatchCardinality = {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %};
  constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
#define OMP_REDUCTION_CLAUSE reduction(max:maxEigenvalueAfterTimeStep)
{% else %}
#define OMP_REDUCTION_CLAUSE
{% endif %}

  traceEnter("Patch updates");
#pragma omp parallel for collapse({{DIMENSIONS + 1}}) schedule(static) \
  OMP_REDUCTION_CLAUSE
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex)
{% if DIMENSIONS==3 %}
  for (int z = 0; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++z)
{% endif %}
  for (int y = 0; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++y)
  for (int x = 0; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++x) {
    const auto p = patchIndex;

    const {{STORAGE_PRECISION}}* const QIn = &QInFlattened[p * PatchCardinality];
    {{STORAGE_PRECISION}}* const QOut      = &QOutFlattened[p * PatchCardinality];

    const auto patchCentre = tarch::la::Vector<DIMENSIONS, double> {
      patchCentres[p * {{DIMENSIONS}}],
      patchCentres[p * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchCentres[p * {{DIMENSIONS}} + 2] {% endif %}
    };
    const auto patchSize = tarch::la::Vector<DIMENSIONS, double> {
      patchSizes[p * {{DIMENSIONS}}],
      patchSizes[p * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchSizes[p * {{DIMENSIONS}} + 2] {% endif %}
    };

    const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
    {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
    for (int f = 0; f < TWO_TIMES_D; f++) {
      auto faceIndex = faceIndices[p * TWO_TIMES_D + f];
      faceQIn[f] = &QInFaces[faceIndex * FaceCardinality];
      faceQOut[f] = &QOutFaces[faceIndex * FaceCardinality];
    }

    {{ ComputeFluxesAndPerformTimeStep() | indent(2)}}
  }
  traceLeave("Patch updates");

{% if COMPUTE_MAX_EIGENVALUE %}
  traceLeave();
  return maxEigenvalueAfterTimeStep;
{% else %}
  traceLeave();
  return 0.0;
{% endif %}
}
