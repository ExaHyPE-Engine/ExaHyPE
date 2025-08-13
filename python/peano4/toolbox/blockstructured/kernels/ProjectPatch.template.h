// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"

#include "tarch/CompilerSpecificSettings.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
  {%- endfor %}

void projectPatch(
  const {{PRECISION}}* NOALIAS const QIn,
  {{PRECISION}}* NOALIAS const faces[TWO_TIMES_D]
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
