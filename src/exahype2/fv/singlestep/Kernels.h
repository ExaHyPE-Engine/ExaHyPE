// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/Kernels.h"
#include "exahype2/fv/singlestep/PatchUpdate.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"

/**
 * @file
 * The loop functors of the single-step finite-volume solver family over the cells of the
 * cellConnectivity. Every loop body calls functions of exahype2::fv::singlestep::PatchUpdate.
 */
TARGET_DECLARE
namespace exahype2::fv::singlestep {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct UpdatePatches;
} // namespace exahype2::fv::singlestep

/**
 * Advances the patch of every listed cell in place and reduces the largest wave speed. One team
 * advances one cell with one thread per volume on a device and with one thread on the host.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::singlestep::UpdatePatches final {
  using Kernel   = exahype2::fv::singlestep::PatchUpdate<Derived, ComputePrecision>;
  using CellPool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;

  peano4::CellConnectivity            cellConnectivity{};
  tarch::Span<const int>       cells{};
  typename CellPool::View      solution{};
  typename FacePool::ConstView halo{};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
    int                            entry,
    tarch::accelerator::TeamHandle team,
    double&                        accumulator
  ) const;

private:
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double updatePatchOnDevice(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision*                            patch,
    const StoragePrecision* const                faces[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          boundaryFaces
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/singlestep/Kernels.cpph"
