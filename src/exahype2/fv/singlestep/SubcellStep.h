// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/limiting/SubcellScheme.h"
#include "exahype2/fv/singlestep/SingleStep.h"
#include "peano4/CellConnectivity.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::singlestep {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class SubcellStep;

  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  struct AdvanceSubgrids;
} // namespace exahype2::fv::singlestep

TARGET_DECLARE
/**
 * The step of the subcell limiter of the ADER-DG solver Derived with a finite-volume scheme that
 * advances every volume in one step from the volumes around it, such as Godunov and MUSCL-Hancock.
 * Every recomputing cell advances its subgrid from its subcells and its halo, the local cells while
 * the ranks exchange the halo of the remote cells. A solver with a fallback reconstruction repeats
 * the step of a cell with it when an evolved subcell fails the physical admissibility test. On a
 * face with one recomputing side, the volumes of that side apply the numerical flux of the Galerkin
 * step instead of their own, averaged over each volume face.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::fv::singlestep::SubcellStep final {
public:
  using Kernel = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using Region = exahype2::dg::limiting::SubcellRegion<StoragePrecision, ComputePrecision>;

  /**
   * The subgrid solver of Derived with the reconstruction Reconstruct, which advances every volume
   * in one step.
   */
  template <typename Reconstruct>
  struct Subgrid final: Kernel::template SubgridSolver<Reconstruct> {
    using TimeIntegration = exahype2::fv::singlestep::SingleStep;
  };

  INLINE_METHOD void rebuild(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void release();
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

  template <typename ExchangeHalo>
  INLINE_METHOD void advance(
    const peano4::CellConnectivity& cellConnectivity,
    const Region&            region,
    const ExchangeHalo&      exchangeHalo,
    double                   timeStamp,
    double                   timeStepSize
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void advanceSubgrid(
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize,
    const ComputePrecision* const                appliedHalves[peano4::FacesPerCell],
    const double*                                nodesToSubcells
  );

  static INLINE_METHOD GPU_CALLABLE_METHOD void advanceSubgridInTeam(
    tarch::accelerator::TeamHandle               team,
    StoragePrecision* NOALIAS                    subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize,
    const ComputePrecision* const                appliedHalves[peano4::FacesPerCell],
    const double*                                nodesToSubcells
  );

private:
  template <typename Reconstruct>
  static INLINE_METHOD GPU_CALLABLE_METHOD void advanceSubgridWith(
    const StoragePrecision* NOALIAS              subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <typename Reconstruct>
  static INLINE_METHOD GPU_CALLABLE_METHOD void imposeAppliedFluxes(
    int                                          volume,
    const StoragePrecision* NOALIAS              subcells,
    StoragePrecision* NOALIAS                    evolvedSubcells,
    const StoragePrecision* const                haloHalves[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    int                                          boundaryFaces,
    double                                       timeStamp,
    double                                       timeStepSize,
    const ComputePrecision* const                appliedHalves[peano4::FacesPerCell],
    const double*                                nodesToSubcells
  );

  /**
   * One evolved subgrid per scratch chunk of the dispatch.
   */
  tarch::accelerator::Buffer<StoragePrecision> _scratch{tarch::MemoryLocation::Device};
};

/**
 * Advances the subgrid of every listed cell in place. The evolved subgrid passes through the
 * scratch chunk of the work item.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
struct exahype2::fv::singlestep::AdvanceSubgrids final {
  using Step       = exahype2::fv::singlestep::SubcellStep<Derived, StoragePrecision, ComputePrecision>;
  using CellPool   = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using FluxPool   = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using StatusPool = tarch::accelerator::ChunkedMemoryPool<int>;

  peano4::CellConnectivity              cellConnectivity{};
  tarch::Span<const int>         cells{};
  typename CellPool::View        subcells{};
  typename CellPool::ConstView   halo{};
  typename StatusPool::ConstView faceStatuses{};
  typename FluxPool::ConstView   appliedFluxes{};
  const double*                  nodesToSubcells{nullptr};
  StoragePrecision*              scratch{nullptr};
  double                       timeStamp{0.0};
  double                       timeStepSize{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry, tarch::accelerator::TeamHandle team) const;
};
TARGET_DECLARE_END

#include "exahype2/fv/singlestep/SubcellStep.cpph"
