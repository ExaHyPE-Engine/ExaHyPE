// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/Detector.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/dg/limiting/SubcellScheme.h"
#include "peano4/CellConnectivity.h"
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Regrid.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

namespace exahype2::dg::limiting {
  template <
    typename Derived,
    typename StoragePrecision,
    typename ComputePrecision,
    template <typename, typename, typename> class SubcellFamily>
  class SubcellLimiter;
} // namespace exahype2::dg::limiting

/**
 * The a-posteriori subcell limiter of one ADER-DG solver. A cell whose candidate fails the
 * physical admissibility test or the discrete maximum principle recomputes its step with its
 * first ring of neighbours on the subcell scheme while the second ring supplies its subgrid. The
 * scheme follows Dumbser, Zanotti, Loubère and Diot, J. Comput. Phys. 278 (2014) 47-75.
 *
 * SubcellFamily<Derived, StoragePrecision, ComputePrecision> advances the subgrids of the
 * recomputing cells, such as exahype2::fv::singlestep::SubcellStep or
 * exahype2::fv::ader::SubcellStep. On a face with one recomputing side, the step takes the other
 * side from the subgrid of that side.
 */
template <
  typename Derived,
  typename StoragePrecision,
  typename ComputePrecision,
  template <typename, typename, typename> class SubcellFamily>
class exahype2::dg::limiting::SubcellLimiter final {
public:
  template <typename Precision, typename Generated = Derived>
  using SubcellProjection = typename Generated::template SubcellProjection<Precision>;

  using Detector      = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using SubcellScheme = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using CellPool      = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool    = tarch::accelerator::ChunkedMemoryPool<int>;
  using FluxPool      = tarch::accelerator::ChunkedMemoryPool<ComputePrecision>;
  using Family        = SubcellFamily<Derived, StoragePrecision, ComputePrecision>;

  SubcellLimiter()                                       = delete;
  SubcellLimiter(const SubcellLimiter& other)            = delete;
  SubcellLimiter(SubcellLimiter&& other)                 = delete;
  SubcellLimiter& operator=(const SubcellLimiter& other) = delete;
  SubcellLimiter& operator=(SubcellLimiter&& other)      = delete;
  ~SubcellLimiter()                                      = default;

  INLINE_METHOD SubcellLimiter(double relaxationParameter, double differencesScaling);

  INLINE_METHOD void allocate(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void release();
  [[nodiscard]] INLINE_METHOD tarch::AllocatedBytes getAllocatedBytes() const;

  INLINE_METHOD void initialise(const peano4::CellConnectivity& cellConnectivity, const CellPool& solution);
  INLINE_METHOD void openTimeStep(const peano4::CellConnectivity& cellConnectivity, const CellPool& solution);
  [[nodiscard]] INLINE_METHOD exahype2::dg::MaxEigenvalueStatistics closeTimeStep(
    const peano4::CellConnectivity& cellConnectivity,
    CellPool&                       solution,
    double                          timeStamp,
    double                          timeStepSize
  );
  INLINE_METHOD void regrid(
    const peano4::CellConnectivity& cellConnectivity,
    const peano4::Regrid&           regrid,
    const CellPool&                 solution
  );
  INLINE_METHOD void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  INLINE_METHOD void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);

  [[nodiscard]] INLINE_METHOD tarch::accelerator::ChunkedMemoryPool<int>::View getStatuses();
  [[nodiscard]] INLINE_METHOD typename FluxPool::View getAppliedFluxes();
  [[nodiscard]] INLINE_METHOD const StatusPool& downloadStatuses();
  [[nodiscard]] INLINE_METHOD const CellPool& downloadSubcells();
  [[nodiscard]] INLINE_METHOD int getNumberOfTroubledCells() const;
  [[nodiscard]] INLINE_METHOD int getGlobalNumberOfTroubledCells() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfRecomputedCells() const;
  [[nodiscard]] INLINE_METHOD int getNumberOfCellsOnSubcellScheme() const;

private:
  INLINE_METHOD void rebuild(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void projectNodesOfFreedCells(const std::vector<int>& freedSlots, const CellPool& solution);
  INLINE_METHOD void sampleSubcells(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void clearFaces(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void openStep(const peano4::CellConnectivity& cellConnectivity, const CellPool& solution);
  INLINE_METHOD void reduceExtrema(tarch::Span<const int> cells, const CellPool& solution);
  INLINE_METHOD void reserveDetectorScratch(int numberOfTeams, int scratchValuesPerTeam);
  INLINE_METHOD void mergeNeighbourExtrema(const peano4::CellConnectivity& cellConnectivity, bool mergeLastRound);
  [[nodiscard]] INLINE_METHOD exahype2::dg::limiting::JudgedCells judgeCandidates(
    const peano4::CellConnectivity& cellConnectivity,
    const CellPool&                 solution,
    double                          timeStamp,
    double                          timeStepSize
  );
  INLINE_METHOD void spreadStatuses(const peano4::CellConnectivity& cellConnectivity);
  INLINE_METHOD void listCellsOnSubcellScheme(const peano4::CellConnectivity& cellConnectivity);
  [[nodiscard]] INLINE_METHOD exahype2::dg::MaxEigenvalueStatistics recompute(
    const peano4::CellConnectivity& cellConnectivity,
    CellPool&                       solution,
    double                          timeStamp,
    double                          timeStepSize
  );

  template <typename Interpolate, typename Restrict, typename Exchange, typename Work>
  INLINE_METHOD void exchangeFaces(
    const peano4::CellConnectivity& cellConnectivity,
    const Interpolate&              interpolation,
    const Restrict&                 restriction,
    Exchange&                       exchange,
    const Work&                     workOnCells
  );

  const double _relaxationParameter;
  const double _differencesScaling;

  CellPool   _previousSolution{SubcellScheme::ValuesPerCell};
  CellPool   _subcells{SubcellScheme::SubcellValuesPerCell};
  CellPool   _subcellHalo{SubcellScheme::HaloValuesPerFace};
  /**
   * The numerical flux that each side of a face applies in the Galerkin step. The Riemann problems
   * of every step fill it, since the cells that recompute are known only after the step.
   */
  FluxPool   _appliedFluxes{SubcellScheme::AppliedFluxValuesPerFace};
  StatusPool _statuses{exahype2::dg::limiting::NumberOfStatusEntries};
  StatusPool _faceStatuses{Detector::StatusValuesPerFace};
  CellPool   _referenceExtrema{Detector::ExtremaPerCell};
  CellPool   _extremaFaces{Detector::ExtremaValuesPerFace};

  peano4::HaloExchange<StoragePrecision>      _haloExchange{_subcellHalo};
  peano4::HaloExchange<int>                   _statusExchange{_faceStatuses};
  peano4::HaloExchange<StoragePrecision>      _extremaExchange{_extremaFaces};
  peano4::MigrationExchange<StoragePrecision> _subcellMigration{_subcells.getLocation()};
  peano4::MigrationExchange<int>              _statusMigration{_statuses.getLocation()};

  StatusPool _statusesOnHost{exahype2::dg::limiting::NumberOfStatusEntries, tarch::MemoryLocation::Heap};
  CellPool   _subcellsOnHost{SubcellScheme::SubcellValuesPerCell, tarch::MemoryLocation::Heap};

  tarch::accelerator::Buffer<int> _cellsOnSubcellScheme{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int> _recomputedCells{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int> _freedCells{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<exahype2::dg::limiting::SubcellSchemeCounts> _cellCountsOfBlocks{
    tarch::MemoryLocation::Device
  };
  exahype2::dg::limiting::SubcellSchemeCounts _cellCounts{};

  int  _localTroubledCells{0};
  int  _globalTroubledCells{0};
  int  _troubledCellsSlot{0};
  bool _extremaOfSolutionAreReduced{false};

  tarch::accelerator::Buffer<StoragePrecision> _recomputeScratch{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<StoragePrecision> _detectorScratch{tarch::MemoryLocation::Device};

  Family _family{};
};

#include "exahype2/dg/limiting/SubcellLimiter.cpph"
