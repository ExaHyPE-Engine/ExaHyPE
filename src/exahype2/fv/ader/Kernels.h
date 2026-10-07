// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/FiniteVolumeScheme.h"
#include "exahype2/fv/ader/Scheme.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"

/**
 * @file
 * The loop functors of the ADER-WENO solver family over the cells, the faces and the level jumps of
 * the partition. Every loop body calls functions of exahype2::fv::ader::Scheme.
 */
TARGET_DECLARE
namespace exahype2::fv::ader {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct PredictPatches;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct SolveRiemannProblems;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct CorrectPatches;

  template <typename Derived, typename ComputePrecision>
  struct InterpolateFaceHalves;

  template <typename Derived, typename ComputePrecision>
  struct RestrictFluxHalves;
} // namespace exahype2::fv::ader

/**
 * Predicts every volume of every listed cell, adds the updates inside the patch to the patch and
 * writes the predicted states and fluxes of the faces of the patch into their own halves. Reduces
 * the largest wave speed of the faces inside the patches. One team advances one cell, with one
 * thread per volume on a device, which keeps the predictions inside the patch in the scratch chunk
 * of its entry in scratch.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::PredictPatches final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::View      solution{};
  typename FacePool::ConstView halo{};
  typename StatePool::View     faceStates{};
  typename StatePool::View     faceFluxes{};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};
  ComputePrecision*            scratch{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    double&                        accumulator
  ) const;

private:
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double predictPatchOnDevice(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision*                            patch,
    const StoragePrecision* const                haloOfCell[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    ComputePrecision*                            scratchOfTeam,
    ComputePrecision* const                      stateHalves[peano4::FacesPerCell],
    ComputePrecision* const                      fluxHalves[peano4::FacesPerCell]
  );
};

/**
 * Solves the Riemann problems on every listed face of a patch and reduces the largest wave speed.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::SolveRiemannProblems final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition        partition{};
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
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ader::CorrectPatches final {
  using Scheme    = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;
  using Patch     = exahype2::fv::FiniteVolumeScheme<Derived, ComputePrecision>;
  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  peano4::Partition             partition{};
  tarch::Span<const int>        cells{};
  typename CellPool::View       solution{};
  typename StatePool::ConstView faceFluxes{};
  double                        timeStamp{0.0};
  double                        timeStepSize{0.0};

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

  peano4::Partition        partition{};
  typename StatePool::View faceFluxes{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/ader/Kernels.cpph"
