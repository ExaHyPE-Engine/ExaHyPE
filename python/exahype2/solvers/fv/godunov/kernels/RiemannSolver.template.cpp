// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolver.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include <algorithm>
#include <cstring>

{% import 'Loop.template.h' as patch with context %}{# get template macros #}

{% if GPU != "no" %}
{% set fused_loop_path = GPU ~ "/Loop.template.h" %}
{% elif MULTITHREADING != "no" %}
{% set fused_loop_path = MULTITHREADING ~ "/Loop.template.h" %}
{% else %}
{% set fused_loop_path = "Loop.template.h" %}
{% endif %}

{% import fused_loop_path as fused with context %}

{% if STATELESS_PDE_TERMS==False -%}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolutionWithRiemann(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                 solver,
  const {{STORAGE_PRECISION}}* __restrict__ const QIn,
  {{STORAGE_PRECISION}}* __restrict__ const       QOut,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  const tarch::la::Vector<Dimensions, double>&    patchCentre,
  const tarch::la::Vector<Dimensions, double>&    patchSize
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

{% else -%}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolutionWithRiemann(
  const {{STORAGE_PRECISION}}* __restrict__ const QIn,
  {{STORAGE_PRECISION}}* __restrict__ const       QOut,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  const tarch::la::Vector<Dimensions, double>&    patchCentre,
  const tarch::la::Vector<Dimensions, double>&    patchSize
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

{% if GPU == "cuda" %}
#include <cub/cub.cuh>
{{ fused.defineGlobalKernel() }}
{% elif GPU == "cpp" %}
#include "tarch/accelerator/cpp/cartesian_product.h"
#include <ranges>
#include <execution>
#include <numeric>
{% endif %}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolutionWithRiemann(
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}>& patchData,
  const int                                                           targetDevice
) {
  {% if COMPUTE_MAX_EIGENVALUE==True %}
  double maxEigenvalueAfterTimeStep = 0.0;
  {% endif %}

  {{ fused.loopOverPatches() }}

  {% if COMPUTE_MAX_EIGENVALUE==True %}
  return maxEigenvalueAfterTimeStep;
  {% else %}
  return 0.0;
  {% endif %}
}

{%- endif %}
