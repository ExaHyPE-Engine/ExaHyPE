// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/fv/FiniteVolume.h"
#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace exahype2::fv {
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getBoundaryFaceMask(const peano4::Partition& partition, int cell);

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct InitialisePatches;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ApplyBoundaryConditions;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct InterpolateHaloHalves;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct InjectHaloHalves;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct RestrictHaloHalves;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct ProjectPatchesOntoFaces;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct UpdatePatches;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct MeasurePatchEigenvalues;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct MeasurePatchErrors;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct PatchTransfer;
} // namespace exahype2::fv

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::InitialisePatches final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  typename CellPool::View solution{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ApplyBoundaryConditions final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  tarch::Span<const int>  boundaryFaces{};
  typename FacePool::View halo{};
  double                  timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills the coarse-side halves of the fine faces of a level jump from its coarse face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::InterpolateHaloHalves final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  typename FacePool::View halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Fills the coarse-side halves of the fine faces of a level jump with copies of its coarse face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::InjectHaloHalves final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  typename FacePool::View halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Fills the fine-side half of the coarse face of a level jump from its fine faces.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::RestrictHaloHalves final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition       partition{};
  typename FacePool::View halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ProjectPatchesOntoFaces final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  typename CellPool::ConstView solution{};
  typename FacePool::View      halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::UpdatePatches final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  tarch::Span<const int>       cells{};
  typename CellPool::ConstView input{};
  typename CellPool::View      output{};
  typename FacePool::ConstView halo{};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, double& accumulator) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::MeasurePatchEigenvalues final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::Partition            partition{};
  typename CellPool::ConstView solution{};
  double                       timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, double& accumulator) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::MeasurePatchErrors final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int NumberOfNorms = exahype2::ErrorMeasurement::NumberOfNorms;

  peano4::Partition            partition{};
  typename CellPool::ConstView solution{};
  double*                      errorsPerCell{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills patches of the new grid from source, the pool of the old grid, during a regrid.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::PatchTransfer final {
  using Kernel   = exahype2::fv::FiniteVolume<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  typename CellPool::ConstView source{};

  INLINE_METHOD GPU_CALLABLE_METHOD void copy(int slot, StoragePrecision* target) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
    int               parentSlot,
    StoragePrecision* target,
    int               childPosition
  ) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(const int* childSlots, StoragePrecision* target) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/Kernels.cpph"
