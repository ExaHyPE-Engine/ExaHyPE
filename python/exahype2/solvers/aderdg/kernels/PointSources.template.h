// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

std::vector<int> pointSources(
  {{FULL_QUALIFIED_SOLVER_NAME}}&              solver,
  const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
