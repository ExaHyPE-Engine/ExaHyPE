// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

double maxScaledEigenvalue(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  const {{SOLUTION_STORAGE_PRECISION}}* const   luh,
  const tarch::la::Vector<Dimensions, double>&  cellCentre,
  const tarch::la::Vector<Dimensions, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
