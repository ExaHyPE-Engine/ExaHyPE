// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

{% if STATELESS_PDE_TERMS==False -%}

double updateSolutionWithRiemann(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                 solver,
  const {{STORAGE_PRECISION}}* __restrict__ const QIn,
  {{STORAGE_PRECISION}}* __restrict__ const       QOut,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  const tarch::la::Vector<Dimensions, double>&    patchCentre,
  const tarch::la::Vector<Dimensions, double>&    patchSize
);

{% else %}

double updateSolutionWithRiemann(
  const {{STORAGE_PRECISION}}* __restrict__ const QIn,
  {{STORAGE_PRECISION}}* __restrict__ const       QOut,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  const tarch::la::Vector<Dimensions, double>&    patchCentre,
  const tarch::la::Vector<Dimensions, double>&    patchSize
);

{%- endif %}

{% for item in NAMESPACE -%}
}
{%- endfor %}
