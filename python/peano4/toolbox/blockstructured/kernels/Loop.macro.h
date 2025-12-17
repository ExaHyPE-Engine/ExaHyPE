// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverCells(operation) %}
  for (int z = 0; z < {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}; z++) {
    for (int y = 0; y < {{DOFS_PER_AXIS}}; y++) {
      for (int x = 0; x < {{DOFS_PER_AXIS}}; x++) {
        {{ operation.inner_loop_macro() | indent(6) }}
      } // x
    } // y
  } // z
{% endmacro %}

{% macro loopOverPatches(operation) %}
  {{ operation.setup_patchdata_variables() }}

  {% if MULTITHREADING == "ON" %}
  #pragma omp parallel for schedule(static)
  {% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    {{ operation.setup_patch_loop() }}
    {{ loopOverCells(operation) | indent(2) }}
  }
{% endmacro %}

{% macro loopOverPatchesNoPatchData(operation) %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    {{ operation.setup_patch_loop() }}
    {{ loopOverCells(operation) | indent(2) }}
  }
{% endmacro %}
