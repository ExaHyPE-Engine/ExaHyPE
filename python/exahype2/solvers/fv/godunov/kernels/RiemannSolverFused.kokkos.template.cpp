// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RiemannSolverFused.h"

#include "exahype2/fv/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"

#include <Kokkos_Core.hpp>

{% from 'ComputeFluxesAndPerformTimeStep.macro.h' import ComputeFluxesAndPerformTimeStep with context %}

struct RiemannSolverFusedFunctor {
  const {{STORAGE_PRECISION}}* _QInFlattened;
  {{STORAGE_PRECISION}}*       _QOutFlattened;
  const {{STORAGE_PRECISION}}* _QInFaces;
  {{STORAGE_PRECISION}}*       _QOutFaces;
  const std::uint64_t*         _faceIndices;
  const double*                _patchCentres;
  const double*                _patchSizes;
  double                       _timeStamp;
  double                       _timeStepSize;

  RiemannSolverFusedFunctor(
    const {{STORAGE_PRECISION}}* QInFlattened,
    {{STORAGE_PRECISION}}*       QOutFlattened,
    const {{STORAGE_PRECISION}}* QInFaces,
    {{STORAGE_PRECISION}}*       QOutFaces,
    const std::uint64_t*         faceIndices,
    const double*                patchCentres,
    const double*                patchSizes,
    double                       timeStamp,
    double                       timeStepSize
  ) : _QInFlattened(QInFlattened),
      _QOutFlattened(QOutFlattened),
      _QInFaces(QInFaces),
      _QOutFaces(QOutFaces),
      _faceIndices(faceIndices),
      _patchCentres(patchCentres),
      _patchSizes(patchSizes),
      _timeStamp(timeStamp),
      _timeStepSize(timeStepSize) {}

  using member_type = Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace>::member_type;

  KOKKOS_INLINE_FUNCTION
  void operator()(const member_type& teamMember, double& maxEigenvalue) const {
    constexpr int FaceCardinality = {% if DIMENSIONS==3 %} {{FACE_CARDINALITY_3D}} {% else %} {{FACE_CARDINALITY_2D}} {% endif %};

    const auto patchIndex     = teamMember.league_rank();
    const double timeStamp    = _timeStamp;
    const double timeStepSize = _timeStepSize;

    const {{STORAGE_PRECISION}}* const QIn = &_QInFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];
    {{STORAGE_PRECISION}}* const QOut      = &_QOutFlattened[patchIndex * {% if DIMENSIONS==3 %} {{PATCH_CARDINALITY_3D}} {% else %} {{PATCH_CARDINALITY_2D}} {% endif %}];

    const tarch::la::Vector<DIMENSIONS, double> patchCentre{_patchCentres + patchIndex * {{DIMENSIONS}}};
    const tarch::la::Vector<DIMENSIONS, double> patchSize{_patchSizes + patchIndex * {{DIMENSIONS}}};

    const {{STORAGE_PRECISION}}* faceQIn[TWO_TIMES_D];
    {{STORAGE_PRECISION}}* faceQOut[TWO_TIMES_D];
    for (int f = 0; f < TWO_TIMES_D; f++) {
      auto faceIndex = _faceIndices[patchIndex * TWO_TIMES_D + f];
      faceQIn[f]  = &_QInFaces[faceIndex * FaceCardinality];
      faceQOut[f] = &_QOutFaces[faceIndex * FaceCardinality];
    }
    Kokkos::parallel_reduce(
      Kokkos::TeamVectorRange(
        teamMember,
        {{NUMBER_OF_VOLUMES_PER_AXIS ** DIMENSIONS}}
      ),
      KOKKOS_LAMBDA(const int index, double& maxEigenvalueAfterTimeStep) {
{% if DIMENSIONS == 3 %}
        const int x = index / ({{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}});
        const int y = (index / {{NUMBER_OF_VOLUMES_PER_AXIS}}) % {{NUMBER_OF_VOLUMES_PER_AXIS}};
        const int z = index % {{NUMBER_OF_VOLUMES_PER_AXIS}};
{% else %}
        const int x = index / {{NUMBER_OF_VOLUMES_PER_AXIS}};
        const int y = index % {{NUMBER_OF_VOLUMES_PER_AXIS}};
{% endif %}
        {{ ComputeFluxesAndPerformTimeStep(False) | indent(8) }}
      },
      Kokkos::Max<double>(maxEigenvalue)
    );
  }
};

double {{FULL_QUALIFIED_NAMESPACE}}::riemannSolverFused(
  [[maybe_unused]] {{FULL_QUALIFIED_SOLVER_NAME}}&    solver,
  const {{STORAGE_PRECISION}}* const                  QInFlattened,
  {{STORAGE_PRECISION}}* const                        QOutFlattened,
  const {{STORAGE_PRECISION}}* const                  QInFaces,
  {{STORAGE_PRECISION}}* const                        QOutFaces,
  const std::uint64_t*                                faceIndices,
  const double                                        timeStamp,
  const double                                        timeStepSize,
  const double* const                                 patchCentres,
  const double* const                                 patchSizes,
  const int                                           numberOfPatches,
  [[maybe_unused]] const int                          targetDevice
) {
{% if COMPUTE_MAX_EIGENVALUE %}
  double maxEigenvalueAfterTimeStep = 0.0;
{% endif %}
  RiemannSolverFusedFunctor functor(
    QInFlattened,
    QOutFlattened,
    QInFaces,
    QOutFaces,
    faceIndices,
    patchCentres,
    patchSizes,
    timeStamp,
    timeStepSize
  );

  Kokkos::TeamPolicy<Kokkos::DefaultExecutionSpace> teamPolicy(numberOfPatches, Kokkos::AUTO);
  Kokkos::parallel_reduce(
    "RiemannSolver",
    teamPolicy,
    functor,
    Kokkos::Max<double>(maxEigenvalueAfterTimeStep)
  );

{% if COMPUTE_MAX_EIGENVALUE %}
  return maxEigenvalueAfterTimeStep;
{% else %}
  return 0.0;
{% endif %}
}
