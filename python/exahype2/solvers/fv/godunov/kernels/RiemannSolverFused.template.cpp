// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

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
{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

  const int volumesPerAxis = {{NUMBER_OF_VOLUMES_PER_AXIS}};
  const int volumesPerPatch = volumesPerAxis * volumesPerAxis {% if DIMENSIONS==3 %} * volumesPerAxis {% endif %};
  const int totalCells = numberOfPatches * volumesPerPatch;

{% if MULTITHREADING == "ON" %}
#pragma omp parallel for schedule(static) {% if COMPUTE_MAX_EIGENVALUE==True %} reduction(max : maxEigenvalueAfterTimeStep) {% endif %}
{% endif %}
  for (int globalIdx = 0; globalIdx < totalCells; ++globalIdx) {
    const int patchIndex = globalIdx / volumesPerPatch;
    const int cellIdx    = globalIdx % volumesPerPatch;

{% if DIMENSIONS == 3 %}
    const int z = cellIdx / (volumesPerAxis * volumesPerAxis);
    const int r = cellIdx % (volumesPerAxis * volumesPerAxis);
    const int y = r / volumesPerAxis;
    const int x = r % volumesPerAxis;
{% else %}
    const int y = cellIdx / volumesPerAxis;
    const int x = cellIdx % volumesPerAxis;
{% endif %}

    const {{COMPUTE_PRECISION}}* const QIn = &QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_WITH_HALO_3D}} {% else %} {{PATCH_CARDINALITY_WITH_HALO_2D}} {% endif %}];
    {{COMPUTE_PRECISION}}* const QOut      = &QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

    const tarch::la::Vector<DIMENSIONS, double> patchCentre {
      patchCentres[patchIndex * {{DIMENSIONS}}],
      patchCentres[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchCentres[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
    };
    const tarch::la::Vector<DIMENSIONS, double> patchSize {
      patchSizes[patchIndex * {{DIMENSIONS}}],
      patchSizes[patchIndex * {{DIMENSIONS}} + 1]
      {% if DIMENSIONS==3 %} , patchSizes[patchIndex * {{DIMENSIONS}} + 2] {% endif %}
    };

    {{ ComputeFluxesAndPerformTimeStep() | indent(2)}}
  } // globalIdx

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
