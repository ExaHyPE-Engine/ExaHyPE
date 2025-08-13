// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverCells(operation) %}
  {% if GPU == "omp" %}
  #pragma omp parallel for collapse({{DIMENSIONS}})
  {% endif %}
  {% if DIMENSIONS==3 %}
  for (int z = 0; z < {{DOFS_PER_AXIS}}; z++) {
  {% endif %}
    for (int y = 0; y < {{DOFS_PER_AXIS}}; y++) {
      for (int x = 0; x < {{DOFS_PER_AXIS}}; x++) {
        {{ operation.inner_loop_macro() | indent(6) }}
      } // x
    } // y
  {% if DIMENSIONS==3 %} } // z {% endif %}
{% endmacro %}

{% macro loopOverPatches(operation) %}
  {{ operation.setup_patchdata_variables() }}

  {% if GPU == "omp" %}
  #pragma omp target teams distribute device(targetDevice)
  {% else %}
  #pragma omp parallel for
  {% endif %}
  for (int patchIndex = 0; patchIndex < numberOfPatches; ++patchIndex) {
    {{ operation.setup_patch_loop() }}
    {{ loopOverCells(operation) | indent(2) }}
  }
{% endmacro %}
