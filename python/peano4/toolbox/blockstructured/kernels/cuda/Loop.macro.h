// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'CopyPatchAndFillHalo.macro.h' as cp with context %}{# get template macros #}

{% macro loopOverCells() %}
  for (int z = threadIdx.z; z < {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
    for (int y = threadIdx.y; y < {{ DOFS_PER_AXIS }}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = threadIdx.x; x < {{ DOFS_PER_AXIS }}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ cp.copyPatchAndFillHalo() | indent(6) }}
      } // x
    } // y
  } // z
{% endmacro %}

{% macro defineGlobalKernel() %}
  __global__ static void
  {% if DIMENSIONS==3 %} __launch_bounds__(512) {% else %} __launch_bounds__(256) {% endif %}
  kernel(
    const {{PRECISION}}* const NOALIAS QInFlattened,
    const {{PRECISION}}* const NOALIAS facesFlattened,
    {{PRECISION}}* const NOALIAS       QOutFlattened
  ) {
    using FaceType          = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];
    const int patchIndex    = blockIdx.x;
    const auto* const QIn   = &QInFlattened[patchIndex  * {{DOFS*UNKNOWNS}}];
    const auto* const faces = reinterpret_cast<FaceType>(&facesFlattened[patchIndex * TwoTimesD * {{FACES_PER_DOF*UNKNOWNS}}]);
    auto* const QOut        = &QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

    {{ loopOverCells() | indent(2) }}
  }
{% endmacro %}

{% macro loopOverPatches() %}
  const int numberOfPatches = patchData.numberOfCells;

  {% if DIMENSIONS==3 %} const dim3 dimBlock(8, 8, 8); {% else %} const dim3 dimBlock(16, 16, 1); {% endif %}
  const dim3 dimGrid(numberOfPatches, 1, 1);
  CHECK_CUDA_ERROR(cudaSetDevice(targetDevice));

  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.QOutFlattened
  );

  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));
{% endmacro %}
