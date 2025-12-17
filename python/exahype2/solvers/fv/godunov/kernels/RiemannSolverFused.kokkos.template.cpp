// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"

#include <Kokkos_Core.hpp>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&                       solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  [[maybe_unused]] const int                                             targetDevice
) {
  const int numberOfPatches                       = patchData.numberOfCells;
  const {{COMPUTE_PRECISION}}* const QInFlattened = patchData.QInFlattened;
  {{COMPUTE_PRECISION}}* const QOutFlattened      = patchData.QOutFlattened;
  const double* const patchCentres                = patchData.cellCentre[0].data();
  const double* const patchSizes                  = patchData.cellSize[0].data();
  const double timeStamp                          = patchData.t;
  const double timeStepSize                       = patchData.dt;

{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_reduce("RiemannSolver", teamPolicy, KOKKOS_LAMBDA (const Kokkos::TeamPolicy<>::member_type& teamMember, double& maxEigenvalueAfterTimeStep) {
    const int patchIndex                   = teamMember.league_rank();
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
{% if DIMENSIONS == 3 %}
    Kokkos::parallel_for(Kokkos::TeamVectorMDRange(teamMember, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}), [=, &maxEigenvalueAfterTimeStep](const int x, const int y, const int z) {
{% else %}
    Kokkos::parallel_for(Kokkos::TeamVectorMDRange(teamMember, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}), [=, &maxEigenvalueAfterTimeStep](const int x, const int y) {
{% endif %}
      {{ ComputeFluxesAndPerformTimeStep() | indent(4)}}
    });
  }, Kokkos::Max<double>(maxEigenvalueAfterTimeStep));

{% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
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

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_reduce("RiemannSolver", teamPolicy, KOKKOS_LAMBDA (const Kokkos::TeamPolicy<>::member_type& teamMember, double& maxEigenvalueAfterTimeStep) {
    const int patchIndex                   = teamMember.league_rank();
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
{% if DIMENSIONS == 3 %}
    Kokkos::parallel_for(Kokkos::TeamVectorMDRange(teamMember, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}), [=, &maxEigenvalueAfterTimeStep](const int x, const int y, const int z) {
{% else %}
    Kokkos::parallel_for(Kokkos::TeamVectorMDRange(teamMember, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}), [=, &maxEigenvalueAfterTimeStep](const int x, const int y) {
{% endif %}
      {{ ComputeFluxesAndPerformTimeStep() | indent(4)}}
    });
  }, Kokkos::Max<double>(maxEigenvalueAfterTimeStep));

{% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
