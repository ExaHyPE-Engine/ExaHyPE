// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  const {{STORAGE_PRECISION}}* NOALIAS const        QIn,
  {{STORAGE_PRECISION}}* NOALIAS const              QOut,
  const double                                      timeStamp,
  const double                                      timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&      patchCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      patchSize
) {
{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

{% if DIMENSIONS==3 %}
  for (int z = 0; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++z) {
{% endif %}
    for (int y = 0; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++y) {
      for (int x = 0; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++x) {
        {{ ComputeFluxesAndPerformTimeStep() | indent(6)}}
      } // x
    } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
