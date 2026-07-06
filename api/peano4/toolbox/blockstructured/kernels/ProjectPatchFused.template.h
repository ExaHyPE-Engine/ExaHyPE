// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/fv/VolumeIndex.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

void projectPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
);

void projectPatchFused(
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices,
  const int                          numberOfPatches,
  const int                          targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
