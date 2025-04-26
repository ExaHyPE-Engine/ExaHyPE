// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

double updateSolutionWithRiemann(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                 solver,
  const {{STORAGE_PRECISION}}* noalias const QIn,
  {{STORAGE_PRECISION}}* noalias const       QOut,
  const double                                    timeStamp,
  const double                                    timeStepSize,
  const tarch::la::Vector<Dimensions, double>&    patchCentre,
  const tarch::la::Vector<Dimensions, double>&    patchSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
