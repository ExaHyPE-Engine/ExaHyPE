// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "exahype2/fv/ader/Kernels.h"
#include "peano4/Partition.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace exahype2::fv::ader {
  template <typename StoragePrecision, typename ComputePrecision>
  class PatchPredictor;
} // namespace exahype2::fv::ader

/**
 * The predictor phase of an ADER-WENO step over the patches of a solver Derived. A launch runs at
 * most as many teams as the device runs at once, so the scratch of the teams does not grow with the
 * mesh. allocate() reserves it once together with the predictor tables of the reconstruction basis.
 *
 * Only the member functions name Derived. A family whose Derived is complete only at the use can
 * hold a predictor.
 */
template <typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::ader::PatchPredictor final {
public:
  using CellPool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FacePool  = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatePool = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;

  template <typename Derived>
  INLINE_METHOD void allocate();
  INLINE_METHOD void release();
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

  template <typename Derived>
  [[nodiscard]] INLINE_METHOD exahype2::fv::ader::PredictionStatistics predict(
    const peano4::Partition&     partition,
    tarch::Span<const int>       cells,
    typename CellPool::View      solution,
    typename FacePool::ConstView halo,
    typename StatePool::View     faceStates,
    typename StatePool::View     faceFluxes,
    double                       timeStamp,
    double                       timeStepSize
  );

private:
  template <typename Derived>
  using Scheme = exahype2::fv::ader::Scheme<Derived, ComputePrecision>;

  template <typename Derived>
  [[nodiscard]] static INLINE_METHOD int getTeamThreads();
  template <typename Derived>
  [[nodiscard]] static INLINE_METHOD int getThreadsPerVolume();
  template <typename Derived>
  [[nodiscard]] static INLINE_METHOD int getMaximumNumberOfTeams();
  template <typename Derived>
  [[nodiscard]] static INLINE_METHOD int getNumberOfTeams(int cells);
  template <typename Derived>
  [[nodiscard]] static INLINE_METHOD int getScratchSizePerTeam();
  template <typename Derived>
  [[nodiscard]] INLINE_METHOD exahype2::dg::BasisTables<ComputePrecision> getTables() const;

  /**
   * One chunk of getScratchSizePerTeam() values per team on a device and per thread on the host.
   */
  tarch::accelerator::Buffer<ComputePrecision> _scratch{tarch::MemoryLocation::Device};

  /**
   * The bytes of one Scheme<Derived>::Tables object.
   */
  tarch::accelerator::Buffer<ComputePrecision> _tables{tarch::MemoryLocation::Device};
};

#include "exahype2/fv/ader/PatchPredictor.cpph"
