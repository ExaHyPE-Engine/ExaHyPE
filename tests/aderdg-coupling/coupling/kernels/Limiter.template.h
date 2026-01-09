// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "tarch/la/Vector.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

  void regularToLimiterFace(
    const {{REGULAR_SOLVER_CORRECTION_PRECISION}}* Qfrom,
    {{LIMITER_SOLVER_CORRECTION_PRECISION}}* Qto,
    int numVars
  );

  void limiterToRegularFace(
    const {{LIMITER_SOLVER_CORRECTION_PRECISION}}* Qfrom,
    {{REGULAR_SOLVER_CORRECTION_PRECISION}}* Qto,
    int numVars
  );

{% for item in NAMESPACE -%}
}
{%- endfor %}
