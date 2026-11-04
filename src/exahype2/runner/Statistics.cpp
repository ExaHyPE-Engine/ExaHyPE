// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "exahype2/runner/Console.h"
#include "peano4/Spacetree.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"

namespace {
  constexpr std::uint64_t SignBias = std::uint64_t(1) << 63;

  std::uint64_t toBits(const double value) {
    std::uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  double fromBits(const std::uint64_t value) {
    double result = 0.0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  std::uint64_t toOrderedBits(const std::int64_t value) {
    std::uint64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result ^ SignBias;
  }

  std::int64_t fromOrderedBits(std::uint64_t value) {
    value ^= SignBias;
    std::int64_t result = 0;
    std::memcpy(&result, &value, sizeof(result));
    return result;
  }

  /**
   * Slots of one reduction record, in three contiguous segments of sums, minima and maxima.
   */
  enum SumSlot {
    SumOfCells,
    SumOfRefinedCells,
    SumOfFaces,
    SumOfHangingFaces,
    SumOfRankBoundaryFaces,
    SumOfCellConnectivityHostBytes,
    SumOfCellConnectivityDeviceBytes,
    SumOfTreeBytes,
    SumOfSolverHostBytes,
    SumOfSolverDeviceBytes,
    SumOfSolverCells,
    SumOfCellDoFs,
    SumOfProcessMemory,
    SumOfTotalMemory,
    SumOfAvailableMemory,
    SumSlotCount
  };

  enum MinimumSlot {
    MinimumRankLoad,
    MinimumLevel,
    MinimumCellWidth,
    MinimumStationaryRegrids,
    MinimumRankBoundaryFaces,
    MinimumSlotCount
  };

  enum MaximumSlot { MaximumRankLoad, MaximumLevel, MaximumRankBoundaryFaces, MaximumSlotCount };

  constexpr int NumberOfSums   = SumSlotCount;
  constexpr int NumberOfMinima = MinimumSlotCount;
  constexpr int NumberOfMaxima = MaximumSlotCount;

  struct RowLabel {
    const char* section{nullptr};
    const char* label{nullptr};
  };

  constexpr std::array<RowLabel, exahype2::runner::GridSnapshot::RowCount> RowLabels{{
    {"Grid", "Cells"},
    {"Grid", "Faces"},
    {"Grid", "Levels"},
    {"MPI Decomposition", "Load Balance"},
    {"MPI Decomposition", "Rank Boundary Faces"},
    {"Memory", "CPU Grid"},
    {"Memory", "GPU Grid"},
    {"Memory", "CPU Solver Data"},
    {"Memory", "GPU Solver Data"},
    {"Memory", "Resident"},
    {"Memory", "Free"},
  }};

  std::string formatEntry(const std::string& value, const exahype2::runner::GridSnapshot::Entry& entry) {
    return value + entry.suffix + (entry.detail.empty() ? "" : "  (" + entry.detail + ")");
  }

  /**
   * Renders every non-empty row under its section title. The free memory joins the line of the
   * resident memory if both are non-empty.
   */
  std::vector<std::string> renderRows(const std::array<std::string, exahype2::runner::GridSnapshot::RowCount>& texts) {
    using GridSnapshot = exahype2::runner::GridSnapshot;
    const bool joinFree =
      not texts[GridSnapshot::MemoryResident].empty() and not texts[GridSnapshot::MemoryFree].empty();
    exahype2::runner::Table report;
    for (int row = 0; row < GridSnapshot::RowCount; row++) {
      if (texts[row].empty() or (joinFree and row == GridSnapshot::MemoryFree)) {
        continue;
      }
      const auto text = joinFree and row == GridSnapshot::MemoryResident
        ? texts[row] + " · Free " + texts[GridSnapshot::MemoryFree]
        : texts[row];
      report.add(RowLabels[row].section, RowLabels[row].label, text);
    }
    return report.render();
  }

  exahype2::runner::GridSnapshot::Entry getGridCellsEntry(
    const std::uint64_t numberOfCells,
    const std::uint64_t numberOfRefinedCells
  ) {
    return {
      exahype2::runner::Format::getGroupedNumber(numberOfCells),
      "Refined " + exahype2::runner::Format::getGroupedNumber(numberOfRefinedCells),
      ""
    };
  }
} // namespace

exahype2::runner::GlobalStatistics&
exahype2::runner::getGlobalStatistics() {
  static exahype2::runner::GlobalStatistics instance;
  return instance;
}

/**
 * Records the host and device bytes of the solvers of this rank. addLocalValuesToGlobalReduction()
 * adds them to the sums over the ranks.
 */
void
exahype2::runner::GlobalStatistics::setLocalSolverBytes(const std::size_t hostBytes, const std::size_t deviceBytes) {
  _localSolverHostBytes   = static_cast<std::uint64_t>(hostBytes);
  _localSolverDeviceBytes = static_cast<std::uint64_t>(deviceBytes);
}

/**
 * Records the solver cells and degrees of freedom of this rank, summed over the solvers.
 * addLocalValuesToGlobalReduction() adds them to the sums over the ranks.
 */
void
exahype2::runner::GlobalStatistics::setLocalCellUpdates(const std::uint64_t solverCells, const std::uint64_t cellDoFs) {
  _localSolverCells = solverCells;
  _localCellDoFs    = cellDoFs;
}

/**
 * Adds the local statistics of this rank to record. Only the node master adds the node memory and
 * every node counts once. A non-negative cell width and a stationary counter with a sign bias keep
 * their order under an unsigned minimum.
 */
void
exahype2::runner::GlobalStatistics::addLocalValuesToGlobalReduction(
  tarch::mpi::ReductionRecord& record,
  const bool                   refreshMemory
) {
  const auto& statistics = peano4::Spacetree::getInstance().getStatistics();
  assertion(statistics.numberOfCells >= 0, statistics.numberOfCells);
  assertion(statistics.numberOfRefinedCells >= 0, statistics.numberOfRefinedCells);
  assertion(statistics.numberOfFaces >= 0, statistics.numberOfFaces);
  assertion(statistics.numberOfHangingFaces >= 0, statistics.numberOfHangingFaces);
  assertion(statistics.numberOfExchangedFaces >= 0, statistics.numberOfExchangedFaces);
  assertion(statistics.numberOfExchangedFaces <= statistics.numberOfFaces, statistics.numberOfExchangedFaces);
  const auto                  localMinimumCellWidth = tarch::la::max(statistics.minimumCellWidth);
  [[maybe_unused]] const bool localWidthIsFinite    = std::isfinite(localMinimumCellWidth);
  assertion(localWidthIsFinite and localMinimumCellWidth >= 0.0, statistics.minimumCellWidth);

  const auto isNodeMaster = tarch::mpi::Rank::getInstance().isNodeMaster();
  const auto now          = std::chrono::steady_clock::now();
  if (
    refreshMemory or _lastMemorySample == std::chrono::steady_clock::time_point{}
    or now - _lastMemorySample >= MemorySamplingInterval
  ) {
    _lastMemorySample               = now;
    _localProcessMemoryUsageInBytes = tarch::getMemoryUsageInBytes();
    _localTotalMemoryInBytes        = isNodeMaster ? tarch::getTotalMemoryInBytes() : 0;
    _localAvailableMemoryInBytes    = isNodeMaster ? tarch::getAvailableMemoryInBytes() : 0;
  }

  const auto localCells             = static_cast<std::uint64_t>(statistics.numberOfCells);
  const auto localRankBoundaryFaces = static_cast<std::uint64_t>(statistics.numberOfExchangedFaces);
  const auto localStationaryRegrids = static_cast<std::int64_t>(statistics.numberOfStationaryRegrids);
  const auto biasedStationary       = toOrderedBits(localStationaryRegrids);

  std::array<std::uint64_t, NumberOfSums>   sums{};
  std::array<std::uint64_t, NumberOfMinima> minima{};
  std::array<std::uint64_t, NumberOfMaxima> maxima{};

  sums[SumOfCells]                       = localCells;
  sums[SumOfRefinedCells]                = static_cast<std::uint64_t>(statistics.numberOfRefinedCells);
  sums[SumOfFaces]                       = static_cast<std::uint64_t>(statistics.numberOfFaces);
  sums[SumOfHangingFaces]                = static_cast<std::uint64_t>(statistics.numberOfHangingFaces);
  sums[SumOfRankBoundaryFaces]           = localRankBoundaryFaces;
  sums[SumOfCellConnectivityHostBytes]   = static_cast<std::uint64_t>(statistics.cellConnectivityBytes.host);
  sums[SumOfCellConnectivityDeviceBytes] = static_cast<std::uint64_t>(statistics.cellConnectivityBytes.device);
  sums[SumOfTreeBytes]                   = static_cast<std::uint64_t>(statistics.treeBytes);
  sums[SumOfSolverHostBytes]             = _localSolverHostBytes;
  sums[SumOfSolverDeviceBytes]           = _localSolverDeviceBytes;
  sums[SumOfSolverCells]                 = _localSolverCells;
  sums[SumOfCellDoFs]                    = _localCellDoFs;
  sums[SumOfProcessMemory]               = _localProcessMemoryUsageInBytes;
  sums[SumOfTotalMemory]                 = isNodeMaster ? _localTotalMemoryInBytes : 0;
  sums[SumOfAvailableMemory]             = isNodeMaster ? _localAvailableMemoryInBytes : 0;

  minima[MinimumRankLoad]          = localCells;
  minima[MinimumLevel]             = static_cast<std::uint64_t>(statistics.minimumLevel);
  minima[MinimumCellWidth]         = toBits(localMinimumCellWidth);
  minima[MinimumStationaryRegrids] = biasedStationary;
  minima[MinimumRankBoundaryFaces] = localRankBoundaryFaces;

  maxima[MaximumRankLoad]          = localCells;
  maxima[MaximumLevel]             = static_cast<std::uint64_t>(statistics.maximumLevel);
  maxima[MaximumRankBoundaryFaces] = localRankBoundaryFaces;

  _firstSumSlot     = record.addSums(sums);
  _firstMinimumSlot = record.addMinima(minima);
  _firstMaximumSlot = record.addMaxima(maxima);
}

/**
 * Reads the global statistics from record after its allreduce. addLocalValuesToGlobalReduction()
 * added the local statistics to this record.
 */
void
exahype2::runner::GlobalStatistics::readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record) {
  const auto getSum     = [this, &record](const SumSlot slot) { return record.getSum(_firstSumSlot + slot); };
  const auto getMinimum = [this, &record](const MinimumSlot slot) {
    return record.getMinimum(_firstMinimumSlot + slot);
  };
  const auto getMaximum = [this, &record](const MaximumSlot slot) {
    return record.getMaximum(_firstMaximumSlot + slot);
  };

  _globalNumberOfInnerUnrefinedCells = getSum(SumOfCells);
  _globalNumberOfInnerRefinedCells   = getSum(SumOfRefinedCells);
  // The ranks on both sides of a rank boundary face count it. The grid contains half of their sum.
  assertion(getSum(SumOfRankBoundaryFaces) % 2 == 0, getSum(SumOfRankBoundaryFaces));
  assertion(
    getSum(SumOfRankBoundaryFaces) / 2 <= getSum(SumOfFaces),
    getSum(SumOfRankBoundaryFaces),
    getSum(SumOfFaces)
  );
  _globalNumberOfRankBoundaryFaces   = getSum(SumOfRankBoundaryFaces) / 2;
  _globalNumberOfFaces               = getSum(SumOfFaces) - _globalNumberOfRankBoundaryFaces;
  _globalNumberOfHangingFaces        = getSum(SumOfHangingFaces);
  _globalCellConnectivityHostBytes   = getSum(SumOfCellConnectivityHostBytes);
  _globalCellConnectivityDeviceBytes = getSum(SumOfCellConnectivityDeviceBytes);
  _globalTreeBytes                   = getSum(SumOfTreeBytes);
  _globalSolverHostBytes             = getSum(SumOfSolverHostBytes);
  _globalSolverDeviceBytes           = getSum(SumOfSolverDeviceBytes);
  _globalNumberOfSolverCells         = getSum(SumOfSolverCells);
  _globalNumberOfCellDoFs            = getSum(SumOfCellDoFs);
  _globalProcessMemoryUsageInBytes   = getSum(SumOfProcessMemory);
  _globalTotalMemoryInBytes          = getSum(SumOfTotalMemory);
  _globalAvailableMemoryInBytes      = getSum(SumOfAvailableMemory);
  _globalMinRankLoad                 = getMinimum(MinimumRankLoad);
  _globalMinimumLevel                = static_cast<int>(getMinimum(MinimumLevel));
  _globalMinimumCellWidth            = fromBits(getMinimum(MinimumCellWidth));
  _globalNumberOfStationaryRegrids   = static_cast<int>(fromOrderedBits(getMinimum(MinimumStationaryRegrids)));
  _globalMinRankBoundaryFaces        = getMinimum(MinimumRankBoundaryFaces);
  _globalMaxRankLoad                 = getMaximum(MaximumRankLoad);
  _globalMaximumLevel                = static_cast<int>(getMaximum(MaximumLevel));
  _globalMaxRankBoundaryFaces        = getMaximum(MaximumRankBoundaryFaces);
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfInnerUnrefinedCells() const {
  return _globalNumberOfInnerUnrefinedCells;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfInnerRefinedCells() const {
  return _globalNumberOfInnerRefinedCells;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfFaces() const {
  return _globalNumberOfFaces;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfHangingFaces() const {
  return _globalNumberOfHangingFaces;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalMinRankBoundaryFaces() const {
  return _globalMinRankBoundaryFaces;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalMaxRankBoundaryFaces() const {
  return _globalMaxRankBoundaryFaces;
}

double
exahype2::runner::GlobalStatistics::getGlobalRankBoundaryFacePercentage() const {
  return _globalNumberOfFaces > 0
    ? 100.0 * static_cast<double>(_globalNumberOfRankBoundaryFaces) / static_cast<double>(_globalNumberOfFaces)
    : 0.0;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalMinRankLoad() const {
  return _globalMinRankLoad;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalMaxRankLoad() const {
  return _globalMaxRankLoad;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalCellConnectivityHostBytes() const {
  return _globalCellConnectivityHostBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalCellConnectivityDeviceBytes() const {
  return _globalCellConnectivityDeviceBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalTreeBytes() const {
  return _globalTreeBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalSolverHostBytes() const {
  return _globalSolverHostBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalSolverDeviceBytes() const {
  return _globalSolverDeviceBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfSolverCells() const {
  return _globalNumberOfSolverCells;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalNumberOfCellDoFs() const {
  return _globalNumberOfCellDoFs;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalProcessMemoryUsageInBytes() const {
  return _globalProcessMemoryUsageInBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalTotalMemoryInBytes() const {
  return _globalTotalMemoryInBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalAvailableMemoryInBytes() const {
  return _globalAvailableMemoryInBytes;
}

/**
 * Returns the percentage of the ideal speed-up that the domain decomposition allows. The mean rank
 * load is the even load. The heaviest rank sets the makespan.
 */
double
exahype2::runner::GlobalStatistics::getGlobalInterRankBalance() const {
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (_globalMaxRankLoad == 0 or numberOfRanks <= 1) {
    return 100.0;
  }
  const auto meanRankLoad = static_cast<double>(_globalNumberOfInnerUnrefinedCells) / numberOfRanks;
  return 100.0 * meanRankLoad / static_cast<double>(_globalMaxRankLoad);
}

int
exahype2::runner::GlobalStatistics::getGlobalMinimumLevel() const {
  return _globalMinimumLevel;
}

int
exahype2::runner::GlobalStatistics::getGlobalMaximumLevel() const {
  return _globalMaximumLevel;
}

double
exahype2::runner::GlobalStatistics::getGlobalMinimumCellWidth() const {
  return _globalMinimumCellWidth;
}

int
exahype2::runner::GlobalStatistics::getGlobalNumberOfStationaryRegrids() const {
  return _globalNumberOfStationaryRegrids;
}

/**
 * Reads the global grid statistics into one snapshot. The MPI rows stay empty on a single rank.
 * The GPU rows stay empty in a build without a GPU backend.
 */
exahype2::runner::GridSnapshot
exahype2::runner::collectGridSnapshot() {
  const auto&                    statistics = exahype2::runner::getGlobalStatistics();
  exahype2::runner::GridSnapshot snapshot;
  auto&                          entries = snapshot.entries;

  entries[GridSnapshot::GridCells] = getGridCellsEntry(
    statistics.getGlobalNumberOfInnerUnrefinedCells(),
    statistics.getGlobalNumberOfInnerRefinedCells()
  );
  entries[GridSnapshot::GridFaces] = {
    exahype2::runner::Format::getGroupedNumber(statistics.getGlobalNumberOfFaces()),
    "Hanging " + exahype2::runner::Format::getGroupedNumber(statistics.getGlobalNumberOfHangingFaces()),
    ""
  };
  entries[GridSnapshot::GridLevels] = {
    std::to_string(statistics.getGlobalMinimumLevel()) + " .. " + std::to_string(statistics.getGlobalMaximumLevel()),
    "",
    ""
  };

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    entries[GridSnapshot::MPILoadBalance] = {
      exahype2::runner::Format::getPercentage(statistics.getGlobalInterRankBalance()),
      exahype2::runner::Format::getGroupedNumber(statistics.getGlobalMinRankLoad()) + " .. "
        + exahype2::runner::Format::getGroupedNumber(statistics.getGlobalMaxRankLoad()) + " Cells/Rank",
      ""
    };
    entries[GridSnapshot::MPIRankBoundaryFaces] = {
      exahype2::runner::Format::getPercentage(statistics.getGlobalRankBoundaryFacePercentage()),
      exahype2::runner::Format::getGroupedNumber(statistics.getGlobalMinRankBoundaryFaces()) + " .. "
        + exahype2::runner::Format::getGroupedNumber(statistics.getGlobalMaxRankBoundaryFaces()) + " Faces/Rank",
      ""
    };
  }

  entries[GridSnapshot::MemoryResident] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalProcessMemoryUsageInBytes()),
    "",
    ""
  };
  entries[GridSnapshot::MemoryCPUGrid] = {
    exahype2::runner::Format::getMemory(
      statistics.getGlobalCellConnectivityHostBytes() + statistics.getGlobalTreeBytes()
    ),
    "",
    ""
  };
  entries[GridSnapshot::MemoryCPUSolverData] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalSolverHostBytes()),
    "",
    ""
  };
#if defined(PEANO_WITH_GPU_BACKEND)
  entries[GridSnapshot::MemoryGPUGrid] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalCellConnectivityDeviceBytes()),
    "",
    ""
  };
  entries[GridSnapshot::MemoryGPUSolverData] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalSolverDeviceBytes()),
    "",
    ""
  };
#endif
  entries[GridSnapshot::MemoryFree] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalAvailableMemoryInBytes()),
    "",
    " of " + exahype2::runner::Format::getMemory(statistics.getGlobalTotalMemoryInBytes())
  };
  return snapshot;
}

/**
 * Renders every row with a value under its section title.
 */
std::vector<std::string>
exahype2::runner::describeGridSnapshot(const exahype2::runner::GridSnapshot& snapshot) {
  std::array<std::string, GridSnapshot::RowCount> texts{};
  for (int row = 0; row < GridSnapshot::RowCount; row++) {
    const auto& entry = snapshot.entries[row];
    if (not entry.value.empty()) {
      texts[row] = formatEntry(entry.value, entry);
    }
  }
  return renderRows(texts);
}

/**
 * Renders every row with a value in after, with the suffix and the detail of after. A changed value
 * reads "before → after".
 */
std::vector<std::string>
exahype2::runner::describeGridTransition(
  const exahype2::runner::GridSnapshot& before,
  const exahype2::runner::GridSnapshot& after
) {
  std::array<std::string, GridSnapshot::RowCount> texts{};
  for (int row = 0; row < GridSnapshot::RowCount; row++) {
    const auto& entryAfter  = after.entries[row];
    const auto& valueBefore = before.entries[row].value;
    if (not entryAfter.value.empty()) {
      const auto value = valueBefore == entryAfter.value ? entryAfter.value : valueBefore + " → " + entryAfter.value;
      texts[row]       = formatEntry(value, entryAfter);
    }
  }
  return renderRows(texts);
}

void
exahype2::runner::writeGridSnapshot(exahype2::runner::Report& report) {
  for (const auto& line : exahype2::runner::describeGridSnapshot(exahype2::runner::collectGridSnapshot())) {
    report.line(line);
  }
}
