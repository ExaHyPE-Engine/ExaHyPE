// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/dg/limiting/Detector.h"

#include "exahype2/dg/Kernels.h"
#include "exahype2/dg/limiting/LimiterStatus.h"
#include "peano4/CellConnectivity.h"
#include "peano4/Constants.h"
#include "peano4/HaloExchange.h"
#include "peano4/Loop.h"
#include "peano4/Spacetree.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
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
#include "unittests/peano4/TestGrids.h"
#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.h"

namespace {
  using Layout       = exahype2::dg::limiting::unittests::StubLayout;
  using Setup        = exahype2::dg::unittests::KernelTestSetup;
  using Projections  = exahype2::dg::limiting::unittests::SubcellProjections;
  using Judge        = exahype2::dg::limiting::Detector<exahype2::dg::limiting::unittests::StubSolver, double>;
  using FixedJudge   = exahype2::dg::limiting::Detector<exahype2::dg::limiting::unittests::FixedStepStubSolver, double>;
  using RelaxedJudge = exahype2::dg::limiting::Detector<exahype2::dg::limiting::unittests::RelaxedStubSolver, double>;
  using Extrema      = std::array<double, Layout::ExtremaPerCell>;

  using RuntimeParameters = exahype2::dg::limiting::unittests::RuntimeParameters;

  TARGET_DECLARE
  constexpr int Regular  = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Regular);
  constexpr int Halo     = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Halo);
  constexpr int Limited  = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Limited);
  constexpr int Troubled = static_cast<int>(exahype2::dg::limiting::LimiterStatus::Troubled);

  constexpr int StatusIndex            = static_cast<int>(exahype2::dg::limiting::StatusEntry::Status);
  constexpr int PreviousStatusIndex    = static_cast<int>(exahype2::dg::limiting::StatusEntry::PreviousStatus);
  constexpr int WasTroubledIndex       = static_cast<int>(exahype2::dg::limiting::StatusEntry::WasTroubled);
  constexpr int PredictorDivergedIndex = static_cast<int>(exahype2::dg::limiting::StatusEntry::PredictorDiverged);

  constexpr double NoRelaxation = 0.0;
  constexpr double NoScaling    = 0.0;

  constexpr int CellsPerTestBatch = 3;
  TARGET_DECLARE_END

  constexpr double PositiveInfinity = std::numeric_limits<double>::infinity();

  /**
   * The sampling table that maps every node onto itself.
   */
  constexpr std::array<double, Layout::NodesPerAxis * Layout::NodesPerAxis> IdentityTable{1.0, 0.0, 0.0, 1.0};

  /**
   * A sampling table whose every sample is a weighted average of the nodes along an axis.
   */
  constexpr std::array<double, Layout::NodesPerAxis * Layout::SubcellsPerAxis>
    AveragingTable{0.25, 0.5, 0.75, 0.75, 0.5, 0.25};

  std::vector<double> getConstantNodes(const double unknown, const double auxiliary) {
    std::vector<double> nodes(Layout::ValuesPerCell);
    for (int node = 0; node < Layout::NodesPerCell; node++) {
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData]     = unknown;
      nodes[static_cast<std::size_t>(node) * Layout::NumberOfData + 1] = auxiliary;
    }
    return nodes;
  }

  std::vector<double> getConstantSubgrid(const double unknown, const double auxiliary) {
    std::vector<double> subgrid(Layout::SubcellValuesPerCell);
    for (int subcell = 0; subcell < Layout::SubcellsPerCell; subcell++) {
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData]     = unknown;
      subgrid[static_cast<std::size_t>(subcell) * Layout::NumberOfData + 1] = auxiliary;
    }
    return subgrid;
  }

  /**
   * Returns the nodes of one cell whose first value rises linearly along the first axis from valueAtLowerFace at
   * the lower face to valueAtUpperFace at the upper face. The auxiliary variable of every node is zero.
   */
  std::vector<double> getLinearNodes(
    const std::vector<double>& quadraturePoints,
    const double               valueAtLowerFace,
    const double               valueAtUpperFace
  ) {
    ASSERTION(quadraturePoints.size() == static_cast<std::size_t>(Layout::NodesPerAxis), quadraturePoints.size());
    std::vector<double> nodes(Layout::ValuesPerCell, 0.0);
    peano4::dForEach(Layout::NodesPerAxis, [&](const tarch::la::Vector<DIMENSIONS, int>& node) {
      const auto index = static_cast<std::size_t>(peano4::dLinearised(node, Layout::NodesPerAxis));
      nodes[index * Layout::NumberOfData] =
        valueAtLowerFace + (valueAtUpperFace - valueAtLowerFace) * quadraturePoints[static_cast<std::size_t>(node(0))];
    });
    return nodes;
  }

  template <typename Value>
  std::array<Value*, peano4::FacesPerCell> getAddressesOf(Value* const halves) {
    std::array<Value*, peano4::FacesPerCell> addresses{};
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
      addresses[static_cast<std::size_t>(faceNumber)] = halves + faceNumber;
    }
    return addresses;
  }

  /**
   * The status of one cell with the face halves that judgeCandidateInTeam() and spreadStatus() read
   * and write.
   */
  struct CellStatus final {
    CellStatus()                                   = default;
    CellStatus(const CellStatus& other)            = delete;
    CellStatus(CellStatus&& other)                 = delete;
    CellStatus& operator=(const CellStatus& other) = delete;
    CellStatus& operator=(CellStatus&& other)      = delete;
    ~CellStatus()                                  = default;

    int    status[exahype2::dg::limiting::NumberOfStatusEntries]{Regular, Regular, 0, 0};
    double candidateExtrema[Layout::ExtremaPerCell]{};
    int    faceHalves[peano4::FacesPerCell]{};
    int    neighbourHalves[peano4::FacesPerCell]{};
    std::array<int*, peano4::FacesPerCell>       ownStatusHalves{getAddressesOf(faceHalves)};
    std::array<const int*, peano4::FacesPerCell> neighbourStatusHalves{getAddressesOf<const int>(neighbourHalves)};
  };

  /**
   * Returns extrema widened by other.
   */
  Extrema getWidenedExtrema(const Extrema& extrema, const Extrema& other) {
    Extrema widened = extrema;
    Judge::widenExtrema(widened.data(), other.data());
    return widened;
  }

  TARGET_DECLARE
  template <typename DetectorType>
  struct JudgeBatchInTeam final {
    using Coordinates = tarch::la::Vector<DIMENSIONS, double>;

    int*          statuses{nullptr};
    int*          faceStatuses{nullptr};
    const double* candidates{nullptr};
    const double* referenceExtrema{nullptr};
    double*       candidateExtrema{nullptr};
    double*       scratch{nullptr};
    int           scratchValuesPerTeam{0};
    const double* nodesToLobatto{nullptr};
    const double* nodesToSubcells{nullptr};
    const double* quadraturePoints{nullptr};
    const double* cellCentres{nullptr};
    const double* cellSizes{nullptr};
    int           numberOfCells{0};
    int           cellsPerBatch{1};
    double        relaxationParameter{0.0};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      const int                              batch,
      const tarch::accelerator::TeamHandle   team,
      exahype2::dg::MaxEigenvalueStatistics& accumulator
    ) const {
      const int     first        = batch * cellsPerBatch;
      const int     cellsOfBatch = numberOfCells - first < cellsPerBatch ? numberOfCells - first : cellsPerBatch;
      double* const chunk        = scratch + static_cast<std::ptrdiff_t>(batch) * scratchValuesPerTeam;
      COMPILE_IF_ON_DEVICE(admitBatchWithThreadsOfTeam(team, first, cellsOfBatch, chunk);)
      exahype2::dg::MaxEigenvalueStatistics statistics;
      for (int cellOfBatch = 0; cellOfBatch < cellsOfBatch; cellOfBatch++) {
        const int cell = first + cellOfBatch;
        int*      ownStatusHalves[peano4::FacesPerCell];
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          ownStatusHalves[faceNumber] =
            faceStatuses + static_cast<std::ptrdiff_t>(cell) * peano4::FacesPerCell + faceNumber;
        }
        const std::ptrdiff_t                        coordinateOffset = static_cast<std::ptrdiff_t>(cell) * DIMENSIONS;
        const tarch::la::Vector<DIMENSIONS, double> cellCentre(cellCentres + coordinateOffset);
        const tarch::la::Vector<DIMENSIONS, double> cellSize(cellSizes + coordinateOffset);
        statistics = exahype2::dg::MaxEigenvalueStatistics::combine(
          statistics,
          DetectorType::judgeCandidateInTeam(
            team,
            statuses + static_cast<std::ptrdiff_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries,
            ownStatusHalves,
            candidates + static_cast<std::ptrdiff_t>(cell) * DetectorType::Scheme::ValuesPerCell,
            cellOfBatch,
            referenceExtrema + static_cast<std::ptrdiff_t>(cell) * Layout::ExtremaPerCell,
            nodesToLobatto,
            nodesToSubcells,
            quadraturePoints,
            relaxationParameter,
            NoScaling,
            cellCentre,
            cellSize,
            Layout::TimeStamp,
            Layout::TimeStepSize,
            candidateExtrema + static_cast<std::ptrdiff_t>(cell) * Layout::ExtremaPerCell,
            chunk,
            RuntimeParameters{}
          )
        );
      }
      COMPILE_IF_ON_HOST(accumulator = exahype2::dg::MaxEigenvalueStatistics::combine(accumulator, statistics);)
      COMPILE_IF_ON_DEVICE(
        tarch::accelerator::reduceOverTeam(
          team,
          statistics,
          tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{accumulator}
        );
      )
    }

  private:
    /**
     * Projects the candidates of the cellsOfBatch cells from first onto their subcells in chunk.
     * Thread i of team then admits cell i of the batch.
     */
    template <typename StoragePrecision>
    INLINE_METHOD GPU_CALLABLE_METHOD void admitBatchWithThreadsOfTeam(
      const tarch::accelerator::TeamHandle team,
      const int                            first,
      const int                            cellsOfBatch,
      StoragePrecision* const              chunk
    ) const {
      for (int cellOfBatch = 0; cellOfBatch < cellsOfBatch; cellOfBatch++) {
        DetectorType::projectCandidateOfBatchInTeam(
          team,
          candidates + static_cast<std::ptrdiff_t>(first + cellOfBatch) * DetectorType::Scheme::ValuesPerCell,
          nodesToSubcells,
          cellOfBatch,
          chunk
        );
      }
      const int cellOfBatch = tarch::accelerator::getThreadOfTeam(team);
      if (cellOfBatch < cellsOfBatch) {
        const int cell = first + cellOfBatch;
        DetectorType::admitCandidateOfBatch(
          team,
          candidates + static_cast<std::ptrdiff_t>(cell) * DetectorType::Scheme::ValuesPerCell,
          cellOfBatch,
          Coordinates(cellCentres + cell * DIMENSIONS),
          Coordinates(cellSizes + cell * DIMENSIONS),
          chunk,
          RuntimeParameters{}
        );
      }
      tarch::accelerator::teamBarrier(team);
    }
  };

  struct ReduceExtremaInTeam final {
    const double* nodes{nullptr};
    double*       extrema{nullptr};
    double*       scratch{nullptr};
    int           scratchValuesPerTeam{0};
    const double* nodesToLobatto{nullptr};
    const double* nodesToSubcells{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(const int cell, const tarch::accelerator::TeamHandle team) const {
      Judge::reduceExtremaInTeam(
        team,
        extrema + static_cast<std::ptrdiff_t>(cell) * Layout::ExtremaPerCell,
        nodes + static_cast<std::ptrdiff_t>(cell) * Layout::ValuesPerCell,
        nodesToLobatto,
        nodesToSubcells,
        scratch + static_cast<std::ptrdiff_t>(cell) * scratchValuesPerTeam,
        RuntimeParameters{}
      );
    }
  };

  struct OpenInitialStatusInTeam final {
    int*          status{nullptr};
    const double* nodes{nullptr};
    const double* sampledExtrema{nullptr};
    const double* nodesToLobatto{nullptr};
    const double* nodesToSubcells{nullptr};
    double        relaxationParameter{0.0};
    double        differencesScaling{0.0};
    double*       scratch{nullptr};

    INLINE_METHOD GPU_CALLABLE_METHOD void operator()(
      [[maybe_unused]] const int           cell,
      const tarch::accelerator::TeamHandle team
    ) const {
      ASSERTION(cell == 0, cell);
      Judge::openInitialStatusInTeam(
        team,
        status,
        nodes,
        sampledExtrema,
        nodesToLobatto,
        nodesToSubcells,
        relaxationParameter,
        differencesScaling,
        scratch,
        RuntimeParameters{}
      );
    }
  };
  TARGET_DECLARE_END

  /**
   * Returns a launch of teams teams with the team size and the team scratch of Judge.
   */
  tarch::accelerator::TeamRange getTeams(const int teams) {
    return tarch::accelerator::TeamRange(teams, Judge::getTeamThreads(), Judge::getTeamScratchBytes());
  }

  /**
   * Returns the number of values in the scratch chunk of one team of a launch of getTeams() that
   * reduces extrema.
   */
  int getExtremaScratchValuesPerTeam() { return Judge::getExtremaScratchValuesPerTeam(Judge::getTeamThreads()); }

  template <typename DetectorType>
  exahype2::dg::MaxEigenvalueStatistics judgeInTeamsOnDevice(
    const Projections&         tables,
    const std::vector<double>& nodesToSubcells,
    const std::vector<double>& cellCentres,
    const std::vector<double>& cellSizes,
    std::vector<int>&          statuses,
    std::vector<int>&          faceStatuses,
    const std::vector<double>& candidates,
    const std::vector<double>& referenceExtrema,
    std::vector<double>&       candidateExtrema,
    const int                  cellsPerBatch,
    const int                  threadsPerTeam,
    const double               relaxationParameter
  ) {
    const auto cells = candidates.size() / Layout::ValuesPerCell;
    ASSERTION(candidates.size() == cells * Layout::ValuesPerCell, candidates.size());
    ASSERTION(statuses.size() == cells * exahype2::dg::limiting::NumberOfStatusEntries, statuses.size(), cells);
    ASSERTION(faceStatuses.size() == cells * peano4::FacesPerCell, faceStatuses.size(), cells);
    ASSERTION(referenceExtrema.size() == cells * Layout::ExtremaPerCell, referenceExtrema.size(), cells);
    ASSERTION(candidateExtrema.size() == cells * Layout::ExtremaPerCell, candidateExtrema.size(), cells);
    ASSERTION(cellCentres.size() == cells * DIMENSIONS, cellCentres.size(), cells);
    ASSERTION(cellSizes.size() == cells * DIMENSIONS, cellSizes.size(), cells);
    ASSERTION(nodesToSubcells.size() == Projections::ProjectionEntries, nodesToSubcells.size());
    ASSERTION(cellsPerBatch > 0, cellsPerBatch);
    ASSERTION(threadsPerTeam > 0, threadsPerTeam);
    const int numberOfCells = static_cast<int>(cells);
    const int batches       = (numberOfCells + cellsPerBatch - 1) / cellsPerBatch;
    const int scratchValuesPerTeam =
      DetectorType::getScratchValuesPerTeam(tarch::accelerator::getTeamThreads(threadsPerTeam));
    tarch::accelerator::Buffer<double> deviceScratch(
      static_cast<std::size_t>(batches) * scratchValuesPerTeam,
      tarch::MemoryLocation::Device
    );
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceStatuses(statuses);
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceFaceStatuses(faceStatuses);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCandidates(candidates);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceReferenceExtrema(referenceExtrema);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCandidateExtrema(candidateExtrema);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToLobatto(tables.nodesToLobatto);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(nodesToSubcells);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceQuadraturePoints(tables.quadraturePoints);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellCentres(cellCentres);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceCellSizes(cellSizes);

    exahype2::dg::MaxEigenvalueStatistics statistics;
    tarch::accelerator::parallelReduce(
      tarch::accelerator::TeamRange(batches, threadsPerTeam, DetectorType::getTeamScratchBytes()),
      JudgeBatchInTeam<DetectorType>{
        deviceStatuses.getData(),
        deviceFaceStatuses.getData(),
        deviceCandidates.getData(),
        deviceReferenceExtrema.getData(),
        deviceCandidateExtrema.getData(),
        deviceScratch.getData(),
        scratchValuesPerTeam,
        deviceNodesToLobatto.getData(),
        deviceNodesToSubcells.getData(),
        deviceQuadraturePoints.getData(),
        deviceCellCentres.getData(),
        deviceCellSizes.getData(),
        numberOfCells,
        cellsPerBatch,
        relaxationParameter
      },
      tarch::accelerator::Custom<exahype2::dg::MaxEigenvalueStatistics>{statistics}
    );

    deviceStatuses.copyBack(statuses);
    deviceFaceStatuses.copyBack(faceStatuses);
    deviceCandidateExtrema.copyBack(candidateExtrema);
    return statistics;
  }

  template <typename DetectorType>
  exahype2::dg::MaxEigenvalueStatistics judgeCellOnDevice(
    const Projections&                           tables,
    const std::vector<double>&                   nodesToSubcells,
    const std::vector<double>&                   candidate,
    const double* const                          referenceExtrema,
    const tarch::la::Vector<DIMENSIONS, double>& centre,
    const tarch::la::Vector<DIMENSIONS, double>& width,
    const double                                 relaxationParameter,
    CellStatus&                                  cell
  ) {
    ASSERTION(referenceExtrema != nullptr);
    std::vector<int>    statuses(std::begin(cell.status), std::end(cell.status));
    std::vector<int>    faceStatuses(std::begin(cell.faceHalves), std::end(cell.faceHalves));
    std::vector<double> candidateExtrema(std::begin(cell.candidateExtrema), std::end(cell.candidateExtrema));
    std::vector<double> cellCentres(DIMENSIONS);
    std::vector<double> cellSizes(DIMENSIONS);
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      cellCentres[static_cast<std::size_t>(axis)] = centre(axis);
      cellSizes[static_cast<std::size_t>(axis)]   = width(axis);
    }
    const auto statistics = judgeInTeamsOnDevice<DetectorType>(
      tables,
      nodesToSubcells,
      cellCentres,
      cellSizes,
      statuses,
      faceStatuses,
      candidate,
      std::vector<double>(referenceExtrema, referenceExtrema + Layout::ExtremaPerCell),
      candidateExtrema,
      1,
      DetectorType::getTeamThreads(),
      relaxationParameter
    );
    std::copy(statuses.begin(), statuses.end(), std::begin(cell.status));
    std::copy(faceStatuses.begin(), faceStatuses.end(), std::begin(cell.faceHalves));
    std::copy(candidateExtrema.begin(), candidateExtrema.end(), std::begin(cell.candidateExtrema));
    return statistics;
  }

  std::vector<double> reduceExtremaOnDevice(
    const std::vector<double>& nodes,
    const double* const        nodesToLobatto,
    const double* const        nodesToSubcells,
    const int                  threadsPerTeam
  ) {
    ASSERTION(nodes.size() % Layout::ValuesPerCell == 0, nodes.size());
    ASSERTION(nodesToLobatto != nullptr);
    ASSERTION(nodesToSubcells != nullptr);
    ASSERTION(threadsPerTeam > 0, threadsPerTeam);
    const auto cells = nodes.size() / Layout::ValuesPerCell;
    const int  scratchValuesPerTeam =
      Judge::getExtremaScratchValuesPerTeam(tarch::accelerator::getTeamThreads(threadsPerTeam));
    const std::vector<double>          lobattoTable(nodesToLobatto, nodesToLobatto + Projections::LobattoEntries);
    const std::vector<double>          subcellTable(nodesToSubcells, nodesToSubcells + Projections::ProjectionEntries);
    std::vector<double>                extrema(cells * Layout::ExtremaPerCell, 0.0);
    tarch::accelerator::Buffer<double> deviceScratch(
      cells * static_cast<std::size_t>(scratchValuesPerTeam),
      tarch::MemoryLocation::Device
    );
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodes(nodes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceExtrema(extrema);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToLobatto(lobattoTable);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(subcellTable);
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(static_cast<int>(cells), threadsPerTeam, 0),
      ReduceExtremaInTeam{
        deviceNodes.getData(),
        deviceExtrema.getData(),
        deviceScratch.getData(),
        scratchValuesPerTeam,
        deviceNodesToLobatto.getData(),
        deviceNodesToSubcells.getData()
      }
    );
    deviceExtrema.copyBack(extrema);
    return extrema;
  }

  std::vector<int> openInitialStatusOnDevice(
    const Projections&         tables,
    const std::vector<int>&    status,
    const std::vector<double>& nodes,
    const Extrema&             sampledExtrema,
    const double               relaxationParameter,
    const double               differencesScaling,
    const int                  threadsPerTeam
  ) {
    ASSERTION(status.size() == static_cast<std::size_t>(exahype2::dg::limiting::NumberOfStatusEntries), status.size());
    ASSERTION(nodes.size() == static_cast<std::size_t>(Layout::ValuesPerCell), nodes.size());
    ASSERTION(threadsPerTeam > 0, threadsPerTeam);
    const std::vector<double> extrema(sampledExtrema.begin(), sampledExtrema.end());
    const int                 scratchValuesPerTeam =
      Judge::getExtremaScratchValuesPerTeam(tarch::accelerator::getTeamThreads(threadsPerTeam));
    tarch::accelerator::Buffer<double> deviceScratch(
      static_cast<std::size_t>(scratchValuesPerTeam),
      tarch::MemoryLocation::Device
    );
    const exahype2::dg::unittests::DeviceBuffer<int>    deviceStatus(status);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodes(nodes);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceSampledExtrema(extrema);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToLobatto(tables.nodesToLobatto);
    const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
    tarch::accelerator::parallelFor(
      tarch::accelerator::TeamRange(1, threadsPerTeam, 0),
      OpenInitialStatusInTeam{
        deviceStatus.getData(),
        deviceNodes.getData(),
        deviceSampledExtrema.getData(),
        deviceNodesToLobatto.getData(),
        deviceNodesToSubcells.getData(),
        relaxationParameter,
        differencesScaling,
        deviceScratch.getData()
      }
    );
    std::vector<int> opened(status.size(), 0);
    deviceStatus.copyBack(opened);
    return opened;
  }

  /**
   * Opens the initial status of a cell with nodes over subcells and returns whether the cell marks itself troubled.
   */
  bool isMarkedTroubledWhenOpened(
    const Projections&         tables,
    const std::vector<double>& nodes,
    const std::vector<double>& subcells,
    const double               relaxationParameter,
    const double               differencesScaling
  ) {
    ASSERTION(nodes.size() == static_cast<std::size_t>(Layout::ValuesPerCell), nodes.size());
    ASSERTION(subcells.size() == static_cast<std::size_t>(Layout::SubcellValuesPerCell), subcells.size());
    Extrema sampledExtrema{};
    Judge::reduceSampledExtrema(sampledExtrema.data(), subcells.data(), RuntimeParameters{});
    const std::vector<int> status = openInitialStatusOnDevice(
      tables,
      std::vector<int>{Regular, Regular, 0, 0},
      nodes,
      sampledExtrema,
      relaxationParameter,
      differencesScaling,
      Judge::getTeamThreads()
    );
    return status[WasTroubledIndex] != 0;
  }

  /**
   * Judges the candidate of a regular cell of unit size at the centre one half with Detector and returns the status of
   * the cell.
   */
  template <typename Detector>
  int getStatusOfJudgedCandidate(
    const Projections&         tables,
    const double* const        nodesToSubcells,
    const std::vector<double>& candidate,
    const Extrema&             referenceExtrema,
    const double               relaxationParameter
  ) {
    ASSERTION(nodesToSubcells != nullptr);
    ASSERTION(candidate.size() == static_cast<std::size_t>(Layout::ValuesPerCell), candidate.size());
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    CellStatus                                  cell;
    static_cast<void>(judgeCellOnDevice<Detector>(
      tables,
      std::vector<double>(nodesToSubcells, nodesToSubcells + Projections::ProjectionEntries),
      candidate,
      referenceExtrema.data(),
      centre,
      width,
      relaxationParameter,
      cell
    ));
    return cell.status[StatusIndex];
  }

  std::vector<int> getStatusesOfChain() {
    std::vector<int> statuses(
      static_cast<std::size_t>(Setup::NumberOfCells) * exahype2::dg::limiting::NumberOfStatusEntries,
      0
    );
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      const auto statusOffset = static_cast<std::size_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries;
      statuses[statusOffset + WasTroubledIndex]       = cell % 3 == 0 ? 1 : 0;
      statuses[statusOffset + PredictorDivergedIndex] = cell == 4 ? 1 : 0;
    }
    return statuses;
  }

  std::vector<double> getReferenceExtremaOfChain() {
    std::vector<double> referenceExtrema(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ExtremaPerCell, 0.0);
    for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
      referenceExtrema[static_cast<std::size_t>(cell) * Layout::ExtremaPerCell + 1] = cell % 2 == 0 ? 100.0 : 0.5;
    }
    return referenceExtrema;
  }

  /**
   * JudgeTest stores the candidates, the marks and the reference extrema of a chain of cells.
   */
  struct JudgeTest final {
    const exahype2::dg::limiting::unittests::SubcellProjections tables{};
    const exahype2::dg::unittests::CellChain                    geometry{};
    const std::vector<double>                                   candidates = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell,
      1.0,
      0.01
    );
    const std::vector<int>    statuses         = getStatusesOfChain();
    const std::vector<double> referenceExtrema = getReferenceExtremaOfChain();
    const std::vector<int>    faceStatuses =
      std::vector<int>(static_cast<std::size_t>(Setup::NumberOfCells) * peano4::FacesPerCell, 0);
  };
} // namespace

UNIT_TEST("Widening extrema commutes, returns one canonical NaN and ranks negative zero below positive zero") {
  GIVEN("Empty extrema and extrema of four, minus two, both zeros and a quiet NaN with a noncanonical payload") {
    constexpr double        FiniteValue         = 4.0;
    constexpr double        SmallerFiniteValue  = -2.0;
    constexpr std::uint64_t NoncanonicalNaNBits = 0x7FF8000000000042ULL;
    constexpr auto          NoncanonicalNaN     = std::bit_cast<double>(NoncanonicalNaNBits);
    constexpr auto          CanonicalNaNBits = std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN());
    const Extrema           finiteExtrema{FiniteValue, FiniteValue};
    const Extrema           smallerExtrema{SmallerFiniteValue, SmallerFiniteValue};
    const Extrema           positiveZeroExtrema{0.0, 0.0};
    const Extrema           negativeZeroExtrema{-0.0, -0.0};
    const Extrema           noncanonicalNaNExtrema{NoncanonicalNaN, NoncanonicalNaN};
    const Extrema           emptyExtrema{PositiveInfinity, -PositiveInfinity};

    WHEN("Extrema of the finite values and the empty extrema widen by each other") {
      const Extrema downwards = getWidenedExtrema(finiteExtrema, smallerExtrema);
      const Extrema upwards   = getWidenedExtrema(smallerExtrema, finiteExtrema);
      const Extrema fromEmpty = getWidenedExtrema(emptyExtrema, finiteExtrema);
      const Extrema byEmpty   = getWidenedExtrema(finiteExtrema, emptyExtrema);
      const Extrema equal     = getWidenedExtrema(finiteExtrema, finiteExtrema);
      const Extrema empty     = getWidenedExtrema(emptyExtrema, emptyExtrema);

      THEN("Either order gives the smaller minimum and the larger maximum and empty extrema stay infinite") {
        VALIDATE_NUMERICAL_EQUALS(downwards[0], SmallerFiniteValue);
        VALIDATE_NUMERICAL_EQUALS(upwards[0], SmallerFiniteValue);
        VALIDATE_NUMERICAL_EQUALS(downwards[1], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(upwards[1], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(fromEmpty[0], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(byEmpty[0], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(fromEmpty[1], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(byEmpty[1], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(equal[0], FiniteValue);
        VALIDATE_NUMERICAL_EQUALS(equal[1], FiniteValue);
        VALIDATE(std::isinf(empty[0]));
        VALIDATE(not std::signbit(empty[0]));
        VALIDATE(std::isinf(empty[1]));
        VALIDATE(std::signbit(empty[1]));
      }
    }

    WHEN("The NaN extrema widen by finite extrema, the finite extrema by the NaN extrema and the result once more") {
      const Extrema fromNaN      = getWidenedExtrema(noncanonicalNaNExtrema, finiteExtrema);
      const Extrema byNaN        = getWidenedExtrema(finiteExtrema, noncanonicalNaNExtrema);
      const Extrema fromNaNAgain = getWidenedExtrema(fromNaN, {SmallerFiniteValue, FiniteValue});

      THEN("Every merge returns the canonical quiet NaN") {
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(fromNaN[0]), CanonicalNaNBits);
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(byNaN[0]), CanonicalNaNBits);
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(fromNaN[1]), CanonicalNaNBits);
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(byNaN[1]), CanonicalNaNBits);
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(fromNaNAgain[0]), CanonicalNaNBits);
        VALIDATE_EQUALS(std::bit_cast<std::uint64_t>(fromNaNAgain[1]), CanonicalNaNBits);
      }
    }

    WHEN("Extrema of the zeros widen by extrema of the zeros") {
      const Extrema positiveByNegative = getWidenedExtrema(positiveZeroExtrema, negativeZeroExtrema);
      const Extrema negativeByPositive = getWidenedExtrema(negativeZeroExtrema, positiveZeroExtrema);
      const Extrema positiveByPositive = getWidenedExtrema(positiveZeroExtrema, positiveZeroExtrema);
      const Extrema negativeByNegative = getWidenedExtrema(negativeZeroExtrema, negativeZeroExtrema);

      THEN(
        "Zeros of either sign give negative zero as the minimum and positive zero as the maximum and equal zeros "
        "keep their sign"
      ) {
        VALIDATE(std::signbit(positiveByNegative[0]));
        VALIDATE(std::signbit(negativeByPositive[0]));
        VALIDATE(not std::signbit(positiveByNegative[1]));
        VALIDATE(not std::signbit(negativeByPositive[1]));
        VALIDATE(not std::signbit(positiveByPositive[0]));
        VALIDATE(std::signbit(negativeByNegative[1]));
      }
    }
  }
}

UNIT_TEST("The maximum principle accepts only an interpolant inside the relaxed range of its subcells") {
  GIVEN("Subcells between zero and one and interpolants along the first axis that stay inside or leave that range") {
    const Projections   tables;
    std::vector<double> subcells = getConstantSubgrid(1.0, 0.0);
    subcells[0]                  = 0.0;

    const std::vector<double> inside               = getLinearNodes(tables.quadraturePoints, 0.4, 0.6);
    const std::vector<double> oneTenthBelow        = getLinearNodes(tables.quadraturePoints, -0.1, 0.6);
    const std::vector<double> oneTenthAbove        = getLinearNodes(tables.quadraturePoints, 0.4, 1.1);
    const std::vector<double> oneTwentiethBelow    = getLinearNodes(tables.quadraturePoints, -0.05, 0.6);
    const std::vector<double> threeTwentiethsBelow = getLinearNodes(tables.quadraturePoints, -0.15, 0.6);
    const std::vector<double> oneQuarterBelow      = getLinearNodes(tables.quadraturePoints, -0.25, 0.6);

    WHEN("The cells open their statuses without relaxation and scaling") {
      const bool insideIsMarked = isMarkedTroubledWhenOpened(tables, inside, subcells, NoRelaxation, NoScaling);
      const bool belowIsMarked  = isMarkedTroubledWhenOpened(tables, oneTenthBelow, subcells, NoRelaxation, NoScaling);
      const bool aboveIsMarked  = isMarkedTroubledWhenOpened(tables, oneTenthAbove, subcells, NoRelaxation, NoScaling);

      THEN("Only the interpolant inside the range leaves its cell unmarked") {
        VALIDATE(not insideIsMarked);
        VALIDATE(belowIsMarked);
        VALIDATE(aboveIsMarked);
      }
    }

#if not defined(PEANO_WITH_FPE)
    WHEN("The cell whose interpolant contains a NaN opens its status without relaxation and scaling") {
      std::vector<double> withNaN = inside;
      withNaN[0]                  = std::numeric_limits<double>::quiet_NaN();
      const bool withNaNIsMarked  = isMarkedTroubledWhenOpened(tables, withNaN, subcells, NoRelaxation, NoScaling);

      THEN("The NaN marks its cell") { VALIDATE(withNaNIsMarked); }
    }
#endif

    WHEN("The cells open their statuses with a relaxation, a scaling of the range of the subcells or both") {
      const bool relaxedIsMarked = isMarkedTroubledWhenOpened(tables, oneTwentiethBelow, subcells, 0.1, NoScaling);
      const bool scaledIsMarked  = isMarkedTroubledWhenOpened(tables, oneTwentiethBelow, subcells, NoRelaxation, 0.1);
      const bool widerIsMarked   = isMarkedTroubledWhenOpened(tables, threeTwentiethsBelow, subcells, 0.2, 0.1);
      const bool beyondIsMarked  = isMarkedTroubledWhenOpened(tables, oneQuarterBelow, subcells, 0.2, 0.1);

      THEN("The larger of the relaxation and the scaled range widens the range of the subcells") {
        VALIDATE(not relaxedIsMarked);
        VALIDATE(not scaledIsMarked);
        VALIDATE(not widerIsMarked);
        VALIDATE(beyondIsMarked);
      }
    }
  }

  GIVEN("A candidate from 0.4 to 0.6 along the first axis and empty reference extrema") {
    const Projections         tables;
    const std::vector<double> candidate = getLinearNodes(tables.quadraturePoints, 0.4, 0.6);
    const Extrema             emptyReferenceExtrema{PositiveInfinity, -PositiveInfinity};

    WHEN("The detector judges the candidate") {
      const int status = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        candidate,
        emptyReferenceExtrema,
        NoRelaxation
      );

      THEN("The cell is troubled") { VALIDATE_EQUALS(status, Troubled); }
    }
  }
}

UNIT_TEST("The maximum principle ignores the last bit of an extremum and tells apart a relative difference of 1e-11") {
  GIVEN("The value 0.6427813001938251, the next larger floating-point number and the value times one plus 1e-11") {
    const Projections tables;
    constexpr double  Value                 = 0.6427813001938251;
    const double      nextLargerValue       = std::nextafter(Value, 1.0);
    const double      relativelyLargerValue = Value * (1.0 + 1.0e-11);

    WHEN("Constant nodes of a larger value open their statuses over constant subcells of the value, once negated") {
      const bool nextLargerIsMarked = isMarkedTroubledWhenOpened(
        tables,
        getConstantNodes(nextLargerValue, 0.0),
        getConstantSubgrid(Value, 0.0),
        NoRelaxation,
        NoScaling
      );
      const bool negatedIsMarked = isMarkedTroubledWhenOpened(
        tables,
        getConstantNodes(-nextLargerValue, 0.0),
        getConstantSubgrid(-Value, 0.0),
        NoRelaxation,
        NoScaling
      );
      const bool relativelyLargerIsMarked = isMarkedTroubledWhenOpened(
        tables,
        getConstantNodes(relativelyLargerValue, 0.0),
        getConstantSubgrid(Value, 0.0),
        NoRelaxation,
        NoScaling
      );

      THEN("Only the nodes of the value times one plus 1e-11 mark their cell") {
        VALIDATE(not nextLargerIsMarked);
        VALIDATE(not negatedIsMarked);
        VALIDATE(relativelyLargerIsMarked);
      }
    }
  }

  GIVEN("Constant nodes and constant subcells of zero") {
    const Projections         tables;
    const std::vector<double> nodes    = getConstantNodes(0.0, 0.0);
    const std::vector<double> subcells = getConstantSubgrid(0.0, 0.0);

    WHEN("The cell opens its status") {
      const bool isMarked = isMarkedTroubledWhenOpened(tables, nodes, subcells, NoRelaxation, NoScaling);

      THEN("The detector leaves the cell unmarked") { VALIDATE(not isMarked); }
    }
  }
}

UNIT_TEST("The extrema of a cell take the observable of every node and ignore the auxiliary variable") {
  GIVEN("Nodes with the observables one, three, zero and further ones and auxiliary variables up to five") {
    std::vector<double> nodes           = getConstantNodes(1.0, 2.0);
    nodes[Layout::NumberOfData]         = 3.0;
    nodes[Layout::NumberOfData + 1]     = 4.0;
    nodes[2 * Layout::NumberOfData]     = 0.0;
    nodes[2 * Layout::NumberOfData + 1] = 5.0;

    WHEN("A team reduces the extrema of the nodes with the identity and the averaging table") {
      const std::vector<double> extrema =
        reduceExtremaOnDevice(nodes, IdentityTable.data(), AveragingTable.data(), Judge::getTeamThreads());

      THEN("The minimum is zero and the maximum three") {
        VALIDATE_NUMERICAL_EQUALS(extrema[0], 0.0);
        VALIDATE_NUMERICAL_EQUALS(extrema[1], 3.0);
      }
    }
  }
}

UNIT_TEST("The samples of the identity table reproduce the extrema of the nodes and averages keep a constant") {
  GIVEN("Nodes whose observables rise from minus two by one per node and constant nodes of seven") {
    std::vector<double> ramp(Layout::ValuesPerCell, 0.0);
    for (int node = 0; node < Layout::NodesPerCell; node++) {
      ramp[static_cast<std::size_t>(node) * Layout::NumberOfData] = -2.0 + node;
    }
    constexpr double          Constant      = 7.0;
    const std::vector<double> constantNodes = getConstantNodes(Constant, 0.0);

    WHEN("A team reduces the extrema of both with the identity and the averaging table") {
      const std::vector<double> rampExtrema =
        reduceExtremaOnDevice(ramp, IdentityTable.data(), AveragingTable.data(), Judge::getTeamThreads());
      const std::vector<double> constantExtrema =
        reduceExtremaOnDevice(constantNodes, IdentityTable.data(), AveragingTable.data(), Judge::getTeamThreads());

      THEN("The ramp spans minus two to its last node and the constant nodes give the constant twice") {
        VALIDATE_NUMERICAL_EQUALS(rampExtrema[0], -2.0);
        VALIDATE_NUMERICAL_EQUALS(rampExtrema[1], -2.0 + Layout::NodesPerCell - 1);
        VALIDATE_NUMERICAL_EQUALS(constantExtrema[0], Constant);
        VALIDATE_NUMERICAL_EQUALS(constantExtrema[1], Constant);
      }
    }
  }
}

UNIT_TEST("The extrema of a profile linear along the first axis lie at the Lobatto samples outside the nodes") {
  GIVEN("The projection tables and nodes that rise linearly from one at the lower face to three at the upper face") {
    const Projections         tables;
    const std::vector<double> nodes = getLinearNodes(tables.quadraturePoints, 1.0, 3.0);

    WHEN("A team reduces the extrema of the nodes") {
      const std::vector<double> extrema = reduceExtremaOnDevice(
        nodes,
        tables.nodesToLobatto.data(),
        tables.nodesToSubcells.data(),
        Judge::getTeamThreads()
      );

      THEN("The minimum is one and the maximum three") {
        VALIDATE_NUMERICAL_EQUALS(extrema[0], 1.0);
        VALIDATE_NUMERICAL_EQUALS(extrema[1], 3.0);
      }
    }
  }
}

UNIT_TEST("The relaxation term of the solver replaces the specified relaxation of the maximum principle") {
  GIVEN("The reference extrema one and ten, the specified relaxation one quarter and the excess 1e-10") {
    const Projections tables;
    constexpr double  ReferenceMinimum    = 1.0;
    constexpr double  SpecifiedRelaxation = 0.25;
    constexpr double  Excess              = 1.0e-10;
    const Extrema     referenceExtrema{ReferenceMinimum, 10.0};

    WHEN("The detectors without and with the relaxation term judge positive candidates that rise to nine") {
      const int statusAtTheRelaxation = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        getLinearNodes(tables.quadraturePoints, ReferenceMinimum - SpecifiedRelaxation, 9.0),
        referenceExtrema,
        SpecifiedRelaxation
      );
      const int statusBeyondTheRelaxation = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        getLinearNodes(tables.quadraturePoints, ReferenceMinimum - SpecifiedRelaxation - Excess, 9.0),
        referenceExtrema,
        SpecifiedRelaxation
      );
      const int statusAtTwiceTheRelaxation = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        getLinearNodes(tables.quadraturePoints, ReferenceMinimum - 2.0 * SpecifiedRelaxation, 9.0),
        referenceExtrema,
        SpecifiedRelaxation
      );
      const int relaxedStatusAtTwiceTheRelaxation = getStatusOfJudgedCandidate<RelaxedJudge>(
        tables,
        tables.nodesToSubcells.data(),
        getLinearNodes(tables.quadraturePoints, ReferenceMinimum - 2.0 * SpecifiedRelaxation, 9.0),
        referenceExtrema,
        SpecifiedRelaxation
      );
      const int relaxedStatusBeyondTwiceTheRelaxation = getStatusOfJudgedCandidate<RelaxedJudge>(
        tables,
        tables.nodesToSubcells.data(),
        getLinearNodes(tables.quadraturePoints, ReferenceMinimum - 2.0 * SpecifiedRelaxation - Excess, 9.0),
        referenceExtrema,
        SpecifiedRelaxation
      );

      THEN("Without the term a candidate may start one relaxation below one and with the term two relaxations") {
        VALIDATE_EQUALS(statusAtTheRelaxation, Regular);
        VALIDATE_EQUALS(statusBeyondTheRelaxation, Troubled);
        VALIDATE_EQUALS(statusAtTwiceTheRelaxation, Troubled);
        VALIDATE_EQUALS(relaxedStatusAtTwiceTheRelaxation, Regular);
        VALIDATE_EQUALS(relaxedStatusBeyondTwiceTheRelaxation, Troubled);
      }
    }
  }
}

UNIT_TEST("The detector accepts a candidate inside the reference extrema") {
  GIVEN("The reference extrema zero and ten and a candidate that rises linearly from one to nine") {
    const Projections         tables;
    const Extrema             referenceExtrema{0.0, 10.0};
    const std::vector<double> candidate = getLinearNodes(tables.quadraturePoints, 1.0, 9.0);

    WHEN("The detector judges the candidate") {
      const int status = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        candidate,
        referenceExtrema,
        NoRelaxation
      );

      THEN("The cell stays regular") { VALIDATE_EQUALS(status, Regular); }
    }
  }
}

UNIT_TEST("The detector rejects a candidate below the reference minimum") {
  GIVEN("The reference extrema zero and ten and a candidate that rises linearly from minus one half to eight") {
    const Projections         tables;
    const Extrema             referenceExtrema{0.0, 10.0};
    const std::vector<double> candidate = getLinearNodes(tables.quadraturePoints, -0.5, 8.0);

    WHEN("The detector judges the candidate") {
      const int status = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        candidate,
        referenceExtrema,
        NoRelaxation
      );

      THEN("The cell is troubled") { VALIDATE_EQUALS(status, Troubled); }
    }
  }
}

UNIT_TEST("A candidate is admissible when its nodes and its subcells pass the admissibility term of the solver") {
  GIVEN(
    "The reference extrema minus five and ten, a constant candidate of one, a candidate with positive nodes whose "
    "last subcells turn negative and a candidate of ones whose first node is zero"
  ) {
    const Projections         tables;
    const Extrema             referenceExtrema{-5.0, 10.0};
    const std::vector<double> positive             = getConstantNodes(1.0, 1.0);
    const std::vector<double> negativeLastSubcells = getLinearNodes(tables.quadraturePoints, 8.0, -2.0);
    std::vector<double>       zeroFirstNode        = getConstantNodes(1.0, 1.0);
    zeroFirstNode[0]                               = 0.0;

    WHEN("The detector judges every candidate and the last one with subcells that average its nodes") {
      const int positiveStatus = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        positive,
        referenceExtrema,
        NoRelaxation
      );
      const int negativeLastSubcellsStatus = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        negativeLastSubcells,
        referenceExtrema,
        NoRelaxation
      );
      const int zeroFirstNodeStatus =
        getStatusOfJudgedCandidate<Judge>(tables, AveragingTable.data(), zeroFirstNode, referenceExtrema, NoRelaxation);

      THEN("Only the constant candidate keeps its cell regular") {
        VALIDATE_EQUALS(positiveStatus, Regular);
        VALIDATE_EQUALS(negativeLastSubcellsStatus, Troubled);
        VALIDATE_EQUALS(zeroFirstNodeStatus, Troubled);
      }
    }
  }
}

UNIT_TEST("A candidate whose interpolant turns negative at a face fails the admissibility term of the solver") {
  GIVEN(
    "The reference extrema minus five and ten and two linear candidates with positive nodes, one of them one and "
    "the other minus one half on the lower face"
  ) {
    const Projections         tables;
    const Extrema             referenceExtrema{-5.0, 10.0};
    const std::vector<double> positiveOnTheFaces     = getLinearNodes(tables.quadraturePoints, 1.0, 3.0);
    const std::vector<double> negativeOnTheLowerFace = getLinearNodes(tables.quadraturePoints, -0.5, 3.5);

    WHEN("The detector judges both candidates") {
      const int positiveOnTheFacesStatus = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        positiveOnTheFaces,
        referenceExtrema,
        NoRelaxation
      );
      const int negativeOnTheLowerFaceStatus = getStatusOfJudgedCandidate<Judge>(
        tables,
        tables.nodesToSubcells.data(),
        negativeOnTheLowerFace,
        referenceExtrema,
        NoRelaxation
      );
      bool everyNodeIsPositive = true;
      for (int node = 0; node < Layout::NodesPerCell; node++) {
        everyNodeIsPositive =
          everyNodeIsPositive and negativeOnTheLowerFace[static_cast<std::size_t>(node) * Layout::NumberOfData] > 0.0;
      }

      THEN("Only the candidate that stays positive on the faces keeps its cell regular") {
        VALIDATE(everyNodeIsPositive);
        VALIDATE_EQUALS(positiveOnTheFacesStatus, Regular);
        VALIDATE_EQUALS(negativeOnTheLowerFaceStatus, Troubled);
      }
    }
  }
}

UNIT_TEST("Widening extrema lowers the minimum and raises the maximum to those of the other extrema") {
  GIVEN("The extrema two and five, the inner extrema three and four and the outer extrema one and six") {
    const Extrema first{2.0, 5.0};
    const Extrema inner{3.0, 4.0};
    const Extrema outer{1.0, 6.0};

    WHEN("The function clearExtrema() clears the extrema") {
      Extrema extrema{};
      Judge::clearExtrema(extrema.data());

      THEN("The minimum is plus infinity and the maximum minus infinity") {
        VALIDATE(std::isinf(extrema[0]) and not std::signbit(extrema[0]));
        VALIDATE(std::isinf(extrema[1]) and std::signbit(extrema[1]));
      }

      AND_WHEN("They widen by the extrema two and five") {
        Judge::widenExtrema(extrema.data(), first.data());

        THEN("They become two and five") {
          VALIDATE_NUMERICAL_EQUALS(extrema[0], 2.0);
          VALIDATE_NUMERICAL_EQUALS(extrema[1], 5.0);
        }

        AND_WHEN("They widen by the inner extrema") {
          Judge::widenExtrema(extrema.data(), inner.data());

          THEN("They stay two and five") {
            VALIDATE_NUMERICAL_EQUALS(extrema[0], 2.0);
            VALIDATE_NUMERICAL_EQUALS(extrema[1], 5.0);
          }

          AND_WHEN("They widen by the outer extrema") {
            Judge::widenExtrema(extrema.data(), outer.data());

            THEN("They become one and six") {
              VALIDATE_NUMERICAL_EQUALS(extrema[0], 1.0);
              VALIDATE_NUMERICAL_EQUALS(extrema[1], 6.0);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("Opening the initial status marks a cell whose interpolant leaves the range of its subcells") {
  GIVEN("The status of a troubled cell, the extrema of subcells of two, rising nodes and constant nodes of two") {
    const Projections         tables;
    const std::vector<double> subcells      = getConstantSubgrid(2.0, 0.0);
    const std::vector<double> risingNodes   = getLinearNodes(tables.quadraturePoints, 1.0, 3.0);
    const std::vector<double> constantNodes = getConstantNodes(2.0, 0.0);
    const std::vector<int>    troubledStatus{Troubled, Troubled, 1, 1};
    Extrema                   sampledExtrema{};
    Judge::reduceSampledExtrema(sampledExtrema.data(), subcells.data(), RuntimeParameters{});

    WHEN("The status opens with the nodes that rise from one to three") {
      const std::vector<int> status = openInitialStatusOnDevice(
        tables,
        troubledStatus,
        risingNodes,
        sampledExtrema,
        NoRelaxation,
        NoScaling,
        Judge::getTeamThreads()
      );

      THEN("The cell opens regular and marked as troubled and its predictor counts as converged") {
        VALIDATE_EQUALS(status[StatusIndex], Regular);
        VALIDATE_EQUALS(status[PreviousStatusIndex], Regular);
        VALIDATE_EQUALS(status[WasTroubledIndex], 1);
        VALIDATE_EQUALS(status[PredictorDivergedIndex], 0);
      }
    }

    WHEN("The status opens with the constant nodes") {
      const std::vector<int> status = openInitialStatusOnDevice(
        tables,
        troubledStatus,
        constantNodes,
        sampledExtrema,
        NoRelaxation,
        NoScaling,
        Judge::getTeamThreads()
      );

      THEN("The detector leaves the cell unmarked") { VALIDATE_EQUALS(status[WasTroubledIndex], 0); }
    }
  }
}

UNIT_TEST("Opening a step copies the solution and opens a troubled cell limited and every other cell regular") {
  GIVEN("A constant solution, a zero previous solution and the statuses of a troubled cell and of a halo cell") {
    const std::vector<double> solution = getConstantNodes(4.0, 0.5);
    std::vector<double>       previous(Layout::ValuesPerCell, 0.0);
    int                       troubledStatus[exahype2::dg::limiting::NumberOfStatusEntries]{Troubled, Regular, 1, 0};
    int                       haloStatus[exahype2::dg::limiting::NumberOfStatusEntries]{Halo, Limited, 0, 0};

    WHEN("The troubled cell opens its step") {
      Judge::openStep(troubledStatus, solution.data(), previous.data());

      THEN("The previous solution equals the solution and the cell opens limited and stays marked") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(previous, solution);
        VALIDATE_EQUALS(troubledStatus[StatusIndex], Limited);
        VALIDATE_EQUALS(troubledStatus[PreviousStatusIndex], Troubled);
        VALIDATE_EQUALS(troubledStatus[WasTroubledIndex], 1);
      }
    }

    WHEN("The halo cell opens its step") {
      Judge::openStep(haloStatus, solution.data(), previous.data());

      THEN("The cell opens regular with the previous status halo") {
        VALIDATE_EQUALS(haloStatus[StatusIndex], Regular);
        VALIDATE_EQUALS(haloStatus[PreviousStatusIndex], Halo);
      }
    }
  }
}

UNIT_TEST("An inadmissible candidate turns its cell troubled and writes the status onto every face") {
  GIVEN("A regular cell, a constant candidate of minus one and the reference extrema minus five and five") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   candidate = getConstantNodes(-1.0, 0.0);
    const double                                referenceExtrema[Layout::ExtremaPerCell]{-5.0, 5.0};
    CellStatus                                  cell;

    WHEN("The detector judges the candidate") {
      const auto statistics = judgeCellOnDevice<Judge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      );

      THEN("The detector marks the cell and every face half troubled and reports the wave speed zero") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Troubled);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 1);
        VALIDATE_EQUALS(cell.status[PredictorDivergedIndex], 0);
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          VALIDATE_EQUALS(cell.faceHalves[faceNumber], Troubled, faceNumber);
        }
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST("A cell that was troubled in the previous step stays on the subcell scheme and reports the wave speed zero") {
  GIVEN(
    "A limited cell that was troubled in the previous step and a constant candidate of two inside the reference "
    "extrema one half and three"
  ) {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   candidate = getConstantNodes(2.0, 0.0);
    const double                                referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
    CellStatus                                  cell;
    cell.status[StatusIndex]         = Limited;
    cell.status[PreviousStatusIndex] = Troubled;
    cell.status[WasTroubledIndex]    = 1;

    WHEN("The detector judges the candidate") {
      const auto statistics = judgeCellOnDevice<Judge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      );

      THEN("The detector keeps the cell and its faces limited, unmarks the cell and reports the wave speed zero") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Limited);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 0);
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          VALIDATE_EQUALS(cell.faceHalves[faceNumber], Limited, faceNumber);
        }
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST("An accepted candidate keeps its cell regular and reports its wave speeds") {
  GIVEN(
    "A regular cell whose width grows along the axes and a constant candidate of two inside the reference "
    "extrema one half and three"
  ) {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    tarch::la::Vector<DIMENSIONS, double>       width;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      width(axis) = 0.5 + 0.5 * axis;
    }
    const std::vector<double> candidate = getConstantNodes(2.0, 0.0);
    const double              referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
    CellStatus                cell;
    double                    scaled = 0.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      scaled += 2.0 / width(axis);
    }

    WHEN("The detector judges the candidate") {
      const auto statistics = judgeCellOnDevice<Judge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      );

      THEN("The detector keeps the cell regular and unmarked and reports the wave speed two plain and per width") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Regular);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 0);
        for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
          VALIDATE_EQUALS(cell.faceHalves[faceNumber], Regular, faceNumber);
        }
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 2.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, scaled);
      }
    }
  }
}

UNIT_TEST("A diverged predictor fails its candidate") {
  GIVEN("A regular cell whose predictor diverged and a constant candidate of two inside the reference extrema") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   candidate = getConstantNodes(2.0, 0.0);
    const double                                referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
    CellStatus                                  cell;
    cell.status[PredictorDivergedIndex] = 1;

    WHEN("The detector judges the candidate") {
      static_cast<void>(judgeCellOnDevice<Judge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      ));

      THEN("The detector marks the cell troubled and resets the divergence of the predictor") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Troubled);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 1);
        VALIDATE_EQUALS(cell.status[PredictorDivergedIndex], 0);
      }
    }
  }
}

UNIT_TEST("A constant candidate above the reference maximum turns its cell troubled and marks it") {
  GIVEN("A regular cell, a constant candidate of two and the reference extrema one half and one") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   candidate = getConstantNodes(2.0, 0.0);
    const double                                referenceExtrema[Layout::ExtremaPerCell]{0.5, 1.0};
    CellStatus                                  cell;

    WHEN("The detector judges the candidate") {
      static_cast<void>(judgeCellOnDevice<Judge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      ));

      THEN("The detector marks the cell troubled") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Troubled);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 1);
      }
    }
  }
}

UNIT_TEST("Spreading the status lifts a cell by the statuses of its neighbours and never lowers it") {
  GIVEN("A regular cell whose first neighbour is troubled") {
    CellStatus cell;
    cell.neighbourHalves[0] = Troubled;

    WHEN("The status spreads") {
      Judge::spreadStatus(cell.status, cell.neighbourStatusHalves.data());

      THEN("The cell turns limited and marked") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Limited);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 1);
      }
    }
  }

  GIVEN("A regular cell whose second neighbour is limited") {
    CellStatus cell;
    cell.neighbourHalves[1] = Limited;

    WHEN("The status spreads") {
      Judge::spreadStatus(cell.status, cell.neighbourStatusHalves.data());

      THEN("The cell turns halo and stays unmarked") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Halo);
        VALIDATE_EQUALS(cell.status[WasTroubledIndex], 0);
      }
    }
  }

  GIVEN("A troubled cell with regular neighbours") {
    CellStatus cell;
    cell.status[StatusIndex] = Troubled;

    WHEN("The status spreads") {
      Judge::spreadStatus(cell.status, cell.neighbourStatusHalves.data());

      THEN("The cell stays troubled") { VALIDATE_EQUALS(cell.status[StatusIndex], Troubled); }
    }
  }
}

UNIT_TEST("Every ring of the spreading reads the statuses of the previous ring from the faces") {
  GIVEN("A regular grid with a troubled corner cell, its statuses on the own halves of its faces and an exchange") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells            = cellConnectivity.getCells();

    tarch::accelerator::ChunkedMemoryPool<int> statuses(exahype2::dg::limiting::NumberOfStatusEntries);
    tarch::accelerator::ChunkedMemoryPool<int> faceStatuses(Judge::StatusValuesPerFace);
    statuses.reserve(cellConnectivity.getNumberOfSlots());
    faceStatuses.reserve(cellConnectivity.getNumberOfFaces());
    std::vector<int> statusValues = tarch::accelerator::unittests::getChunksOf(statuses);
    std::vector<int> faceValues   = tarch::accelerator::unittests::getChunksOf(faceStatuses);
    std::fill(statusValues.begin(), statusValues.end(), 0);
    std::fill(faceValues.begin(), faceValues.end(), Regular);
    const auto paddedStatus = static_cast<std::size_t>(statuses.getPaddedChunkSize());
    const auto paddedFace   = static_cast<std::size_t>(faceStatuses.getPaddedChunkSize());
    for (int entry = 0; entry < cells.size; entry++) {
      const int  slot    = cells[entry];
      const auto lattice = cellConnectivity.getCellLattice(slot);
      int        sum     = 0;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        sum += lattice(axis);
      }
      const int status                                                               = sum == 0 ? Troubled : Regular;
      statusValues[static_cast<std::size_t>(slot) * paddedStatus + StatusIndex]      = status;
      statusValues[static_cast<std::size_t>(slot) * paddedStatus + WasTroubledIndex] = sum == 0 ? 1 : 0;
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
        const auto face = static_cast<std::size_t>(cellConnectivity.getCellFace(slot, faceNumber));
        faceValues[face * paddedFace + exahype2::dg::getOwnHalf(faceNumber)] = status;
      }
    }
    tarch::accelerator::unittests::setChunksOf(statuses, statusValues);
    tarch::accelerator::unittests::setChunksOf(faceStatuses, faceValues);
    peano4::HaloExchange<int> exchange(faceStatuses);
    exchange.rebuild(cellConnectivity);

    WHEN("Every ring exchanges the faces, spreads the statuses and writes them onto the faces for the next ring") {
      std::array<std::vector<int>, exahype2::dg::limiting::NumberOfSpreadRings> statusesAfterSpread;
      std::array<bool, exahype2::dg::limiting::NumberOfSpreadRings>             facesUnchanged{};
      for (int ring = 0; ring < exahype2::dg::limiting::NumberOfSpreadRings; ring++) {
        exchange.post();
        exchange.wait();
        faceValues = tarch::accelerator::unittests::getChunksOf(faceStatuses);
        tarch::accelerator::parallelFor(
          tarch::accelerator::Range(cells.size),
          exahype2::dg::limiting::SpreadStatuses<exahype2::dg::limiting::unittests::StubSolver, double>{
            cellConnectivity.onDevice(),
            cellConnectivity.onDevice().getCells(),
            statuses.getView(),
            std::as_const(faceStatuses).getView()
          }
        );
        const std::vector<int> facesAfterSpread = tarch::accelerator::unittests::getChunksOf(faceStatuses);
        facesUnchanged[ring]                    = facesAfterSpread == faceValues;
        statusesAfterSpread[ring]               = tarch::accelerator::unittests::getChunksOf(statuses);
        if (ring + 1 < exahype2::dg::limiting::NumberOfSpreadRings) {
          tarch::accelerator::parallelFor(
            tarch::accelerator::Range(cells.size),
            exahype2::dg::limiting::WriteStatusesOntoFaces{
              cellConnectivity.onDevice(),
              std::as_const(statuses).getView(),
              faceStatuses.getView()
            }
          );
        }
      }

      THEN("Every spreading keeps the faces, the first ring adds Limited cells and the second ring Halo cells") {
        for (int ring = 0; ring < exahype2::dg::limiting::NumberOfSpreadRings; ring++) {
          VALIDATE(facesUnchanged[ring], ring);
          for (int entry = 0; entry < cells.size; entry++) {
            const int  slot     = cells[entry];
            const auto lattice  = cellConnectivity.getCellLattice(slot);
            int        distance = 0;
            for (int axis = 0; axis < DIMENSIONS; axis++) {
              distance += lattice(axis);
            }
            const int expected = distance <= ring + 1 ? std::max(Regular, Troubled - distance) : Regular;
            VALIDATE_EQUALS(
              statusesAfterSpread[ring][static_cast<std::size_t>(slot) * paddedStatus + StatusIndex],
              expected,
              slot,
              ring
            );
            VALIDATE_EQUALS(
              statusesAfterSpread[ring][static_cast<std::size_t>(slot) * paddedStatus + WasTroubledIndex],
              distance <= 1 ? 1 : 0,
              slot,
              ring
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("Clearing the faces writes regular into both status halves and zeros into every value of the subcell halo") {
  GIVEN("Five faces with pools of statuses and extrema and a subcell halo of seven values per face filled with NaN") {
    constexpr int NumberOfFaces     = 5;
    constexpr int HaloValuesPerFace = 7;
    using Clear = exahype2::dg::limiting::ClearFaces<exahype2::dg::limiting::unittests::StubSolver, double, double>;

    tarch::accelerator::ChunkedMemoryPool<int>    faceStatuses(Judge::StatusValuesPerFace);
    tarch::accelerator::ChunkedMemoryPool<double> extremaFaces(Judge::ExtremaValuesPerFace);
    tarch::accelerator::ChunkedMemoryPool<double> subcellHalo(HaloValuesPerFace);
    faceStatuses.reserve(NumberOfFaces);
    extremaFaces.reserve(NumberOfFaces);
    subcellHalo.reserve(NumberOfFaces);
    std::vector<double> haloOfNaN = tarch::accelerator::unittests::getChunksOf(subcellHalo);
    std::fill(haloOfNaN.begin(), haloOfNaN.end(), std::numeric_limits<double>::quiet_NaN());
    tarch::accelerator::unittests::setChunksOf(subcellHalo, haloOfNaN);

    WHEN("The functor ClearFaces clears the faces") {
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(NumberOfFaces),
        Clear{faceStatuses.getView(), extremaFaces.getView(), subcellHalo.getView()}
      );
      const std::vector<int>    statusesOnHost = tarch::accelerator::unittests::getChunksOf(faceStatuses);
      const std::vector<double> haloOnHost     = tarch::accelerator::unittests::getChunksOf(subcellHalo);
      const auto                paddedStatus   = static_cast<std::size_t>(faceStatuses.getPaddedChunkSize());
      const auto                paddedHalo     = static_cast<std::size_t>(subcellHalo.getPaddedChunkSize());

      THEN("Both status halves of every face are regular and every value of the halo is zero") {
        for (int face = 0; face < NumberOfFaces; face++) {
          VALIDATE_EQUALS(statusesOnHost[static_cast<std::size_t>(face) * paddedStatus], Regular, face);
          VALIDATE_EQUALS(statusesOnHost[static_cast<std::size_t>(face) * paddedStatus + 1], Regular, face);
          for (std::size_t value = 0; value < paddedHalo; value++) {
            VALIDATE_EQUALS(haloOnHost[static_cast<std::size_t>(face) * paddedHalo + value], 0.0, face, value);
          }
        }
      }
    }
  }
}

UNIT_TEST("A candidate of a solver with a fixed time step rule reports the wave speed zero") {
  GIVEN("A regular cell of unit size and a constant candidate of two inside the reference extrema one half and three") {
    const Projections                           tables;
    const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
    const tarch::la::Vector<DIMENSIONS, double> width(1.0);
    const std::vector<double>                   candidate = getConstantNodes(2.0, 0.0);
    const double                                referenceExtrema[Layout::ExtremaPerCell]{0.5, 3.0};
    CellStatus                                  cell;

    WHEN("The detector of the solver with a fixed rule judges the candidate") {
      const auto statistics = judgeCellOnDevice<FixedJudge>(
        tables,
        tables.nodesToSubcells,
        candidate,
        referenceExtrema,
        centre,
        width,
        NoRelaxation,
        cell
      );

      THEN("The cell stays regular and the detector reports the wave speed zero") {
        VALIDATE_EQUALS(cell.status[StatusIndex], Regular);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxPhysicalEigenvalue, 0.0);
        VALIDATE_NUMERICAL_EQUALS(statistics.maxScaledEigenvalue, 0.0);
      }
    }
  }
}

UNIT_TEST(
  "The block kernels count and list the cells on the subcell scheme in cell connectivity order, local cells first"
) {
  GIVEN("A regular grid of depth one whose cells take the statuses regular, halo, troubled and limited in turn") {
    constexpr int                       Depth          = 1;
    constexpr int                       NumberOfBlocks = 2;
    static constexpr std::array<int, 4> StatusPattern{Regular, Halo, Troubled, Limited};
    peano4::unittests::buildRegularGrid(Depth);
    const auto& cellConnectivity   = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto  cells              = cellConnectivity.getCells();
    const int   numberOfSlots      = cellConnectivity.getNumberOfSlots();
    const int   numberOfLocalCells = cellConnectivity.getCells(peano4::Adjacency::Local).size;

    tarch::accelerator::ChunkedMemoryPool<int> statuses(exahype2::dg::limiting::NumberOfStatusEntries);
    statuses.reserve(numberOfSlots);
    const auto       paddedChunkSize = static_cast<std::size_t>(statuses.getPaddedChunkSize());
    std::vector<int> chunks          = tarch::accelerator::unittests::getChunksOf(statuses);
    std::vector<int> expectedCellsOnScheme;
    std::vector<int> expectedRecomputedCells;
    exahype2::dg::limiting::SubcellSchemeCounts expected{};
    for (int entry = 0; entry < cells.size; entry++) {
      const int cell   = cells[entry];
      const int status = StatusPattern[static_cast<std::size_t>(entry) % StatusPattern.size()];
      chunks[static_cast<std::size_t>(cell) * paddedChunkSize + static_cast<std::size_t>(StatusIndex)] = status;
      if (status != Regular) {
        expectedCellsOnScheme.push_back(cell);
        expected.cellsOnScheme++;
      }
      if (status == Troubled or status == Limited) {
        expectedRecomputedCells.push_back(cell);
        expected.recomputedCells++;
        expected.localRecomputedCells += entry < numberOfLocalCells ? 1 : 0;
      }
      expected.troubledCells += status == Troubled ? 1 : 0;
    }
    tarch::accelerator::unittests::setChunksOf(statuses, chunks);

    WHEN("Two blocks count and list the cells on the subcell scheme") {
      tarch::accelerator::Buffer<exahype2::dg::limiting::SubcellSchemeCounts> countsOfBlocks(
        NumberOfBlocks,
        tarch::MemoryLocation::Device
      );
      exahype2::dg::limiting::SubcellSchemeCounts counts{};
      tarch::accelerator::parallelReduce(
        tarch::accelerator::Range(NumberOfBlocks),
        exahype2::dg::limiting::CountCellsOnSubcellScheme{
          cellConnectivity.onDevice(),
          std::as_const(statuses).getView(),
          NumberOfBlocks,
          numberOfLocalCells,
          countsOfBlocks.getData()
        },
        tarch::accelerator::Custom<exahype2::dg::limiting::SubcellSchemeCountsReduction>{counts}
      );

      tarch::accelerator::Buffer<int> cellsOnScheme(numberOfSlots, tarch::MemoryLocation::Device);
      tarch::accelerator::Buffer<int> recomputedCells(numberOfSlots, tarch::MemoryLocation::Device);
      tarch::accelerator::parallelFor(
        tarch::accelerator::Range(NumberOfBlocks),
        exahype2::dg::limiting::ListCellsOnSubcellScheme{
          cellConnectivity.onDevice(),
          std::as_const(statuses).getView(),
          NumberOfBlocks,
          std::as_const(countsOfBlocks).getData(),
          cellsOnScheme.getData(),
          recomputedCells.getData()
        }
      );
      const std::vector<int> listedCellsOnScheme   = cellsOnScheme.getValues();
      const std::vector<int> listedRecomputedCells = recomputedCells.getValues();

      THEN("The counts match the statuses and both lists contain the cells in the order of the cell connectivity") {
        VALIDATE_EQUALS(counts.cellsOnScheme, expected.cellsOnScheme);
        VALIDATE_EQUALS(counts.recomputedCells, expected.recomputedCells);
        VALIDATE_EQUALS(counts.localRecomputedCells, expected.localRecomputedCells);
        VALIDATE_EQUALS(counts.troubledCells, expected.troubledCells);
        for (std::size_t entry = 0; entry < expectedCellsOnScheme.size(); entry++) {
          VALIDATE_EQUALS(listedCellsOnScheme[entry], expectedCellsOnScheme[entry], entry);
        }
        for (std::size_t entry = 0; entry < expectedRecomputedCells.size(); entry++) {
          VALIDATE_EQUALS(listedRecomputedCells[entry], expectedRecomputedCells[entry], entry);
        }
      }
    }
  }
}

UNIT_TEST("A team per batch of three cells judges every candidate like a team of one thread per cell") {
  GIVEN("A ramp of candidates on a chain of cells with marked and diverged cells and two reference maxima") {
    const JudgeTest test;

    WHEN("The teams of a device per batch of three cells and teams of one thread per cell judge every candidate") {
      std::vector<int>    teamStatuses     = test.statuses;
      std::vector<int>    teamFaceStatuses = test.faceStatuses;
      std::vector<double> teamExtrema(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ExtremaPerCell, 0.0);
      const auto          teamStatistics = judgeInTeamsOnDevice<Judge>(
        test.tables,
        test.tables.nodesToSubcells,
        test.geometry.cellCentres,
        test.geometry.cellSizes,
        teamStatuses,
        teamFaceStatuses,
        test.candidates,
        test.referenceExtrema,
        teamExtrema,
        CellsPerTestBatch,
        Judge::getTeamThreads(),
        NoRelaxation
      );
      std::vector<int>    serialStatuses     = test.statuses;
      std::vector<int>    serialFaceStatuses = test.faceStatuses;
      std::vector<double> serialExtrema(static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ExtremaPerCell, 0.0);
      const auto          serialStatistics = judgeInTeamsOnDevice<Judge>(
        test.tables,
        test.tables.nodesToSubcells,
        test.geometry.cellCentres,
        test.geometry.cellSizes,
        serialStatuses,
        serialFaceStatuses,
        test.candidates,
        test.referenceExtrema,
        serialExtrema,
        1,
        1,
        NoRelaxation
      );
      int troubledCells = 0;
      for (int cell = 0; cell < Setup::NumberOfCells; cell++) {
        const auto statusOffset = static_cast<std::size_t>(cell) * exahype2::dg::limiting::NumberOfStatusEntries;
        troubledCells += serialStatuses[statusOffset + StatusIndex] == Troubled ? 1 : 0;
      }

      THEN("Both write the same statuses, faces, extrema and wave speeds and half the cells plus one turn troubled") {
        VALIDATE(serialStatistics.maxPhysicalEigenvalue > 0.0, serialStatistics.maxPhysicalEigenvalue);
        VALIDATE_EQUALS(troubledCells, Setup::NumberOfCells / 2 + 1);
        for (std::size_t entry = 0; entry < serialStatuses.size(); entry++) {
          VALIDATE_EQUALS(teamStatuses[entry], serialStatuses[entry], entry);
        }
        for (std::size_t entry = 0; entry < serialFaceStatuses.size(); entry++) {
          VALIDATE_EQUALS(teamFaceStatuses[entry], serialFaceStatuses[entry], entry);
        }
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamExtrema, serialExtrema);
        VALIDATE_NUMERICAL_EQUALS(teamStatistics.maxPhysicalEigenvalue, serialStatistics.maxPhysicalEigenvalue);
        VALIDATE_NUMERICAL_EQUALS(teamStatistics.maxScaledEigenvalue, serialStatistics.maxScaledEigenvalue);
      }
    }
  }
}

UNIT_TEST("A team per cell reduces the extrema of the nodes of its cell like a team of one thread") {
  GIVEN("The projection tables and a ramp of nodes on a chain of cells") {
    const Projections         tables;
    const std::vector<double> nodes = exahype2::dg::unittests::getRamp(
      static_cast<std::size_t>(Setup::NumberOfCells) * Layout::ValuesPerCell,
      -2.0,
      0.013
    );

    WHEN("The teams of a device and teams of one thread reduce the extrema of every cell") {
      const std::vector<double> teamExtrema = reduceExtremaOnDevice(
        nodes,
        tables.nodesToLobatto.data(),
        tables.nodesToSubcells.data(),
        Judge::getTeamThreads()
      );
      const std::vector<double> serialExtrema =
        reduceExtremaOnDevice(nodes, tables.nodesToLobatto.data(), tables.nodesToSubcells.data(), 1);

      THEN("Both write the same extrema within rounding") {
        VALIDATE_NUMERICAL_BUFFERS_EQUALS(teamExtrema, serialExtrema);
      }
    }
  }
}

UNIT_TEST("The teams of a launch open the initial status of every cell like a team of one thread per cell") {
  GIVEN("A regular grid whose cells alternate between rising and constant nodes over subcells of two") {
    constexpr int Depth = 1;
    peano4::unittests::buildRegularGrid(Depth);
    const auto&       cellConnectivity = peano4::Spacetree::getInstance().getCellConnectivity();
    const auto        cells            = cellConnectivity.getCells();
    const Projections tables;
    tarch::accelerator::ChunkedMemoryPool<double> nodes(Layout::ValuesPerCell);
    tarch::accelerator::ChunkedMemoryPool<double> sampledExtrema(Judge::ExtremaPerCell);
    tarch::accelerator::ChunkedMemoryPool<int>    statuses(exahype2::dg::limiting::NumberOfStatusEntries);
    nodes.reserve(cellConnectivity.getNumberOfSlots());
    sampledExtrema.reserve(cellConnectivity.getNumberOfSlots());
    statuses.reserve(cellConnectivity.getNumberOfSlots());
    std::vector<double> nodeValues       = tarch::accelerator::unittests::getChunksOf(nodes);
    std::vector<double> extremaValues    = tarch::accelerator::unittests::getChunksOf(sampledExtrema);
    std::vector<int>    expectedStatuses = tarch::accelerator::unittests::getChunksOf(statuses);
    std::fill(expectedStatuses.begin(), expectedStatuses.end(), 0);
    tarch::accelerator::unittests::setChunksOf(statuses, expectedStatuses);

    const std::vector<double> risingNodes   = getLinearNodes(tables.quadraturePoints, 1.0, 3.0);
    const std::vector<double> constantNodes = getConstantNodes(2.0, 0.0);
    const std::vector<double> subcells      = getConstantSubgrid(2.0, 0.0);
    Extrema                   sampled{};
    Judge::reduceSampledExtrema(sampled.data(), subcells.data(), RuntimeParameters{});
    const auto paddedNodes   = static_cast<std::size_t>(nodes.getPaddedChunkSize());
    const auto paddedExtrema = static_cast<std::size_t>(sampledExtrema.getPaddedChunkSize());
    const auto paddedStatus  = static_cast<std::size_t>(statuses.getPaddedChunkSize());
    for (int entry = 0; entry < cells.size; entry++) {
      const auto                 slot      = static_cast<std::size_t>(cells[entry]);
      const std::vector<double>& cellNodes = entry % 2 == 0 ? risingNodes : constantNodes;
      std::copy(
        cellNodes.begin(),
        cellNodes.end(),
        nodeValues.begin() + static_cast<std::ptrdiff_t>(slot * paddedNodes)
      );
      std::copy(
        sampled.begin(),
        sampled.end(),
        extremaValues.begin() + static_cast<std::ptrdiff_t>(slot * paddedExtrema)
      );
      const std::vector<int> opened = openInitialStatusOnDevice(
        tables,
        std::vector<int>(exahype2::dg::limiting::NumberOfStatusEntries, 0),
        cellNodes,
        sampled,
        NoRelaxation,
        NoScaling,
        1
      );
      std::copy(
        opened.begin(),
        opened.end(),
        expectedStatuses.begin() + static_cast<std::ptrdiff_t>(slot * paddedStatus)
      );
    }
    tarch::accelerator::unittests::setChunksOf(nodes, nodeValues);
    tarch::accelerator::unittests::setChunksOf(sampledExtrema, extremaValues);

    WHEN("The teams of a launch open the initial status of every cell on the device") {
      const int                          numberOfTeams        = Judge::getNumberOfTeams(cells.size);
      const int                          scratchValuesPerTeam = getExtremaScratchValuesPerTeam();
      tarch::accelerator::Buffer<double> deviceScratch(
        static_cast<std::size_t>(tarch::accelerator::getNumberOfScratchChunks(numberOfTeams)) * scratchValuesPerTeam,
        tarch::MemoryLocation::Device
      );
      const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToLobatto(tables.nodesToLobatto);
      const exahype2::dg::unittests::DeviceBuffer<double> deviceNodesToSubcells(tables.nodesToSubcells);
      tarch::accelerator::parallelFor(
        getTeams(numberOfTeams),
        exahype2::dg::limiting::OpenInitialStatuses<exahype2::dg::limiting::unittests::StubSolver, double, double>{
          cellConnectivity.onDevice(),
          numberOfTeams,
          std::as_const(nodes).getView(),
          std::as_const(sampledExtrema).getView(),
          statuses.getView(),
          deviceNodesToLobatto.getData(),
          deviceNodesToSubcells.getData(),
          NoRelaxation,
          NoScaling,
          deviceScratch.getData(),
          scratchValuesPerTeam,
          RuntimeParameters{}
        }
      );
      const std::vector<int> openedStatuses = tarch::accelerator::unittests::getChunksOf(statuses);

      THEN("Every cell stores the status that a team of one thread writes") {
        for (int entry = 0; entry < cells.size; entry++) {
          const auto slot = static_cast<std::size_t>(cells[entry]);
          for (int value = 0; value < exahype2::dg::limiting::NumberOfStatusEntries; value++) {
            const std::size_t index = slot * paddedStatus + static_cast<std::size_t>(value);
            VALIDATE_EQUALS(openedStatuses[index], expectedStatuses[index], entry, value);
          }
        }
      }
    }
  }
}

UNIT_TEST(
  "A launch of the detector covers every cell with at most one team per cell and reserves the scratch it reads"
) {
  GIVEN("The threads of a team of the detector") {
    const int teamThreads = Judge::getTeamThreads();

    THEN("Every cell count gets one to that many teams, a batch fits a team and the scratch fits its device code") {
      for (const int cells : {1, 2, 7, 1000, 1000000}) {
        const int teams = Judge::getNumberOfTeams(cells);
        VALIDATE(teams >= 1 and teams <= cells, cells, teams);
      }
      VALIDATE(Judge::CellsPerBatch >= 1 and Judge::CellsPerBatch <= teamThreads, Judge::CellsPerBatch, teamThreads);
      VALIDATE(
        Judge::getTeamScratchBytes() >= Judge::CellsPerBatch * static_cast<int>(sizeof(int)),
        Judge::getTeamScratchBytes()
      );
#if defined(PEANO_WITH_GPU_BACKEND)
      VALIDATE(
        Judge::getExtremaScratchValuesPerTeam(teamThreads)
          >= Judge::ProjectionScratchValues + (teamThreads + 1) * Layout::ExtremaPerCell,
        teamThreads
      );
      VALIDATE(
        Judge::getScratchValuesPerTeam(teamThreads)
          >= Judge::CellsPerBatch * Layout::SubcellValuesPerCell + Judge::getExtremaScratchValuesPerTeam(teamThreads),
        teamThreads
      );
#else
      VALIDATE_EQUALS(Judge::getExtremaScratchValuesPerTeam(teamThreads), 0, teamThreads);
      VALIDATE_EQUALS(Judge::getScratchValuesPerTeam(teamThreads), 0, teamThreads);
#endif
    }
  }
}

UNIT_TEST("A child copies the status of its parent and a parent takes the entry-wise maximum of its children") {
  GIVEN("A pool with the statuses of the children of one cell that differ per child and per entry") {
    using Transfer = exahype2::dg::limiting::StatusTransfer;
    tarch::accelerator::ChunkedMemoryPool<int> pool(Transfer::WorkItemsPerChunk, tarch::MemoryLocation::Heap);
    pool.reserve(peano4::ChildrenPerCell);
    int childSlots[peano4::ChildrenPerCell];
    for (int child = 0; child < peano4::ChildrenPerCell; child++) {
      for (int entry = 0; entry < Transfer::WorkItemsPerChunk; entry++) {
        pool.getChunk(child)[entry] = (3 * child + 2 * entry) % 5;
      }
      childSlots[child] = child;
    }
    const Transfer transfer{std::as_const(pool).getView()};

    WHEN("The transfer restricts the children onto every entry of their parent") {
      int restricted[Transfer::WorkItemsPerChunk];
      for (int entry = 0; entry < Transfer::WorkItemsPerChunk; entry++) {
        transfer.restrictFromChildren(childSlots, restricted, entry, nullptr);
      }

      THEN("Every entry of the parent equals the largest entry of the children") {
        for (int entry = 0; entry < Transfer::WorkItemsPerChunk; entry++) {
          int largest = 0;
          for (int child = 0; child < peano4::ChildrenPerCell; child++) {
            largest = pool.getChunk(child)[entry] > largest ? pool.getChunk(child)[entry] : largest;
          }
          VALIDATE_EQUALS(restricted[entry], largest, entry);
        }
      }
    }

    WHEN("The transfer copies every slot and interpolates every slot as a parent onto a child") {
      std::vector<int> copied(static_cast<std::size_t>(peano4::ChildrenPerCell) * Transfer::WorkItemsPerChunk);
      std::vector<int> interpolated(copied.size());
      for (int child = 0; child < peano4::ChildrenPerCell; child++) {
        const auto chunkOffset       = static_cast<std::ptrdiff_t>(child) * Transfer::WorkItemsPerChunk;
        int* const copiedChunk       = copied.data() + chunkOffset;
        int* const interpolatedChunk = interpolated.data() + chunkOffset;
        for (int entry = 0; entry < Transfer::WorkItemsPerChunk; entry++) {
          transfer.copy(child, copiedChunk, entry);
          transfer.interpolateFromParent(child, interpolatedChunk, peano4::ChildrenPerCell - 1 - child, entry, nullptr);
        }
      }

      THEN("The copy and the interpolation of every slot store the statuses of that slot") {
        for (int child = 0; child < peano4::ChildrenPerCell; child++) {
          for (int entry = 0; entry < Transfer::WorkItemsPerChunk; entry++) {
            const auto index = static_cast<std::size_t>(child * Transfer::WorkItemsPerChunk + entry);
            VALIDATE_EQUALS(copied[index], pool.getChunk(child)[entry], child, entry);
            VALIDATE_EQUALS(interpolated[index], pool.getChunk(child)[entry], child, entry);
          }
        }
      }
    }
  }
}
