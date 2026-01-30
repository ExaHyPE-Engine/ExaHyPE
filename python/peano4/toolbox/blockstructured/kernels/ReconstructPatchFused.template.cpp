// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatchFused.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"

{% from 'CopyPatchAndFillHalo.macro.h' import CopyPatchAndFillHalo with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  reconstructPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.QOutFlattened,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  const {{PRECISION}}* NOALIAS const  QInFlattened,
  const {{PRECISION}}* NOALIAS const  facePool,
  const std::uint64_t*                faceIndices,
  {{PRECISION}}* NOALIAS const        QOutFlattened,
  const int                           numberOfPatches,
  const int                           targetDevice
) {
{% if MULTITHREADING == "ON" %}
  #pragma omp parallel for schedule(static)
{% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
    auto* const QOut      = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

{% if DIMENSIONS==3 %}
    for (int z = 0; z < {{DOFS_PER_AXIS}}; ++z)
{% endif %}
    for (int y = 0; y < {{DOFS_PER_AXIS}}; ++y)
    for (int x = 0; x < {{DOFS_PER_AXIS}}; ++x) {
      {{ CopyPatchAndFillHalo() | indent(2) }}
    }
  }
}
