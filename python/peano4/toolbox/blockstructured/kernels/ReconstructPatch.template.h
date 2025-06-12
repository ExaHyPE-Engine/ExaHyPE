// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"

#include "tarch/CompilerSpecificSettings.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void reconstructPatch(
  const {{PRECISION}}* NOALIAS const  QIn,
  {{PRECISION}}* NOALIAS const        faces[TwoTimesD],
  {{PRECISION}}* NOALIAS const        QOut
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
