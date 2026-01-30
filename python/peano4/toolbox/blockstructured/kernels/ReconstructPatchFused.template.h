// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/fv/VolumeIndex.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
);

void reconstructPatchFused(
  const {{PRECISION}}* NOALIAS const  QInFlattened,
  const {{PRECISION}}* NOALIAS const  facePool,
  const std::uint64_t*                faceIndices,
  {{PRECISION}}* NOALIAS const        QOutFlattened,
  const int                           numberOfPatches,
  const int                           targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
