// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverCells(operation) %}
  auto ids = std::views::cartesian_product(
    std::views::iota(0, numberOfPatches),
    std::views::iota(0, {{DOFS_PER_AXIS}}),
    std::views::iota(0, {{DOFS_PER_AXIS}}),
    std::views::iota(0, {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %})
  );

  std::for_each_n(std::execution::par_unseq, ids.begin(), std::ranges::size(ids),
    [{{ operation.parameters_passed_to_inner_loop() }}](auto idx) {
      auto [patchIndex, x, y, z] = idx;
      {{ operation.setup_patch_loop() | indent(4) }}
      {{ operation.inner_loop_macro() | indent(4) }}
    }
  );
{% endmacro %}

{% macro loopOverPatches(operation) %}
  {{ operation.setup_patchdata_variables() }}
  {{ loopOverCells(operation) }}
{% endmacro %}
