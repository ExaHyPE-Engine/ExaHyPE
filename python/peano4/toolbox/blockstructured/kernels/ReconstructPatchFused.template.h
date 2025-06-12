// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}>& patchData,
  const int                                           targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
