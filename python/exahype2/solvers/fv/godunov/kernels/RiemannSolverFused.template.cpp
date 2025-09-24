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
{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

  const int numberOfPatches                       = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QInFlattened = patchData.QInFlattened;
  {{COMPUTE_PRECISION}}* const QOutFlattened      = patchData.QOutFlattened;
  const double* const patchCentres                = patchData.cellCentre[0].data();
  const double* const patchSizes                  = patchData.cellSize[0].data();
  const double timeStamp                          = patchData.t;
  const double timeStepSize                       = patchData.dt;

{% if MULTITHREADING == "ON" %}
#pragma omp parallel for schedule(static) {% if COMPUTE_MAX_EIGENVALUE==True %} reduction(max : maxEigenvalueAfterTimeStep) {% endif %}
{% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
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

{% if DIMENSIONS==3 %}
    for (int z = 0; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++z) {
{% endif %}
      for (int y = 0; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++y) {
        for (int x = 0; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++x) {
          {{ ComputeFluxesAndPerformTimeStep() | indent(8)}}
        } // x
      } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}
  } // patchIndex

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
