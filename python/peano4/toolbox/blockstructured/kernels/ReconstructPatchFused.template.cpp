// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatchFused.h"

#include "peano4/utils/Loop.h"

{% if GPU != "no" %}
{% set fused_loop_path = GPU ~ "/Loop.macro.h" %}
{% elif MULTITHREADING != "no" %}
{% set fused_loop_path = MULTITHREADING ~ "/Loop.macro.h" %}
{% else %}
{% set fused_loop_path = "Loop.macro.h" %}
{% endif %}

{% import fused_loop_path as fused with context %}
{% import 'CopyPatchAndFillHalo.macro.h' as operation with context %}{# get template macros #}

{% if GPU == "cuda" %}
{{ fused.defineGlobalKernel(operation) }}
{% elif GPU == "cpp" %}
#include "tarch/accelerator/stdpar/cartesian_product.h"
#include <ranges>
#include <execution>
#include <numeric>
{% elif GPU == "sycl" %}
#include "tarch/accelerator/sycl/Device.h"
#include <sycl/sycl.hpp>
{% elif GPU == "kokkos" %}
#include "Kokkos_Core.hpp"
{% endif %}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  {{ fused.loopOverPatches(operation) }}
}
