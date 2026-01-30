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
#if 1
#define OMP_DATA_CLAUSE \
  is_device_ptr(QInFlattened, facePool, faceIndices)
#else
  std::size_t facePoolLength = 0;
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    for(int f = 0; f < TWO_TIMES_D; ++f) {
      facePoolLength = std::max(facePoolLength, faceIndices[patchIndex * TWO_TIMES_D + f]);
    }
  }
#define OMP_DATA_CLAUSE \
  map(to:   QInFlattened[0:numberOfPatches*{{DOFS*UNKNOWNS}}], \
            faceIndices[0:numberOfPatches*TWO_TIMES_D]) \
  map(from: facePool[0:facePoolLength * {{UNKNOWNS * FACES_PER_DOF}}])
#endif

#pragma omp target teams distribute \
  device(targetDevice) \
  OMP_DATA_CLAUSE
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];

    {% if DIMENSIONS==3 %}
    for (int z = 0; z < {{DOFS_PER_AXIS}}; ++z)
    {% endif %}
    for (int y = 0; y < {{DOFS_PER_AXIS}}; ++y)
    for (int x = 0; x < {{DOFS_PER_AXIS}}; ++x) {
      {{ CopyPatchOntoFaces() | indent(4) }}
    }
  }
}
