// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchFused.h"

#include "peano4/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"

{% from 'CopyPatchOntoFaces.macro.h' import CopyPatchOntoFaces with context %}

__global__ static void
{% if DIMENSIONS==3 %} __launch_bounds__(512) {% else %} __launch_bounds__(256) {% endif %}
kernel(
  const {{PRECISION}}* const NOALIAS QInFlattened,
  {{PRECISION}}*       const NOALIAS facePool,
  const std::uint64_t*               faceIndices
) {
  const auto patchIndex = blockIdx.x;
  const auto* const QIn = &QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];

  {% if DIMENSIONS==3 %}
  for (int z = threadIdx.z; z < {{DOFS_PER_AXIS}}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
    {% endif %}
    for (int y = threadIdx.y; y < {{DOFS_PER_AXIS}}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = threadIdx.x; x < {{DOFS_PER_AXIS}}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ CopyPatchOntoFaces() | indent(8)}}
      } // x
    } // y
  {% if DIMENSIONS==3 %}  } // z {% endif %}
}

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
  {% if DIMENSIONS==3 %} const dim3 dimBlock(8, 8, 8); {% else %} const dim3 dimBlock(16, 16, 1); {% endif %}
  const dim3 dimGrid(numberOfPatches, 1, 1);
  CHECK_HIP_ERROR(hipSetDevice(targetDevice));

  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    QInFlattened,
    facePool,
    faceIndices
  );
  CHECK_HIP_ERROR(hipStreamSynchronize(tarch::accelerator::deviceStreamGet()));
}
