// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include <algorithm>
#include <cstring>

{% if GPU != "no" %}
{% set fused_loop_path = GPU ~ "/Loop.template.h" %}
{% elif MULTITHREADING != "no" %}
{% set fused_loop_path = MULTITHREADING ~ "/Loop.template.h" %}
{% else %}
{% set fused_loop_path = "Loop.template.h" %}
{% endif %}

{% import fused_loop_path as fused with context %}

{% if GPU == "cuda" %}
#include <cub/cub.cuh>
{{ fused.defineGlobalKernel() }}
{% elif GPU == "cpp" %}
#include "tarch/accelerator/cpp/cartesian_product.h"
#include <ranges>
#include <execution>
#include <numeric>
{% elif GPU == "sycl" or MULTITHREADING == "sycl" %}
#include "tarch/accelerator/sycl/Device.h"
#pragma push_macro("Dimensions")
#undef Dimensions
#include <sycl/sycl.hpp>
#pragma pop_macro("Dimensions")
{% endif %}

double {{FULL_QUALIFIED_NAMESPACE}}::updateSolutionWithRiemannFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&                    solver,
  ::exahype2::CellData<{{STORAGE_PRECISION}}, {{STORAGE_PRECISION}}>& patchData,
  const int                                                           targetDevice
) {
  {{ fused.loopOverPatches() }}
}
