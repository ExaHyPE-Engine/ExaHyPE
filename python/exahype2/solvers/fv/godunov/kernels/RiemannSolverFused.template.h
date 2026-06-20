// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "exahype2/CellData.h"

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}

double riemannSolverFused(
  {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{STORAGE_PRECISION}}* const QInFlattened,
  {{STORAGE_PRECISION}}* const       QOutFlattened,
  const {{STORAGE_PRECISION}}* const QInFaces,
  {{STORAGE_PRECISION}}* const       QOutFaces,
  const std::uint64_t*               faceIndices,
  const double                       timeStamp,
  const double                       timeStepSize,
  const double* const                patchCentres,
  const double* const                patchSizes,
  const int                          numberOfPatches,
  const int                          targetDevice
);

{% for item in NAMESPACE -%}
}
{%- endfor %}
