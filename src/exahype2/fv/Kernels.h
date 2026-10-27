// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/ErrorMeasurement.h"
#include "exahype2/fv/Patch.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"

/**
 * @file
 * The loop functors a finite-volume solver runs over the cells, the faces and the level jumps of
 * the cell connectivity. Every loop body calls functions of exahype2::fv::Patch on the patch or
 * face of its work item.
 */
TARGET_DECLARE
namespace exahype2::fv {
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD int getBoundaryFaceMask(
    const peano4::CellConnectivity& cellConnectivity,
    int                             cell
  );

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
  struct MeasurePatchEigenvalues;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct MeasurePatchErrors;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct PatchTransfer;
} // namespace exahype2::fv

/**
 * Fills the outer half of every listed boundary face through the boundary conditions. The launch
 * runs one thread per volume of a layer of every listed face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ApplyBoundaryConditions final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity cellConnectivity{};
  tarch::Span<const int>   boundaryFaces{};
  typename FacePool::View  halo{};
  double                   timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills the coarse-side halves of the fine faces of a level jump from its coarse face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::InterpolateHaloHalves final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity cellConnectivity{};
  typename FacePool::View  halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Fills the coarse-side halves of the fine faces of a level jump with copies of its coarse face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::InjectHaloHalves final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity cellConnectivity{};
  typename FacePool::View  halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Fills the fine-side half of the coarse face of a level jump from its fine faces.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::RestrictHaloHalves final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity cellConnectivity{};
  typename FacePool::View  halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const peano4::LevelJump& levelJump) const;
};

/**
 * Copies the boundary layers of the patch of every cell into the halves of its faces that the
 * cell writes. The launch runs one team per cell and one thread per value of one half.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::ProjectPatchesOntoFaces final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity     cellConnectivity{};
  typename CellPool::ConstView solution{};
  typename FacePool::View      halo{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, tarch::accelerator::TeamHandle team) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::MeasurePatchEigenvalues final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity     cellConnectivity{};
  typename CellPool::ConstView solution{};
  double                       timeStamp{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, double& accumulator) const;
};

template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::MeasurePatchErrors final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int NumberOfNorms = exahype2::ErrorMeasurement::NumberOfNorms;

  peano4::CellConnectivity     cellConnectivity{};
  typename CellPool::ConstView solution{};
  double*                      errorsPerCell{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/**
 * Fills patches of the new grid from source, the pool of the old grid, during a regrid. Every work
 * item fills one volume of a patch.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::PatchTransfer final {
  using Kernel   = exahype2::fv::Patch<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  static constexpr int WorkItemsPerChunk      = Kernel::VolumesPerPatch;
  static constexpr int PreparedValuesPerChunk = 0;

  typename CellPool::ConstView source{};

  INLINE_METHOD GPU_CALLABLE_METHOD void copy(int slot, StoragePrecision* target, int itemOfChunk) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void interpolateFromParent(
    int               parentSlot,
    StoragePrecision* target,
    int               childPosition,
    int               itemOfChunk,
    const double*     prepared
  ) const;
  INLINE_METHOD GPU_CALLABLE_METHOD void restrictFromChildren(
    const int*        childSlots,
    StoragePrecision* target,
    int               itemOfChunk,
    const double*     prepared
  ) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/Kernels.cpph"
