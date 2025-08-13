// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% macro loopOverCells(operation) %}
  Kokkos::parallel_for(
    Kokkos::TeamThreadMDRange(
      teamMember,
      {{DOFS_PER_AXIS}},
      {{DOFS_PER_AXIS}},
      {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}
    ), KOKKOS_LAMBDA(const int x, const int y, const int z) {
      {{ operation.inner_loop_macro() | indent(4) }}
  });
{% endmacro %}

{% macro loopOverPatches(operation) %}
  {{ operation.setup_patchdata_variables() }}

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_for("patch-loop", teamPolicy, KOKKOS_LAMBDA(const Kokkos::TeamPolicy<>::member_type& teamMember) {
    const int patchIndex = teamMember.league_rank();
    {{ operation.setup_patch_loop() }}
    {{ loopOverCells(operation) | indent(2) }}
  });
  Kokkos::fence();
{% endmacro %}
