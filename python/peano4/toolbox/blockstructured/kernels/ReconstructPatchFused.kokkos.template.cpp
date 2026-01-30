// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReconstructPatchFused.h"

#include "peano4/utils/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"

{% from 'CopyPatchAndFillHalo.macro.h' import CopyPatchAndFillHalo with context %}

#include "Kokkos_Core.hpp"

struct ReconstructPatchFusedFunctor {
  const {{PRECISION}}*  _QInFlattened;
  const {{PRECISION}}*  _facePool;
  const std::uint64_t*  _faceIndices;
  {{PRECISION}}*        _QOutFlattened;

  ReconstructPatchFusedFunctor(
    const {{PRECISION}}* QInFlattened,
    const {{PRECISION}}* facePool,
    const std::uint64_t* faceIndices,
    {{PRECISION}}* QOutFlattened
  ) : _QInFlattened(QInFlattened),
      _facePool(facePool),
      _faceIndices(faceIndices),
      _QOutFlattened(QOutFlattened) {}

  using member_type = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;

  KOKKOS_INLINE_FUNCTION
  void operator()(const member_type& teamMember) const {
    const int patchIndex  = teamMember.league_rank();
    const auto* const QIn = &_QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
    auto* const QOut      = &_QOutFlattened[patchIndex * {{DOFS_WITH_OVERLAP*UNKNOWNS}}];

    const auto* const facePool = _facePool;
    const auto* faceIndices    = _faceIndices;
{% if DIMENSIONS == 3 %}
    Kokkos::parallel_for(
      Kokkos::TeamVectorMDRange(
        teamMember,
        {{DOFS_PER_AXIS}},
        {{DOFS_PER_AXIS}},
        {{DOFS_PER_AXIS}}
      ), KOKKOS_LAMBDA(const int x, const int y, const int z) {
{% else %}
    Kokkos::parallel_for(
      Kokkos::TeamVectorMDRange(
        teamMember,
        {{DOFS_PER_AXIS}},
        {{DOFS_PER_AXIS}}
      ), KOKKOS_LAMBDA(const int x, const int y) {
{% endif %}
      {{ CopyPatchAndFillHalo() | indent(8) }}
    });
  }
};

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  ::exahype2::CellData<{{PRECISION}}, {{PRECISION}}, 3>& patchData,
  const int                                              targetDevice
) {
  reconstructPatchFused(
    patchData.QInFlattened,
    patchData.facesFlattened,
    patchData.faceIndices,
    patchData.QOutFlattened,
    patchData.numberOfCells,
    targetDevice
  );
}

void {{FULL_QUALIFIED_NAMESPACE}}::reconstructPatchFused(
  const {{PRECISION}}* NOALIAS const  QInFlattened,
  const {{PRECISION}}* NOALIAS const  facePool,
  const std::uint64_t*                faceIndices,
  {{PRECISION}}* NOALIAS const        QOutFlattened,
  const int                           numberOfPatches,
  const int                           targetDevice
) {
  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  ReconstructPatchFusedFunctor functor(
    QInFlattened,
    facePool,
    faceIndices,
    QOutFlattened
  );

  Kokkos::parallel_for("patch-loop", teamPolicy, functor);
  Kokkos::fence();
}
