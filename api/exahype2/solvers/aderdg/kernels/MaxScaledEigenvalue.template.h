// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

struct MaxEigenvalueStatistics {
  double maxScaledEigenvalue;
  double maxPhysicalEigenvalue;
};

MaxEigenvalueStatistics reduceMaxEigenvalues(
  {{FULL_QUALIFIED_SOLVER_NAME}}&               solver,
  const {{SOLUTION_STORAGE_PRECISION}}* const   luh,
  const tarch::la::Vector<DIMENSIONS, double>&  cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>&  cellSize,
  const double                                  timeStamp,
  const double                                  timeStepSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
