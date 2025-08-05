// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

{% import 'CopyPatchAndFillHalo.macro.h' as cp with context %}{# get template macros #}

{% macro loopOverCells() %}
  Kokkos::parallel_for(
    Kokkos::TeamThreadMDRange(
      teamMember,
      {{DOFS_PER_AXIS}},
      {{DOFS_PER_AXIS}},
      {% if DIMENSIONS==3 %} {{DOFS_PER_AXIS}} {% else %} 1 {% endif %}
    ), KOKKOS_LAMBDA(const int x, const int y, const int z) {
      {{ cp.copyPatchAndFillHalo() | indent(4) }}
  });
{% endmacro %}

{% macro loopOverPatches() %}
  using FaceType = const {{PRECISION}}(*)[{{FACES_PER_DOF*UNKNOWNS}}];
  using ViewType = Kokkos::View<double**, Kokkos::LayoutRight, Kokkos::SharedHostPinnedSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;
  using ViewFaceType = Kokkos::View<const double***, Kokkos::LayoutRight, Kokkos::SharedHostPinnedSpace, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;

  const int numberOfPatches = patchData.numberOfCells;
  ViewType     QInFlattened(patchData.QInFlattened, numberOfPatches, {{DOFS*UNKNOWNS}});
  ViewFaceType facesFlattened(patchData.facesFlattened, numberOfPatches, TWO_TIMES_D, {{FACES_PER_DOF*UNKNOWNS}});
  ViewType     QOutFlattened(patchData.QOutFlattened, numberOfPatches, {{DOFS_WITH_OVERLAP*UNKNOWNS}});

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_for("reconstruct-patch-loop", teamPolicy, KOKKOS_LAMBDA(const Kokkos::TeamPolicy<>::member_type& teamMember) {
    const int patchIndex   = teamMember.league_rank();
    const auto const QIn   = Kokkos::subview(QInFlattened, patchIndex, Kokkos::ALL).data();
    const auto const faces = reinterpret_cast<FaceType>(Kokkos::subview(facesFlattened, patchIndex, Kokkos::ALL, Kokkos::ALL).data());
    auto const QOut        = Kokkos::subview(QOutFlattened, patchIndex, Kokkos::ALL).data();

    {{ loopOverCells() | indent(2) }}
  });
{% endmacro %}
