// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchFused.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"

{% from 'CopyPatchOntoFaces.macro.h' import CopyPatchOntoFaces with context %}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  projectPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::projectPatchFused(
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices,
  const int                          numberOfPatches,
  const int                          targetDevice
) {
{% if MULTITHREADING == "ON" %}
  #pragma omp parallel for schedule(static)
{% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];

{% if DIMENSIONS==3 %}
    for (int z = 0; z < {{DOFS_PER_AXIS}}; ++z)
{% endif %}
    for (int y = 0; y < {{DOFS_PER_AXIS}}; ++y)
    for (int x = 0; x < {{DOFS_PER_AXIS}}; ++x) {
      {{ CopyPatchOntoFaces() | indent(2) }}
    }
  }
}
