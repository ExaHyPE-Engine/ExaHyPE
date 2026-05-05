// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"

#include <omp.h>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&                       solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  [[maybe_unused]] const int                                             targetDevice
) {
  return riemannSolverFused(
    solver,
    patchData.QInFlattened,
    patchData.QOutFlattened,
    patchData.t,
    patchData.dt,
    patchData.cellCentre[0].data(),
    patchData.cellSize[0].data(),
    patchData.numberOfCells,
    targetDevice
  );
}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{COMPUTE_PRECISION}}* const                  QInFlattened,
  {{COMPUTE_PRECISION}}* const                        QOutFlattened,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
  constexpr int PatchCardinality = {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %};
  constexpr int PatchCardinalityWithHalo = {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %};

{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
#define OMPT_REDUCTION_CLAUSE reduction(max:maxEigenvalueAfterTimeStep)
{% else %}
#define OMPT_REDUCTION_CLAUSE
{% endif %}

#if 1
#define OMPT_DATA_CLAUSE \
  is_device_ptr(QInFlattened, patchCentres, patchSizes, QOutFlattened)
#else
#define OMPT_DATA_CLAUSE \
  map(to:   QInFlattened[0:numberOfPatches*PatchCardinalityWithHalo], \
            patchCentres[0:numberOfPatches*DIMENSIONS], \
            patchSizes[0:numberOfPatches*DIMENSIONS]) \
  map(from: QOutFlattened[0:numberOfPatches*PatchCardinality])
#endif

#pragma omp target teams distribute parallel for \
  collapse(DIMENSIONS + 1) \
  device(targetDevice) \
  OMPT_REDUCTION_CLAUSE \
  OMPT_DATA_CLAUSE
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex)
{% if DIMENSIONS==3 %}
  for (int z = 0; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++z)
{% endif %}
  for (int y = 0; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++y)
  for (int x = 0; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++x) {
    const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * PatchCardinalityWithHalo];
    {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * PatchCardinality];

    const auto patchCentre = tarch::la::Vector<DIMENSIONS, double> {
      patchCentres[patchIndex * {{DIMENSIONS}}],
      patchCentres[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchCentres[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
    };
    const auto patchSize = tarch::la::Vector<DIMENSIONS, double> {
      patchSizes[patchIndex * {{DIMENSIONS}}],
      patchSizes[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchSizes[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
    };

    {{ ComputeFluxesAndPerformTimeStep(False) | indent(2)}}
  }

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
