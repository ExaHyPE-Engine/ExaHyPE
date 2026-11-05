// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/limiting/SubcellScheme.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/singlestep/SubcellStep.h"
#include "exahype2/terms/Flux.h"
#include "exahype2/terms/MaxEigenvalue.h"
#include "exahype2/terms/RiemannSolver.h"
#include "exahype2/terms/SourceTerm.h"
#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/accelerator/ParallelReduce.h"
#include "tarch/accelerator/Range.h"
#include "tarch/accelerator/Reducer.h"
#include "tarch/accelerator/Scratch.h"
#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"
#include "unittests/exahype2/dg/KernelTestData.h"
#include "unittests/exahype2/dg/limiting/LimiterTestData.h"

namespace {
  using Layout = exahype2::dg::limiting::unittests::StubLayout;
  using Setup  = exahype2::dg::unittests::KernelTestSetup;
  using Scheme = exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::unittests::StubSolver, double>;
  using FixedScheme =
    exahype2::dg::limiting::SubcellScheme<exahype2::dg::limiting::unittests::FixedStepStubSolver, double>;

  using RuntimeParameters = exahype2::dg::limiting::unittests::RuntimeParameters;

  /**
   * The stub solver with a subcell Riemann solver that reports SubcellWaveSpeed and writes no flux.
   */
  struct RiemannStubSolver final : exahype2::dg::limiting::unittests::StubSolver {
    static constexpr double SubcellWaveSpeed = 2.0;

    static INLINE_METHOD GPU_CALLABLE_METHOD double solveSubcellRiemannProblem(
      [[maybe_unused]] const double* const                          QL,
      [[maybe_unused]] const double* const                          QR,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
      [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
      [[maybe_unused]] const double                                 t,
      [[maybe_unused]] const double                                 dt,
      [[maybe_unused]] const int                                    normal,
      [[maybe_unused]] const bool                                   isBoundaryFace,
      [[maybe_unused]] const int                                    faceIndex,
      [[maybe_unused]] double* const                                FL,
      [[maybe_unused]] double* const                                FR,
      [[maybe_unused]] const RuntimeParameters&                     parameters
    ) {
      return SubcellWaveSpeed;
    }
  };

  /**
   * Returns the padded chunks of every slot of pool in slot order.
   */
  template <typename T>
  std::vector<T> getChunksOnHost(const tarch::accelerator::ChunkedMemoryPool<T>& pool) {
    tarch::accelerator::ChunkedMemoryPool<T>        copyOnHost(pool.getChunkSize(), tarch::MemoryLocation::Heap);
    const tarch::accelerator::ChunkedMemoryPool<T>& onHost = pool.download(copyOnHost);
    const auto                                      padded = static_cast<std::size_t>(pool.getPaddedChunkSize());
    std::vector<T>                                  values(static_cast<std::size_t>(pool.getCapacity()) * padded);
    for (int slot = 0; slot < pool.getCapacity(); slot++) {
      std::copy_n(onHost.getChunk(slot), padded, values.data() + static_cast<std::size_t>(slot) * padded);
    }
    return values;
  }

  /**
   * Overwrites the padded chunks of every slot of pool with values in slot order.
   */
  template <typename T>
  void setChunksFromHost(tarch::accelerator::ChunkedMemoryPool<T>& pool, const std::vector<T>& values) {
    tarch::accelerator::ChunkedMemoryPool<T>  copyOnHost(pool.getChunkSize(), tarch::MemoryLocation::Heap);
    tarch::accelerator::ChunkedMemoryPool<T>& onHost = pool.download(copyOnHost);
    const auto                                padded = static_cast<std::size_t>(pool.getPaddedChunkSize());
    for (int slot = 0; slot < pool.getCapacity(); slot++) {
      std::copy_n(values.data() + static_cast<std::size_t>(slot) * padded, padded, onHost.getChunk(slot));
    }
    pool.upload(onHost);
  }

  TARGET_DECLARE
  constexpr int HaloValuesPerFace = Scheme::HaloValuesPerFace;
  constexpr int HaloValuesPerCell = peano4::FacesPerCell * HaloValuesPerFace;

  /**
   * Advances the subgrid of one cell with the single-step subcell family and projects it onto the
   * nodes, as the limiter recomputes a cell.
   */
  template <typename Solver>
  INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::MaxEigenvalueStatistics recomputeSubgrid(
    double* const                                nodes,
    double* const                                subcells,
    double* const                                evolvedSubcells,
    const double* const                          haloHalves[peano4::FacesPerCell],
    const double* const                          subcellsToNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const int                                    boundaryFaces,
    const double                                 timeStamp,
    const double                                 timeStepSize
  ) {
    const double* const noAppliedFluxes[peano4::FacesPerCell]{};
    exahype2::fv::singlestep::SubcellStep<Solver, double, double>::advanceSubgrid(
      subcells,
      evolvedSubcells,
      haloHalves,
      cellCentre,
      cellSize,
      boundaryFaces,
      timeStamp,
      timeStepSize,
      noAppliedFluxes,
      nullptr,
      RuntimeParameters{}
    );
    return exahype2::dg::limiting::SubcellScheme<Solver, double>::projectRecomputedSubgrid(
      nodes,
      subcells,
      subcellsToNodes,
      cellCentre,
      cellSize,
      timeStamp,
      timeStepSize,
      RuntimeParameters{}
    );
  }
  TARGET_DECLARE_END

  constexpr double Tolerance = 1.0e-12;

  /**
   * The arguments of every visit of the subcell loop in the order of the visits.
   */
  struct Visits final {
    int                                   numberOfVisits{0};
    const double*                         values[Layout::SubcellsPerCell]{};
    tarch::la::Vector<DIMENSIONS, double> centres[Layout::SubcellsPerCell]{};
    tarch::la::Vector<DIMENSIONS, double> sizes[Layout::SubcellsPerCell]{};
  };

  std::vector<double> getConstantSubgrid(const double unknown, const double auxiliary) {
    std::vector<double> subgrid(Layout::SubcellValuesPerCell);
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = unknown;
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = auxiliary;
    }
    return subgrid;
  }

  std::vector<double> getConstantNodes(const double unknown, const double auxiliary) {
    std::vector<double> nodes(Layout::ValuesPerCell);
    for (int node = 0; node < Layout::NodesPerCell; node++) {
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData]     = unknown;
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData + 1] = auxiliary;
    }
    return nodes;
  }

  std::vector<double> getConstantHalo(const double unknown, const double auxiliary) {
    std::vector<double> halo(HaloValuesPerCell);
    for (std::size_t entry = 0; entry < halo.size(); entry += Layout::NumberOfData) {
      halo[entry]     = unknown;
      halo[entry + 1] = auxiliary;
    }
    return halo;
  }

  /**
   * Returns the number of entries of values within the tolerance of value.
   */
  int countValues(const double* const values, const int count, const double value) {
    int result = 0;
    for (int entry = 0; entry < count; entry++) {
      result += std::abs(values[entry] - value) <= Tolerance ? 1 : 0;
    }
    return result;
  }

  TARGET_DECLARE
  struct ProjectRecomputedSubgridOfCell final {
    double*       nodes{nullptr};
    const double* subcells{nullptr};
    const double* subcellsToNodes{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    double*       scratch{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              cell,
      const tarch::accelerator::TeamHandle   team,
      exahype2::dg::MaxEigenvalueStatistics& accumulator
    ) const {
      const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      const exahype2::dg::MaxEigenvalueStatistics statistics       = Scheme::projectRecomputedSubgridInTeam(
        team,
        nodes + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        subcellsToNodes,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        Layout::TimeStamp,
        Layout::TimeStepSize,
        scratch + static_cast<std::ptrdiff_t>(cell) * Scheme::ProjectionScratchValues,
        RuntimeParameters{}
      );
      COMPILE_IF_ON_HOST(accumulator = exahype2::dg::MaxEigenvalueStatistics::combine(accumulator, statistics);)
      COMPILE_IF_ON_DEVICE(
        tarch::accelerator::reduceOverTeam(
          team,
          statistics,
          tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{accumulator}
        );
      )
    }
  };

  struct ProjectSubgridOfCell final {
    const int*    statuses{nullptr};
    double*       subcells{nullptr};
    const double* previousSolution{nullptr};
    double*       halo{nullptr};
    const double* nodesToSubcells{nullptr};
    double*       scratch{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      double* haloHalves[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        haloHalves[faceNumber] =
          halo + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloValuesPerFace;
      }
      Scheme::projectSubgridOntoFacesInTeam(
        team,
        statuses + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
        subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        previousSolution + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        haloHalves,
        nodesToSubcells,
        false,
        scratch + static_cast<std::ptrdiff_t>(cell) * Scheme::ProjectionScratchValues
      );
    }
  };

  using Step = exahype2::fv::singlestep::SubcellStep<exahype2::dg::limiting::unittests::StubSolver, double, double>;

  constexpr int AppliedValuesPerHalf = Scheme::AppliedFluxValuesPerFace / 2;
  constexpr int AppliedValuesPerCell = peano4::FacesPerCell * AppliedValuesPerHalf;

  /**
   * Points haloHalves and appliedHalves at the halo and the applied fluxes of cell. Every face with
   * an even number applies a Galerkin flux and every other face keeps the flux of the subgrid.
   */
  INLINE_METHOD GPU_CALLABLE_METHOD void getHalvesOfCell(
    const int           cell,
    const double* const halo,
    const double* const appliedFluxes,
    const double*       haloHalves[peano4::FacesPerCell],
    const double*       appliedHalves[peano4::FacesPerCell]
  ) {
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      const std::ptrdiff_t face = static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber;
      haloHalves[faceNumber]    = halo + face * HaloValuesPerFace;
      appliedHalves[faceNumber] = faceNumber % 2 == 0 ? appliedFluxes + face * AppliedValuesPerHalf : nullptr;
    }
  }

  /**
   * Advances the subgrid of one cell with a team, as exahype2::fv::singlestep::AdvanceSubgrids does.
   */
  struct AdvanceSubgridOfCell final {
    double*       subcells{nullptr};
    double*       scratch{nullptr};
    const double* halo{nullptr};
    const double* appliedFluxes{nullptr};
    const double* nodesToSubcells{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      const double* haloHalves[peano4::FacesPerCell];
      const double* appliedHalves[peano4::FacesPerCell];
      getHalvesOfCell(cell, halo, appliedFluxes, haloHalves, appliedHalves);
      const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
      Step::advanceSubgridInTeam(
        team,
        subcells + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        scratch + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        haloHalves,
        tarch::la::Vector<DIMENSIONS, double>(cellCentres + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(cellSizes + coordinateOffset),
        0,
        Layout::TimeStamp,
        Layout::TimeStepSize,
        appliedHalves,
        nodesToSubcells,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  /**
   * The subgrids, halos and applied fluxes of the cells of the cell chain.
   */
  struct SubgridSteps final {
    const exahype2::dg::limiting::unittests::SubcellProjections tables{};
    const exahype2::dg::unittests::CellChain                    geometry{};
    const std::vector<double>                                   subcells = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
      1.0,
      0.002
    );
    const std::vector<double> halo =
      exahype2::dg::unittests::getRamp(static_cast<std::size_t>(Setup::NumberOfCells) * HaloValuesPerCell, 1.5, 0.003);
    const std::vector<double> appliedFluxes = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * AppliedValuesPerCell,
      0.4,
      0.01
    );

    /**
     * Returns the subgrids after every cell advanced its subgrid with advanceSubgrid() on the host.
     */
    std::vector<double> advanceOnTheHost() const {
      std::vector<double> result = subcells;
      std::vector<double> evolved(Layout::SubcellValuesPerCell, 0.0);
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        const double* haloHalves[peano4::FacesPerCell];
        const double* appliedHalves[peano4::FacesPerCell];
        getHalvesOfCell(cell, halo.data(), appliedFluxes.data(), haloHalves, appliedHalves);
        const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
        Step::advanceSubgrid(
          result.data() + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
          evolved.data(),
          haloHalves,
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
          tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
          0,
          Layout::TimeStamp,
          Layout::TimeStepSize,
          appliedHalves,
          tables.nodesToSubcells.data(),
          RuntimeParameters{}
        );
      }
      return result;
    }
  };
} // namespace

UNIT_TEST("Every split of the stages projects like the whole cell") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  std::vector<double>                                     subcells(Layout::SubcellValuesPerCell);
  for (std::size_t entry = 0; entry < subcells.size(); entry++) {
    subcells[entry] = 1.0 + 0.37 * static_cast<double>(entry % 11) - 0.05 * static_cast<double>(entry);
  }
  std::vector<double> whole(Layout::ValuesPerCell);
  Scheme::projectSubcellsOntoNodes(subcells.data(), whole.data(), tables.subcellsToNodes.data());
  const auto weights = exahype2::dg::getProjectionWeightsOfDofPairs<Layout::SubcellsPerAxis, Layout::NodesPerAxis>(
    tables.subcellsToNodes.data()
  );
  for (const int threads : {1, 5, 32}) {
    std::vector<double> scratch(Scheme::ProjectionScratchValues);
    std::vector<double> split(Layout::ValuesPerCell);
    for (int stage = 0; stage < exahype2::dg::getStagesOfTensorProduct<DIMENSIONS>(); stage++) {
      for (int thread = 0; thread < threads; thread++) {
        Scheme::projectSubcellsOntoNodes(
          subcells.data(),
          split.data(),
          tables.subcellsToNodes.data(),
          weights,
          scratch.data(),
          stage,
          thread,
          threads
        );
      }
    }
    for (int value = 0; value < Layout::ValuesPerCell; value++) {
      VALIDATE_EQUALS(split[value], whole[value], threads, value);
    }
  }
}

/**
 * Every split of the stages of the projection onto the subcells writes the subcells of the whole
 * cell.
 */
UNIT_TEST("Every split of the stages projects the nodes like the whole cell") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  std::vector<double>                                     nodes(Layout::ValuesPerCell);
  for (std::size_t entry = 0; entry < nodes.size(); entry++) {
    nodes[entry] = 1.0 + 0.37 * static_cast<double>(entry % 11) - 0.05 * static_cast<double>(entry);
  }
  std::vector<double> whole(Layout::SubcellValuesPerCell);
  Scheme::projectNodesOntoSubcells(nodes.data(), whole.data(), tables.nodesToSubcells.data());
  const auto weights = exahype2::dg::getProjectionWeightsOfDofPairs<Layout::NodesPerAxis, Layout::SubcellsPerAxis>(
    tables.nodesToSubcells.data()
  );
  for (const int threads : {1, 5, 32}) {
    std::vector<double> scratch(Scheme::ProjectionScratchValues);
    std::vector<double> split(Layout::SubcellValuesPerCell);
    for (int stage = 0; stage < exahype2::dg::getStagesOfTensorProduct<DIMENSIONS>(); stage++) {
      for (int thread = 0; thread < threads; thread++) {
        Scheme::projectNodesOntoSubcells(
          nodes.data(),
          split.data(),
          tables.nodesToSubcells.data(),
          weights,
          scratch.data(),
          stage,
          thread,
          threads
        );
      }
    }
    for (int value = 0; value < Layout::SubcellValuesPerCell; value++) {
      VALIDATE_EQUALS(split[value], whole[value], threads, value);
    }
  }
}

/**
 * A team per cell writes the subgrid and the halo halves of projectSubgridOntoFaces() after either
 * scheme of the previous step.
 */
UNIT_TEST("Every team of a device projects the subgrid onto the faces like the host") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const std::vector<double>                                   previousSolution = exahype2::dg::unittests::getRamp(
    static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell,
    1.0,
    0.002
  );
  const std::vector<double> subcells = exahype2::dg::unittests::getRamp(
    static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
    2.0,
    0.003
  );
  const std::vector<double> halo(static_cast<std::size_t>(Setup::NumberOfCells) * HaloValuesPerCell, 0.0);
  std::vector<int>          statuses(
    static_cast<std::size_t>(Setup::NumberOfCells) * exahype2::dg::limiting::NumberOfStatusEntries,
    0
  );
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const auto previousStatus =
      cell % 2 == 0 ? exahype2::dg::limiting::LimiterStatus::Regular : exahype2::dg::limiting::LimiterStatus::Limited;
    statuses
      [static_cast<std::size_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries
       + static_cast<std::size_t>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
        static_cast<int>(previousStatus);
  }

  std::vector<double> hostSubcells = subcells;
  std::vector<double> hostHalo     = halo;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    double* haloHalves[peano4::FacesPerCell];
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      haloHalves[faceNumber] =
        hostHalo.data() + (static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber) * HaloValuesPerFace;
    }
    Scheme::projectSubgridOntoFaces(
      statuses.data() + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
      hostSubcells.data() + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
      previousSolution.data() + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
      haloHalves,
      tables.nodesToSubcells.data(),
      false
    );
  }

  std::vector<double>       deviceSubcells = subcells;
  std::vector<double>       deviceHalo     = halo;
  const std::vector<double> scratch(
    static_cast<std::size_t>(Setup::NumberOfCells) * Scheme::ProjectionScratchValues,
    0.0
  );
  const exahype2::dg::unittests::DeviceBuffer<int>    deviceStatuses(statuses);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcellsBuffer(deviceSubcells);
  const exahype2::dg::unittests::DeviceBuffer<double> devicePreviousSolution(previousSolution);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceHaloBuffer(deviceHalo);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(Setup::NumberOfCells, Scheme::getTeamThreads(), 0),
    ProjectSubgridOfCell{
      deviceStatuses.getData(),
      deviceSubcellsBuffer.getData(),
      devicePreviousSolution.getData(),
      deviceHaloBuffer.getData(),
      deviceNodesToSubcells.getData(),
      deviceScratch.getData()
    }
  );
  deviceSubcellsBuffer.copyBack(deviceSubcells);
  deviceHaloBuffer.copyBack(deviceHalo);

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceSubcells, hostSubcells);
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceHalo, hostHalo);
}

/**
 * Every listed cell on the Galerkin scheme writes the projection of its nodes into its subgrid and
 * every other subgrid keeps its values.
 */
UNIT_TEST("Listed Galerkin cells project their nodes onto their subgrids") {
  constexpr int NumberOfTeams = 2;
  constexpr int UnlistedSlot  = 2;
  constexpr int StatusIndex   = static_cast<int>(exahype2::dg::limiting::StatusEntry::Status);
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  tarch::accelerator::ChunkedMemoryPool<double>           nodes(Layout::ValuesPerCell);
  tarch::accelerator::ChunkedMemoryPool<double>           subcells(Layout::SubcellValuesPerCell);
  tarch::accelerator::ChunkedMemoryPool<int>              statuses(exahype2::dg::limiting::NumberOfStatusEntries);
  nodes.reserve(Setup::NumberOfCells);
  subcells.reserve(Setup::NumberOfCells);
  statuses.reserve(Setup::NumberOfCells);
  std::vector<double> nodeValues;
  std::vector<double> subcellValues;
  std::vector<int>    statusValues;
  nodeValues    = getChunksOnHost(nodes);
  subcellValues = getChunksOnHost(subcells);
  statusValues  = getChunksOnHost(statuses);
  nodeValues    = exahype2::dg::unittests::getRamp(nodeValues.size(), 1.0, 0.002);
  subcellValues = exahype2::dg::unittests::getRamp(subcellValues.size(), -2.0, 0.003);
  std::fill(statusValues.begin(), statusValues.end(), 0);
  const auto       paddedNodes    = static_cast<std::size_t>(nodes.getPaddedChunkSize());
  const auto       paddedSubcells = static_cast<std::size_t>(subcells.getPaddedChunkSize());
  const auto       paddedStatus   = static_cast<std::size_t>(statuses.getPaddedChunkSize());
  std::vector<int> listedSlots;
  for (int slot = 0; slot < Setup::NumberOfCells; slot++) {
    const auto status =
      slot % 2 == 0 ? exahype2::dg::limiting::LimiterStatus::Regular : exahype2::dg::limiting::LimiterStatus::Limited;
    statusValues[static_cast<std::size_t>(slot) * paddedStatus + StatusIndex] = static_cast<int>(status);
    if (slot != UnlistedSlot) {
      listedSlots.push_back(slot);
    }
  }
  setChunksFromHost(nodes, nodeValues);
  setChunksFromHost(subcells, subcellValues);
  setChunksFromHost(statuses, statusValues);

  std::vector<double> expectedSubcells = subcellValues;
  for (const int slot : listedSlots) {
    if (slot % 2 == 0) {
      Scheme::projectNodesOntoSubcells(
        nodeValues.data() + static_cast<std::ptrdiff_t>(slot) * static_cast<std::ptrdiff_t>(paddedNodes),
        expectedSubcells.data() + static_cast<std::ptrdiff_t>(slot) * static_cast<std::ptrdiff_t>(paddedSubcells),
        tables.nodesToSubcells.data()
      );
    }
  }

  const exahype2::dg::unittests::DeviceBuffer<int>    deviceSlots(listedSlots);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
  tarch::accelerator::Buffer<double>              scratch(
    static_cast<std::size_t>(tarch::accelerator::getNumberOfScratchChunks(NumberOfTeams))
      * Scheme::SubcellProjectionScratchValues,
    tarch::MemoryLocation::Device
  );
  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(NumberOfTeams, Scheme::getTeamThreads(), 0),
    exahype2::dg::limiting::ProjectNodesOntoSubgrids<exahype2::dg::limiting::unittests::StubSolver, double, double>{
      tarch::Span<const int>{deviceSlots.getData(), static_cast<int>(listedSlots.size())},
      NumberOfTeams,
      std::as_const(nodes).getView(),
      subcells.getView(),
      std::as_const(statuses).getView(),
      deviceNodesToSubcells.getData(),
      scratch.getData()
    }
  );
  std::vector<double> projectedSubcells;
  projectedSubcells = getChunksOnHost(subcells);

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(projectedSubcells, expectedSubcells);
}

UNIT_TEST("A constant state projects onto constant subcells and back") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const std::vector<double>                               nodes = getConstantNodes(3.0, -1.5);
  std::vector<double>                                     subcells(Layout::SubcellValuesPerCell);
  Scheme::projectNodesOntoSubcells(nodes.data(), subcells.data(), tables.nodesToSubcells.data());
  for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
    VALIDATE_NUMERICAL_EQUALS(subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData], 3.0, subcell);
    VALIDATE_NUMERICAL_EQUALS(subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1], -1.5, subcell);
  }

  std::vector<double> back(Layout::ValuesPerCell);
  Scheme::projectSubcellsOntoNodes(subcells.data(), back.data(), tables.subcellsToNodes.data());
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(back, nodes);
}

UNIT_TEST("The initial condition is sampled at the subcell centres") {
  const tarch::la::Vector<DIMENSIONS, double> centre(2.0);
  const tarch::la::Vector<DIMENSIONS, double> width(0.6);
  std::vector<double>                         subcells(Layout::SubcellValuesPerCell);
  Scheme::sampleInitialCondition(subcells.data(), centre, width, RuntimeParameters{});

  peano4::dForEach(Layout::SubcellsPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& subcell) {
    const int    index    = peano4::dLinearised(subcell, Layout::SubcellsPerAxis);
    const double expected = centre(0) - 0.5 * width(0) + (subcell(0) + 0.5) * width(0) / Layout::SubcellsPerAxis;
    VALIDATE_NUMERICAL_EQUALS(subcells[static_cast<std::size_t>(index) * Layout::NumberOfData], expected, index);
    VALIDATE_NUMERICAL_EQUALS(
      subcells[static_cast<std::size_t>(index) * Layout::NumberOfData + 1],
      width(0) / Layout::SubcellsPerAxis,
      index
    );
  });
}

/**
 * forEachSubcell() visits the subcells in linearised order at their centres and with their size.
 */
UNIT_TEST("Every subcell is visited at its centre") {
  std::vector<double>                         values(Layout::SubcellValuesPerCell, 0.0);
  const tarch::la::Vector<DIMENSIONS, double> cellCentre(0.3);
  const tarch::la::Vector<DIMENSIONS, double> cellSize(0.7);

  Visits visits{};
  Scheme::forEachSubcell(
    cellCentre,
    cellSize,
    values.data(),
    [&](
      const double* const                          value,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      const tarch::la::Vector<DIMENSIONS, double>& h
    ) {
      const int visit = visits.numberOfVisits;
      VALIDATE(visit < Layout::SubcellsPerCell);
      visits.values[visit]  = value;
      visits.centres[visit] = x;
      visits.sizes[visit]   = h;
      visits.numberOfVisits++;
    }
  );

  VALIDATE_EQUALS(visits.numberOfVisits, Layout::SubcellsPerCell);
  const auto expectedSize = exahype2::fv::getVolumeSize(cellSize, Layout::SubcellsPerAxis);
  for (int visit = 0; visit < Layout::SubcellsPerCell; visit++) {
    VALIDATE(visits.values[visit] == values.data() + static_cast<std::ptrdiff_t>(visit) * Layout::NumberOfData);
    const auto subcell        = peano4::dDelinearised(visit, Layout::SubcellsPerAxis);
    const auto expectedCentre = exahype2::fv::getVolumeCentre(cellCentre, cellSize, Layout::SubcellsPerAxis, subcell);
    std::vector<double> measured(visits.centres[visit].getData(), visits.centres[visit].getData() + DIMENSIONS);
    std::vector<double> expected(expectedCentre.getData(), expectedCentre.getData() + DIMENSIONS);
    measured.insert(measured.end(), visits.sizes[visit].getData(), visits.sizes[visit].getData() + DIMENSIONS);
    expected.insert(expected.end(), expectedSize.getData(), expectedSize.getData() + DIMENSIONS);
    VALIDATE_NUMERICAL_BUFFERS_EQUALS(measured, expected);
  }
}

UNIT_TEST("The subcell wave speeds scale with the cell size") {
  const tarch::la::Vector<DIMENSIONS, double> centre(0.0);
  tarch::la::Vector<DIMENSIONS, double>       width;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis) = 0.5 + 0.5 * axis;
  }
  std::vector<double> subcells(Layout::SubcellValuesPerCell);
  for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
    subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = 1.0 + subcell;
    subcells[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = 0.0;
  }
  const auto statistics = Scheme::reduceMaxEigenvalues(subcells.data(), centre, width, 0.0, 0.1, RuntimeParameters{});
  VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, static_cast<double>(Layout::SubcellsPerCell));
  double scaled = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    scaled += Layout::SubcellsPerCell / width(axis);
  }
  VALIDATE(std::abs((statistics.maxScaledEigenvalue) - (scaled)) <= Tolerance, statistics.maxScaledEigenvalue, scaled);
}

/**
 * SubgridSolver has a term only where the solver declares it.
 */
UNIT_TEST("The terms forward only what the solver offers") {
  using Reconstruction = exahype2::dg::limiting::unittests::StubSolver::Reconstruction;
  using SubgridSolver  = Scheme::SubgridSolver<Reconstruction>;
  using RiemannSubgridSolver =
    exahype2::dg::limiting::SubcellScheme<RiemannStubSolver, double>::SubgridSolver<Reconstruction>;

  VALIDATE_EQUALS((exahype2::terms::HasFlux<SubgridSolver, double>), true);
  VALIDATE_EQUALS((exahype2::terms::HasMaxEigenvalue<SubgridSolver, double>), true);
  VALIDATE_EQUALS((exahype2::terms::HasSourceTerm<SubgridSolver, double>), false);
  VALIDATE_EQUALS((exahype2::terms::HasRiemannSolver<SubgridSolver, double>), false);
  VALIDATE_EQUALS((exahype2::terms::HasRiemannSolver<RiemannSubgridSolver, double>), true);
  VALIDATE_EQUALS(SubgridSolver::PatchSize, Layout::SubcellsPerAxis);
  VALIDATE(SubgridSolver::TimeSteppingRule == exahype2::TimeStepping::Rule::Fixed);
}

/**
 * SubgridSolver::solveRiemannProblem() returns the wave speed of the subcell Riemann solver and
 * the other terms call the terms of the solver.
 */
UNIT_TEST("The Riemann solver calls the subcell Riemann solver") {
  using Reconstruction = exahype2::dg::limiting::unittests::StubSolver::Reconstruction;
  using RiemannTerms = exahype2::dg::limiting::SubcellScheme<RiemannStubSolver, double>::SubgridSolver<Reconstruction>;
  const double                                state[Layout::NumberOfData]{3.0, 0.0};
  double                                      leftFlux[1]{};
  double                                      rightFlux[1]{};
  const tarch::la::Vector<DIMENSIONS, double> x(0.5);
  const tarch::la::Vector<DIMENSIONS, double> h(0.1);

  VALIDATE_NUMERICAL_EQUALS(
    RiemannTerms::solveRiemannProblem(
      state,
      state,
      x,
      h,
      0.0,
      0.1,
      1,
      false,
      1,
      leftFlux,
      rightFlux,
      RuntimeParameters{}
    ),
    RiemannStubSolver::SubcellWaveSpeed
  );
  VALIDATE_NUMERICAL_EQUALS(RiemannTerms::maxEigenvalue(state, x, h, 0.0, 0.1, 1, RuntimeParameters{}), 3.0);

  double flux[1]{};
  RiemannTerms::flux(state, x, h, 0.0, 0.1, 1, flux, RuntimeParameters{});
  VALIDATE_NUMERICAL_EQUALS(flux[0], 6.0);
}

UNIT_TEST("A constant subgrid recomputes to the constant and reports its wave speed") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  tarch::la::Vector<DIMENSIONS, double>                   width;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    width(axis) = 0.5 + 0.5 * axis;
  }
  std::vector<double>       subcells = getConstantSubgrid(2.0, 0.5);
  const std::vector<double> halo     = getConstantHalo(2.0, 0.5);
  std::vector<double>       nodes(Layout::ValuesPerCell, 0.0);
  std::vector<double>       evolved(Layout::SubcellValuesPerCell, 0.0);
  const double*             haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }

  const auto statistics = recomputeSubgrid<exahype2::dg::limiting::unittests::StubSolver>(
    nodes.data(),
    subcells.data(),
    evolved.data(),
    haloHalves,
    tables.subcellsToNodes.data(),
    centre,
    width,
    0,
    Layout::TimeStamp,
    Layout::TimeStepSize
  );

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(2.0, 0.5));
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(nodes, getConstantNodes(2.0, 0.5));
  VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 2.0);
  double scaled = 0.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    scaled += 2.0 / width(axis);
  }
  VALIDATE(std::abs((statistics.maxScaledEigenvalue) - (scaled)) <= Tolerance, statistics.maxScaledEigenvalue, scaled);
}

UNIT_TEST("A fixed rule reports no wave speeds") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const tarch::la::Vector<DIMENSIONS, double>             centre(0.5);
  const tarch::la::Vector<DIMENSIONS, double>             width(1.0);
  std::vector<double>                                     subcells = getConstantSubgrid(2.0, 0.5);
  const std::vector<double>                               halo     = getConstantHalo(2.0, 0.5);
  std::vector<double>                                     nodes(Layout::ValuesPerCell, 0.0);
  std::vector<double>                                     evolved(Layout::SubcellValuesPerCell, 0.0);
  const double*                                           haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }

  const auto statistics = recomputeSubgrid<exahype2::dg::limiting::unittests::FixedStepStubSolver>(
    nodes.data(),
    subcells.data(),
    evolved.data(),
    haloHalves,
    tables.subcellsToNodes.data(),
    centre,
    width,
    0,
    Layout::TimeStamp,
    Layout::TimeStepSize
  );

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(nodes, getConstantNodes(2.0, 0.5));
  VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
  VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
}

/**
 * A cell projects its previous solution onto its subgrid only after a Galerkin step and never in
 * the first step.
 */
UNIT_TEST("A Galerkin cell projects its previous solution onto the subgrid and the faces") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const std::vector<double>                               previous = getConstantNodes(3.0, 0.25);
  std::vector<double>                                     subcells = getConstantSubgrid(5.0, 0.125);
  std::vector<double>                                     halo(HaloValuesPerCell, 0.0);
  double*                                                 haloHalves[peano4::FacesPerCell];
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    haloHalves[faceNumber] = halo.data() + static_cast<std::ptrdiff_t>(faceNumber) * HaloValuesPerFace;
  }
  int status[exahype2::dg::limiting::NumberOfStatusEntries] = {
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular),
    0,
    0
  };

  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    false
  );
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(3.0, 0.25));
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    const int unknowns    = countValues(haloHalves[faceNumber], HaloValuesPerFace, 3.0);
    const int auxiliaries = countValues(haloHalves[faceNumber], HaloValuesPerFace, 0.25);
    const int zeros       = countValues(haloHalves[faceNumber], HaloValuesPerFace, 0.0);
    VALIDATE(unknowns > 0, faceNumber, unknowns);
    VALIDATE_EQUALS(auxiliaries, unknowns, faceNumber);
    VALIDATE_EQUALS(zeros, HaloValuesPerFace - 2 * unknowns, faceNumber);
  }

  status[static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Limited);
  subcells = getConstantSubgrid(5.0, 0.125);
  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    false
  );
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(5.0, 0.125));
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
    VALIDATE_EQUALS(countValues(haloHalves[faceNumber], HaloValuesPerFace, 3.0), 0, faceNumber);
    VALIDATE(countValues(haloHalves[faceNumber], HaloValuesPerFace, 5.0) > 0, faceNumber);
  }

  status[static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus)] =
    static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular);
  Scheme::projectSubgridOntoFaces(
    status,
    subcells.data(),
    previous.data(),
    haloHalves,
    tables.nodesToSubcells.data(),
    true
  );
  VALIDATE_NUMERICAL_BUFFERS_EQUALS(subcells, getConstantSubgrid(5.0, 0.125));
}

/**
 * A team per cell projects its recomputed subgrid onto its nodes and reduces the wave speeds as
 * projectRecomputedSubgrid() does on the host.
 */
UNIT_TEST("Every team of a device projects the recomputed subgrid like the host") {
  const exahype2::dg::limiting::unittests::SubcellProjections tables;
  const exahype2::dg::unittests::CellChain                    geometry;
  const std::vector<double>                                   subcells = exahype2::dg::unittests::getRamp(
    static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell,
    1.0,
    0.002
  );
  const std::vector<double> nodes(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell, 0.0);

  std::vector<double>                   hostNodes = nodes;
  exahype2::dg::MaxEigenvalueStatistics hostStatistics;
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
    hostStatistics                        = exahype2::dg::MaxEigenvalueStatistics::combine(
      hostStatistics,
      Scheme::projectRecomputedSubgrid(
        hostNodes.data() + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        subcells.data() + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
        tables.subcellsToNodes.data(),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellCentres.data() + coordinateOffset),
        tarch::la::Vector<DIMENSIONS, double>(geometry.cellSizes.data() + coordinateOffset),
        Layout::TimeStamp,
        Layout::TimeStepSize,
        RuntimeParameters{}
      )
    );
  }

  std::vector<double>       deviceNodes = nodes;
  const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * Scheme::ProjectionScratchValues, 0.0);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesBuffer(deviceNodes);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcells(subcells);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcellsToNodes(tables.subcellsToNodes);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(geometry.cellCentres);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(geometry.cellSizes);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
  exahype2::dg::MaxEigenvalueStatistics           deviceStatistics;
  tarch::accelerator::parallelReduce(
    tarch::accelerator::TeamRange(Setup::NumberOfCells, Scheme::getTeamThreads(), 0),
    ProjectRecomputedSubgridOfCell{
      deviceNodesBuffer.getData(),
      deviceSubcells.getData(),
      deviceSubcellsToNodes.getData(),
      deviceCellCentres.getData(),
      deviceCellSizes.getData(),
      deviceScratch.getData()
    },
    tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{deviceStatistics}
  );
  deviceNodesBuffer.copyBack(deviceNodes);

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceNodes, hostNodes);
  VALIDATE_NUMERICAL_EQUALS(deviceStatistics.maxPhysicalEigenvalue, hostStatistics.maxPhysicalEigenvalue);
  VALIDATE_NUMERICAL_EQUALS(deviceStatistics.maxScaledEigenvalue, hostStatistics.maxScaledEigenvalue);
}

/**
 * A team per cell advances the subgrid of advanceSubgridInTeam() as advanceSubgrid() does on the
 * host, with the applied Galerkin flux on half of the faces.
 */
UNIT_TEST("Every team of a device advances the subgrid like the host") {
  const SubgridSteps        steps;
  const std::vector<double> hostSubcells = steps.advanceOnTheHost();

  std::vector<double>       deviceSubcells = steps.subcells;
  const std::vector<double> scratch(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::SubcellValuesPerCell, 0.0);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceSubcellsBuffer(deviceSubcells);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceScratch(scratch);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceHalo(steps.halo);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceAppliedFluxes(steps.appliedFluxes);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(steps.tables.nodesToSubcells);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(steps.geometry.cellCentres);
  const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(steps.geometry.cellSizes);
  tarch::accelerator::parallelFor(
    tarch::accelerator::TeamRange(Setup::NumberOfCells),
    AdvanceSubgridOfCell{
      deviceSubcellsBuffer.getData(),
      deviceScratch.getData(),
      deviceHalo.getData(),
      deviceAppliedFluxes.getData(),
      deviceNodesToSubcells.getData(),
      deviceCellCentres.getData(),
      deviceCellSizes.getData()
    }
  );
  deviceSubcellsBuffer.copyBack(deviceSubcells);

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(deviceSubcells, hostSubcells);
}

/**
 * advanceSubgridWithThreadsOfTeam() advances the subgrid as advanceSubgrid() does when one host
 * thread runs the team. A build with a GPU backend has no host team and runs the team kernel in
 * testEveryTeamOfADeviceAdvancesTheSubgridLikeTheHost() instead.
 */
UNIT_TEST("The threads of a team advance the subgrid like one host thread") {
#if not defined(PEANO_WITH_GPU_BACKEND)
  const SubgridSteps         steps;
  const std::vector<double>  hostSubcells = steps.advanceOnTheHost();
  std::vector<double>        teamSubcells = steps.subcells;
  std::vector<double>        evolved(Layout::SubcellValuesPerCell, 0.0);
  std::vector<unsigned char> teamScratch(tarch::accelerator::TeamScratchBytes);
  const tarch::accelerator::TeamHandle team{teamScratch.data()};
  for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
    const double* haloHalves[peano4::FacesPerCell];
    const double* appliedHalves[peano4::FacesPerCell];
    getHalvesOfCell(cell, steps.halo.data(), steps.appliedFluxes.data(), haloHalves, appliedHalves);
    const std::ptrdiff_t coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
    Step::advanceSubgridWithThreadsOfTeam(
      team,
      teamSubcells.data() + static_cast<std::ptrdiff_t>(cell) * Layout::SubcellValuesPerCell,
      evolved.data(),
      haloHalves,
      tarch::la::Vector<DIMENSIONS, double>(steps.geometry.cellCentres.data() + coordinateOffset),
      tarch::la::Vector<DIMENSIONS, double>(steps.geometry.cellSizes.data() + coordinateOffset),
      0,
      Layout::TimeStamp,
      Layout::TimeStepSize,
      appliedHalves,
      steps.tables.nodesToSubcells.data(),
      RuntimeParameters{}
    );
  }

  VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamSubcells, hostSubcells);
#endif
}
