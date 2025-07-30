// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

{% import 'LoopOverVolumes.macro.h' as patch with context %}
{% import 'LoopOverDimensions.macro.h' as dimensions with context %}
{% import 'GatherQ.macro.h' as gather with context %}
{% import 'GatherQNeighbors.macro.h' as neighbors with context %}
{% import 'ScatterQ.macro.h' as scatter with context %}
{% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
{% import 'CustomRiemann.template.h' as riemann with context %}{# get template macros #}
{% else %}
{% import 'GenericRusanov.template.h' as riemann with context %}{# get template macros #}
{% endif %}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolutionWithRiemann(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&  solver,
  const {{STORAGE_PRECISION}}* NOALIAS const        QIn,
  {{STORAGE_PRECISION}}* NOALIAS const              QOut,
  const double                                      timeStamp,
  const double                                      timeStepSize,
  const tarch::la::Vector<Dimensions, double>&      patchCentre,
  const tarch::la::Vector<Dimensions, double>&      patchSize
) {
  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  {{ riemann.createMemoryOnStack() }}

  {% call patch.loopOverVolumes() %}
    {{ gather.gatherQ() }}

    {% call dimensions.loopOverDimensions() %}
      {{ neighbors.gatherQNeighbors() }}
      {{ riemann.computeFluxesAndPerformTimeStep() }}
    {% endcall %}

    {{ scatter.scatterQ() }}

    {% if COMPUTE_MAX_EIGENVALUE==True %}
    {{ riemann.computeMaxEigenvalueAfterTimeStep() }}
    {% endif %}
  {% endcall %}

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
}
