// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <type_traits>

#include "exahype2/dg/Basis.h"
#include "exahype2/dg/ader/Predictor.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/ader/Fallback.h"
#include "exahype2/fv/ader/Scheme.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"

/**
 * @file
 * The loop functors of the ADER-WENO solver family over the cells, the faces and the level jumps of
 * the cell connectivity. Every loop body calls functions of exahype2::fv::ader::Scheme.
 */
TARGET_DECLARE
namespace exahype2::fv::ader {
  struct PredictionStatistics;

  struct CorrectionStatistics;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct PredictPatches;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct SolveRiemannProblems;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename FallbackDerived = void>
  struct CorrectPatches;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename FallbackDerived>
  struct FixUpPatches;

  template <typename Derived, typename ComputePrecision>
  struct InterpolateFaceHalves;

  template <typename Derived, typename ComputePrecision>
  struct RestrictFluxHalves;
} // namespace exahype2::fv::ader

/**
 * The largest wave speed of the faces inside the patches of a prediction and the numbers of its
 * volumes whose Picard iteration diverged or stopped at its sweep limit.
 */
struct exahype2::fv::ader::PredictionStatistics final {
  using ValueType = exahype2::fv::ader::PredictionStatistics;

  double                               maxEigenvalue{0.0};
  exahype2::dg::ader::PredictionCounts counts{};

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType identity();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType combine(ValueType left, ValueType right);
};

/**
 * The largest wave speed of the corrected volumes and the number of patches, or of volumes with a
 * fallback per volume, that repeat their step with the fallback scheme.
 */
struct exahype2::fv::ader::CorrectionStatistics final {
  using ValueType = exahype2::fv::ader::CorrectionStatistics;

  double maxEigenvalue{0.0};
  int    fallbackCells{0};

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType identity();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD ValueType combine(ValueType left, ValueType right);
};

/**
 * Predicts every volume of every listed cell, adds the updates inside the patch to the patch and
 * writes the predicted states and fluxes of the faces of the patch into their own halves. Reduces
 * the largest wave speed of the faces inside the patches and counts the volumes whose Picard
 * iteration diverged or stopped at its sweep limit. Team entry advances the cells entry,
 * entry + numberOfTeams and so on. All its threads reconstruct the patch together and then predict
 * groups of volumes at once, threadsPerVolume threads per volume. With statuses, a cell with such a
 * volume writes FallbackStatus::FailedPrediction into its status and the volume writes NaN, as a
 * diverged one does, so a neighbour that reads its face halves falls back as well. The team keeps the
 * reconstruction and the predictions inside the patch and the
 * work of its volumes in its chunk of scratchSizePerTeam values in scratch. With interiorUpdates,
 * every cell keeps the updates of its volumes through the faces inside the patch there.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::PredictPatches final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::CellConnectivity                           cellConnectivity{};
  tarch::Span<const int>                      cells{};
  int                                         numberOfTeams{0};
  int                                         threadsPerVolume{1};
  typename CellPool::View                     solution{};
  typename CellPool::View                     previousSolution{};
  typename FacePool::ConstView                halo{};
  typename StatePool::View                    faceStates{};
  typename StatePool::View                    faceFluxes{};
  exahype2::dg::BasisTables<ComputePrecision> tables{};
  ComputePrecision*                           scratch{nullptr};
  int                                         scratchSizePerTeam{0};
  int                                         scratchChunks{0};
  double                                      timeStamp{0.0};
  double                                      timeStepSize{0.0};
  typename StatusPool::View                   statuses{};
  typename StatePool::View                    interiorUpdates{};
  typename StatusPool::ConstView              limiterFaceStatuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    PredictionStatistics&          accumulator
  ) const;
};

/**
 * Solves the Riemann problems on every listed face of a patch and reduces the largest wave speed.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::SolveRiemannProblems final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::CellConnectivity        cellConnectivity{};
  tarch::Span<const int>   faces{};
  typename StatePool::View faceStates{};
  typename StatePool::View faceFluxes{};
  double                   timeStamp{0.0};
  double                   timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, double& accumulator) const;
};

/**
 * Adds the numerical fluxes on the faces of every listed patch to its volumes and reduces the
 * largest wave speed of the updated volumes. One team corrects one cell, with one thread per volume
 * on a device and with one thread on the host.
 *
 * With a FallbackDerived, every patch then judges its corrected volumes with
 * exahype2::fv::ader::Fallback. A troubled patch or a patch with a failed prediction repeats its step from
 * previousSolution and the halo with FallbackDerived, keeping the ADER-WENO flux on the faces where
 * it does not share the fallback flux with its neighbour: the faces of a level jump and, with
 * limiterFaceStatuses, the faces to a cell off the subcell scheme. Every patch writes its status
 * into statuses and into its own half of faceStatuses, and the VolumeStatus of each volume into
 * volumeStatuses. With a fallback per volume, only the troubled volumes repeat their step, a kept
 * volume takes the fallback flux on its faces to them inside the patch from its update in
 * interiorUpdates, and every volume on a patch face writes its own status into its half of the face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename FallbackDerived>
struct exahype2::fv::ader::CorrectPatches final {
  using Scheme     = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using Patch      = exahype2::fv::Patch<Derived, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool  = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  static constexpr bool FallsBack = not std::is_void_v<FallbackDerived>;

  peano4::CellConnectivity              cellConnectivity{};
  tarch::Span<const int>         cells{};
  typename CellPool::View        solution{};
  typename StatePool::ConstView  faceFluxes{};
  double                         timeStamp{0.0};
  double                         timeStepSize{0.0};
  typename CellPool::ConstView   previousSolution{};
  typename FacePool::ConstView   halo{};
  typename StatusPool::View      statuses{};
  typename StatusPool::View      faceStatuses{};
  typename StatusPool::ConstView limiterFaceStatuses{};
  typename StatePool::ConstView  interiorUpdates{};
  typename StatusPool::View      volumeStatuses{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    CorrectionStatistics&          accumulator
  ) const;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getSharedFallbackFaces(int slot) const;
};

/**
 * Swaps, in every listed patch that keeps its corrected step, the ADER-WENO flux for the fallback
 * flux on every face whose other side repeats its step with the fallback scheme and shares that
 * flux, and reduces the largest wave speed of the patches it changes. One team fixes one cell. With
 * a fallback per volume, this holds for each kept volume on a face and the volume behind it.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision, typename FallbackDerived>
struct exahype2::fv::ader::FixUpPatches final {
  using Correct    = exahype2::fv::ader::CorrectPatches<Derived, StoragePrecision, ComputePrecision, FallbackDerived>;
  using Patch      = typename Correct::Patch;
  using CellPool   = typename Correct::CellPool;
  using FacePool   = typename Correct::FacePool;
  using StatePool  = typename Correct::StatePool;
  using StatusPool = typename Correct::StatusPool;

  Correct correct{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    double&                        accumulator
  ) const;
};

/**
 * Fills the coarse-side halves of the fine faces of a level jump, states and fluxes, from the
 * coarse face.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::fv::ader::InterpolateFaceHalves final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  typename StatePool::View faceStates{};
  typename StatePool::View faceFluxes{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Restricts the numerical fluxes of the fine faces of a level jump into the fine-side flux half of
 * the coarse face. A hanging fine face contributes its coarse-side half, every other fine face its
 * fine-side half.
 */
template <typename Derived, typename ComputePrecision>
struct exahype2::fv::ader::RestrictFluxHalves final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::CellConnectivity        cellConnectivity{};
  typename StatePool::View faceFluxes{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/Kernels.cpph"
