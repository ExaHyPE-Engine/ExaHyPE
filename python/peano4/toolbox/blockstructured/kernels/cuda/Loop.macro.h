// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverCells(operation) %}
  for (int z = threadIdx.z; z < {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}; z += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
    for (int y = threadIdx.y; y < {{ DOFS_PER_AXIS }}; y += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
      for (int x = threadIdx.x; x < {{ DOFS_PER_AXIS }}; x += {% if DIMENSIONS==3 %} 8 {% else %} 16 {% endif %}) {
        {{ operation.inner_loop_macro() | indent(6) }}
      } // x
    } // y
  } // z
{% endmacro %}

{% macro defineGlobalKernel(operation) %}
  __global__ static void
  {% if DIMENSIONS==3 %} __launch_bounds__(512) {% else %} __launch_bounds__(256) {% endif %}
  kernel(
    {{ operation.cuda_kernel_parameters() | indent(2) }}
  ) {
    const int patchIndex    = blockIdx.x;
    {{ operation.setup_patch_loop() | indent(2) }}

    {{ loopOverCells(operation) | indent(2) }}
  }
{% endmacro %}

{% macro loopOverPatches(operation) %}
  {{ operation.setup_patchdata_variables() }}

  {% if DIMENSIONS==3 %} const dim3 dimBlock(8, 8, 8); {% else %} const dim3 dimBlock(16, 16, 1); {% endif %}
  const dim3 dimGrid(numberOfPatches, 1, 1);
  CHECK_CUDA_ERROR(cudaSetDevice(targetDevice));

  kernel<<<dimGrid, dimBlock, 0, tarch::accelerator::deviceStreamGet()>>>(
    {{ operation.parameters_passed_to_inner_loop() | indent(2) }}
  );

  CHECK_CUDA_ERROR(cudaStreamSynchronize(tarch::accelerator::deviceStreamGet()));
{% endmacro %}
