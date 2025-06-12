// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatchFused.h"

#include "peano4/utils/Loop.h"

{% set fused_loop_path = "Loop.template.h" %}

{% import fused_loop_path as fused with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}>& patchData,
  const int                                           targetDevice
) {
  {{ fused.loopOverPatches() }}
}
