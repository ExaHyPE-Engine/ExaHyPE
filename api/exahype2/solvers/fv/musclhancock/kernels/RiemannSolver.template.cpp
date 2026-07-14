// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "tarch/instrumentation/Log.h"

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

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  {{ riemann.createMemoryOnStack() }}

  traceEnter("Patch update");
  {% call patch.loopOverVolumes() %}
    {{ gather.gatherQ() }}

    {% call dimensions.loopOverDimensions() %}
      {{ neighbors.gatherQNeighbors() }}
      {{ riemann.computeFluxesAndPerformTimeStep() }}
    {% endcall %}

    {{ scatter.scatterQ() }}
    {{ scatter.projectOntoFaces() }}

    {% if COMPUTE_MAX_EIGENVALUE==True %}
    {{ riemann.computeMaxEigenvalueAfterTimeStep() }}
    {% endif %}
  {% endcall %}
  traceLeave("Patch update");

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  traceLeave();
  return maxEigenvalueAfterTimeStep;
  {% else %}
  traceLeave();
  return 0.0;
  {% endif %}
}
