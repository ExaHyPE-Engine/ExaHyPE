// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/Detector.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/dg/limiting/SubcellScheme.h"
#include "peano4/HaloExchange.h"
#include "peano4/MigrationExchange.h"
#include "peano4/Partition.h"
#include "peano4/Regrid.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/MemoryLocation.h"

namespace exahype2::dg::limiting {
  template <typename Derived, typename StoragePrecision, typename ComputePrecision>
  class SubcellLimiter;
} // namespace exahype2::dg::limiting

/**
 * The a-posteriori subcell limiter of one ADER-DG solver: Every cell holds a status, a subgrid
 * of finite volumes and the solution of the previous time step. After the corrector the
 * candidate of every cell is tested against the physical admissibility term and the discrete
 * maximum principle of the solver; a rejected cell and its first ring of neighbours recompute
 * their step on the subcell scheme from the previous solution, and the second ring supplies its
 * subgrid. Derived is the generated solver class as for exahype2::dg::ader::Solver and adds
 * the subcell scheme, the observables of the discrete maximum principle and the projection
 * tables SubcellProjection between the nodes and the subcells.
 */
template <typename Derived, typename StoragePrecision, typename ComputePrecision>
class exahype2::dg::limiting::SubcellLimiter final {
public:
  // The defaulted parameter defers the lookup in Derived to the use, where Derived is complete.
  template <typename Precision, typename Generated = Derived>
  using SubcellProjection = typename Generated::template SubcellProjection<Precision>;

  using Detector      = exahype2::dg::limiting::Detector<Derived, ComputePrecision>;
  using SubcellScheme = exahype2::dg::limiting::SubcellScheme<Derived, ComputePrecision>;
  using CellPool      = tarch::accelerator::ChunkedMemoryPool<StoragePrecision>;
  using StatusPool    = tarch::accelerator::ChunkedMemoryPool<int>;

  SubcellLimiter()                                 = delete;
  SubcellLimiter(const SubcellLimiter&)            = delete;
  SubcellLimiter(SubcellLimiter&&)                 = delete;
  SubcellLimiter& operator=(const SubcellLimiter&) = delete;
  SubcellLimiter& operator=(SubcellLimiter&&)      = delete;
  ~SubcellLimiter()                                = default;

  SubcellLimiter(double relaxationParameter, double differencesScaling);

  void allocate(const peano4::Partition& partition);
  void release();
  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;

  void initialise(const peano4::Partition& partition, const CellPool& solution);
  void openTimeStep(const peano4::Partition& partition, const CellPool& solution);
  [[nodiscard]] exahype2::dg::MaxEigenvalueStatistics closeTimeStep(
    const peano4::Partition& partition,
    CellPool&                solution,
    double                   timeStamp,
    double                   timeStepSize
  );
  void regrid(const peano4::Partition& partition, const peano4::Regrid& regrid, const CellPool& solution);

  [[nodiscard]] typename StatusPool::View getStatuses();
  [[nodiscard]] const int* downloadStatuses();
  [[nodiscard]] const StoragePrecision* downloadSubcells();
  [[nodiscard]] int getNumberOfTroubledCells() const;
  [[nodiscard]] int getGlobalNumberOfTroubledCells() const;
  [[nodiscard]] int getNumberOfRecomputedCells() const;
  [[nodiscard]] int getNumberOfCellsOnSubcellScheme() const;

private:
  void rebuild(const peano4::Partition& partition);
  void sampleSubcells(const peano4::Partition& partition);
  void clearFaces(const peano4::Partition& partition);
  void openStep(const peano4::Partition& partition, const CellPool& solution);
  void reduceReferenceExtrema(const peano4::Partition& partition);
  [[nodiscard]] exahype2::dg::MaxEigenvalueStatistics judgeCandidates(
    const peano4::Partition& partition,
    const CellPool&          solution,
    double                   timeStamp,
    double                   timeStepSize
  );
  void spreadStatuses(const peano4::Partition& partition);
  void listCellsOnSubcellScheme(const peano4::Partition& partition);
  [[nodiscard]] exahype2::dg::MaxEigenvalueStatistics recompute(
    const peano4::Partition& partition,
    CellPool&                solution,
    double                   timeStamp,
    double                   timeStepSize
  );

  template <typename Interpolate, typename Restrict, typename Exchange, typename Work>
  void exchangeFaces(
    const peano4::Partition& partition,
    const Interpolate&       interpolation,
    const Restrict&          restriction,
    Exchange&                exchange,
    const Work&              workOnCells
  );

  const double _relaxationParameter;
  const double _differencesScaling;

  CellPool   _previousSolution{SubcellScheme::ValuesPerCell};
  CellPool   _subcells{SubcellScheme::SubcellValuesPerCell};
  CellPool   _subcellHalo{SubcellScheme::HaloValuesPerFace};
  StatusPool _statuses{exahype2::dg::limiting::NumberOfStatusEntries};
  StatusPool _faceStatuses{Detector::StatusValuesPerFace};
  CellPool   _referenceExtrema{Detector::ExtremaPerCell};
  CellPool   _extremaFaces{Detector::ExtremaValuesPerFace};

  peano4::HaloExchange<StoragePrecision>      _haloExchange{_subcellHalo};
  peano4::HaloExchange<int>                   _statusExchange{_faceStatuses};
  peano4::HaloExchange<StoragePrecision>      _extremaExchange{_extremaFaces};
  peano4::MigrationExchange<StoragePrecision> _subcellMigration{_subcells.getLocation()};
  peano4::MigrationExchange<int>              _statusMigration{_statuses.getLocation()};

  std::vector<int>              _statusesOnHost{};
  std::vector<StoragePrecision> _subcellsOnHost{};

  /**
   * The slots of the cells whose status is not Regular and of the cells that recompute, one entry per cell.
   */
  tarch::accelerator::Buffer<int> _cellsOnSubcellScheme{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<int> _recomputedCells{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<exahype2::dg::limiting::SubcellSchemeCounts> _cellCountsOfBlocks{
    tarch::MemoryLocation::Device
  };
  exahype2::dg::limiting::SubcellSchemeCounts _cellCounts{};
  /**
   * Troubled cells of all ranks at the end of the last closeTimeStep().
   */
  int _globalTroubledCells{0};

  /**
   * One evolved subgrid per scratch chunk of the recompute dispatch.
   */
  tarch::accelerator::Buffer<StoragePrecision> _recomputeScratch{tarch::MemoryLocation::Device};
};

#include "exahype2/dg/limiting/SubcellLimiter.cpph"
