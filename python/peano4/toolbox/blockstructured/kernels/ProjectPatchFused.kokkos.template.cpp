// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ProjectPatchFused.h"

#include "peano4/Loop.h"

#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Projection.h"

{% from 'CopyPatchOntoFaces.macro.h' import CopyPatchOntoFaces with context %}

#include "Kokkos_Core.hpp"

struct ProjectPatchFusedFunctor {
  const {{PRECISION}}*  _QInFlattened;
  {{PRECISION}}*        _facePool;
  const std::uint64_t*  _faceIndices;

  ProjectPatchFusedFunctor(
    const {{PRECISION}}* QInFlattened,
    {{PRECISION}}* facePool,
    const std::uint64_t* faceIndices
  ) : _QInFlattened(QInFlattened),
      _facePool(facePool),
      _faceIndices(faceIndices) {}

  using member_type = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;

  KOKKOS_INLINE_FUNCTION
  void operator()(const member_type& teamMember) const {
    const auto patchIndex      = teamMember.league_rank();

    const auto* const QIn      = &_QInFlattened[patchIndex * {{DOFS*UNKNOWNS}}];
    auto* const       facePool = _facePool;
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
      {{ CopyPatchOntoFaces() | indent(8) }}
    });
  }
};

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
  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);

  ProjectPatchFusedFunctor functor(QInFlattened, facePool, faceIndices);
  Kokkos::parallel_for("PatchLoop", teamPolicy, functor);
  Kokkos::fence();
}
