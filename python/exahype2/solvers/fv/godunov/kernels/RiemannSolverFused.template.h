// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "exahype2/CellData.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

double updateSolutionWithRiemannFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                        solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  const int                                                              targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
