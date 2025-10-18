// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "HandleBoundaryFused.h"

{% if GPU != "no" %}
{% set fused_loop_path = GPU ~ "/Loop.macro.h" %}
{% else %}
{% set fused_loop_path = "Loop.macro.h" %}
{% endif %}

{% import fused_loop_path as fused with context %}

{% if GPU == "cuda" %}
{{ fused.defineGlobalKernel() }}
{% elif GPU == "sycl" or MULTITHREADING == "sycl" %}
#include "tarch/accelerator/sycl/Device.h"
#include <sycl/sycl.hpp>
{% elif GPU == "kokkos" %}
#include "Kokkos_Core.hpp"
{% endif %}

void {{FULL_QUALIFIED_NAMESPACE}}::handleBoundaryFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&                                        solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}, 3>& patchData,
  const int                                                              targetDevice
) {
  {{ fused.loopOverPatches() }}
}
