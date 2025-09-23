// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Solver.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

{% import 'Loop.template.h' as patch with context %}{# get template macros #}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolution(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  const {{STORAGE_PRECISION}}* NOALIAS const        QIn,
  {{STORAGE_PRECISION}}* NOALIAS const              QOut,
  const double                                      timeStamp,
  const double                                      timeStepSize,
  const tarch::la::Vector<DIMENSIONS, double>&      patchCentre,
  const tarch::la::Vector<DIMENSIONS, double>&      patchSize
) {
  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  {{ patch.loopOverVolumes() }}

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
}
