// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "tarch/instrumentation/Log.h"

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolver(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&      solver,
  const {{STORAGE_PRECISION}}* NOALIAS const            QIn,
  {{STORAGE_PRECISION}}* NOALIAS const                  QOut,
  const {{STORAGE_PRECISION}}* NOALIAS const            faceQIn[TWO_TIMES_D],
  [[maybe_unused]] {{STORAGE_PRECISION}}* NOALIAS const faceQOut[TWO_TIMES_D],
  const double                                          timeStamp,
  const double                                          timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&          patchCentre,
  const tarch::la::Vector<DIMENSIONS, double>&          patchSize
) {
  traceEnter();

{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}

  traceEnter("Patch update");
{% if DIMENSIONS==3 %}
  for (int z = 0; z < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++z) {
{% endif %}
    for (int y = 0; y < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++y) {
      for (int x = 0; x < {{NUMBER_OF_VOLUMES_PER_AXIS}}; ++x) {
        {{ ComputeFluxesAndPerformTimeStep(True) | indent(6)}}
      } // x
    } // y
{% if DIMENSIONS==3 %}  } // z {% endif %}
  traceLeave("Patch update");

{% if COMPUTE_MAX_EIGENVALUE %}
  traceLeave();
  return maxEigenvalueAfterTimeStep;
{% else %}
  traceLeave();
  return 0.0;
{% endif %}
}
