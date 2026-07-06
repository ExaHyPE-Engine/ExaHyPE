// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "exahype2/fv/VolumeIndex.h"
#include "tarch/CompilerSpecificSettings.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void reconstructPatch(
  const {{PRECISION}}* NOALIAS const  QIn,
  {{PRECISION}}* NOALIAS const        faces[TWO_TIMES_D],
  {{PRECISION}}* NOALIAS const        QOut
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
