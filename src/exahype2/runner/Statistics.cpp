// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Statistics.h"

#include "exahype2/runner/Console.h"
#include "peano4/Spacetree.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

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

  /** Slots of one reduction record, in the three contiguous segments the operation sums, minimises and maximises. */
  enum SumSlot {
    SumOfCells,
    SumOfRefinedCells,
    SumOfFaces,
    SumOfHangingFaces,
    SumOfRankBoundaryFaces,
    SumOfPartitionHostBytes,
    SumOfPartitionDeviceBytes,
    SumOfTreeBytes,
    SumOfSolverHostBytes,
    SumOfSolverDeviceBytes,
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
  constexpr int NumberOfValues = NumberOfSums + NumberOfMinima + NumberOfMaxima;

  MPI_Datatype StatisticsRecordType         = MPI_DATATYPE_NULL;
  MPI_Op       StatisticsReductionOperation = MPI_OP_NULL;

  /**
   * Applies sum, minimum and maximum to the fixed segments of each record. The parameter types
   * follow MPI_User_function, which MPI_Op_create() takes.
   */
  void reduceStatisticsRecords(
    void*                          incoming,
    void*                          accumulating,
    int*                           numberOfRecords, // NOLINT(readability-non-const-parameter)
    [[maybe_unused]] MPI_Datatype* datatype
  ) {
    const auto* const incomingValues = static_cast<const std::uint64_t*>(incoming);
    auto* const       accumulator    = static_cast<std::uint64_t*>(accumulating);
    for (int record = 0; record < *numberOfRecords; record++) {
      const auto        offset = static_cast<std::ptrdiff_t>(record) * NumberOfValues;
      const auto* const source = incomingValues + offset;
      auto* const       target = accumulator + offset;
      for (int entry = 0; entry < NumberOfSums; entry++) {
        target[entry] += source[entry];
      }
      for (int entry = NumberOfSums; entry < NumberOfSums + NumberOfMinima; entry++) {
        target[entry] = std::min(target[entry], source[entry]);
      }
      for (int entry = NumberOfSums + NumberOfMinima; entry < NumberOfValues; entry++) {
        target[entry] = std::max(target[entry], source[entry]);
      }
    }
  }

  /**
   * Returns the reduction operation over whole statistics records and hands out the committed
   * record datatype, creating both on first use. A record datatype keeps the segment offsets of
   * the operation valid where the library pipelines a reduction, because it never splits inside
   * an element. shutdownGlobalStatistics() releases both handles before MPI is finalised.
   */
  MPI_Op getStatisticsReduction(MPI_Datatype& recordType) {
    if (StatisticsReductionOperation == MPI_OP_NULL) {
      tarch::mpi::checkReturnValue(
        MPI_Type_contiguous(NumberOfValues, MPI_UINT64_T, &StatisticsRecordType),
        "MPI_Type_contiguous",
        std::source_location::current()
      );
      tarch::mpi::checkReturnValue(
        MPI_Type_commit(&StatisticsRecordType),
        "MPI_Type_commit",
        std::source_location::current()
      );
      tarch::mpi::checkReturnValue(
        MPI_Op_create(reduceStatisticsRecords, 1, &StatisticsReductionOperation),
        "MPI_Op_create",
        std::source_location::current()
      );
    }
    recordType = StatisticsRecordType;
    return StatisticsReductionOperation;
  }

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
    {"Memory", "Used"},
    {"Memory", "Free"},
  }};

  std::string formatEntry(const std::string& value, const exahype2::runner::GridSnapshot::Entry& entry) {
    return value + entry.suffix + (entry.detail.empty() ? "" : "  (" + entry.detail + ")");
  }

  /**
   * Renders every row with a text under its section title. The free memory continues the line
   * of the used memory when both carry a text.
   */
  std::vector<std::string> renderRows(const std::array<std::string, exahype2::runner::GridSnapshot::RowCount>& texts) {
    using GridSnapshot  = exahype2::runner::GridSnapshot;
    const bool joinFree = not texts[GridSnapshot::MemoryUsed].empty() and not texts[GridSnapshot::MemoryFree].empty();
    exahype2::runner::Table report;
    for (int row = 0; row < GridSnapshot::RowCount; row++) {
      if (texts[row].empty() or (joinFree and row == GridSnapshot::MemoryFree)) {
        continue;
      }
      const auto text = joinFree and row == GridSnapshot::MemoryUsed
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

void
exahype2::runner::shutdownGlobalStatistics() {
  if (StatisticsReductionOperation != MPI_OP_NULL) {
    tarch::mpi::checkReturnValue(
      MPI_Op_free(&StatisticsReductionOperation),
      "MPI_Op_free",
      std::source_location::current()
    );
  }
  if (StatisticsRecordType != MPI_DATATYPE_NULL) {
    tarch::mpi::checkReturnValue(
      MPI_Type_free(&StatisticsRecordType),
      "MPI_Type_free",
      std::source_location::current()
    );
  }
}

/**
 * Records the bytes the solvers of this rank hold in host and in device memory; synchronise()
 * sums them over the ranks.
 */
void
exahype2::runner::GlobalStatistics::setLocalSolverBytes(const std::size_t hostBytes, const std::size_t deviceBytes) {
  _localSolverHostBytes   = static_cast<std::uint64_t>(hostBytes);
  _localSolverDeviceBytes = static_cast<std::uint64_t>(deviceBytes);
}

/** Records the degrees of freedom on the cells of this rank; synchronise() sums them over the ranks. */
void
exahype2::runner::GlobalStatistics::setLocalCellDoFs(const std::uint64_t cellDoFs) {
  _localCellDoFs = cellDoFs;
}

/**
 * Reduces the local grid numbers into the global view with one blocking allreduce. The total and
 * the available memory of a node enter through its node master alone, so every node counts once.
 * The cell width travels as its bit pattern and the stationary counter with a sign bias, so both
 * stay order-preserving under an unsigned minimum. With refreshMemory set, the memory figures
 * are read anew.
 */
void
exahype2::runner::GlobalStatistics::synchronise(const bool refreshMemory) {
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

  std::array<std::uint64_t, NumberOfValues> values{};
  auto* const                               sums   = values.data();
  auto* const                               minima = values.data() + NumberOfSums;
  auto* const                               maxima = values.data() + NumberOfSums + NumberOfMinima;

  sums[SumOfCells]                = localCells;
  sums[SumOfRefinedCells]         = static_cast<std::uint64_t>(statistics.numberOfRefinedCells);
  sums[SumOfFaces]                = static_cast<std::uint64_t>(statistics.numberOfFaces);
  sums[SumOfHangingFaces]         = static_cast<std::uint64_t>(statistics.numberOfHangingFaces);
  sums[SumOfRankBoundaryFaces]    = localRankBoundaryFaces;
  sums[SumOfPartitionHostBytes]   = static_cast<std::uint64_t>(statistics.partitionBytes.host);
  sums[SumOfPartitionDeviceBytes] = static_cast<std::uint64_t>(statistics.partitionBytes.device);
  sums[SumOfTreeBytes]            = static_cast<std::uint64_t>(statistics.treeBytes);
  sums[SumOfSolverHostBytes]      = _localSolverHostBytes;
  sums[SumOfSolverDeviceBytes]    = _localSolverDeviceBytes;
  sums[SumOfCellDoFs]             = _localCellDoFs;
  sums[SumOfProcessMemory]        = _localProcessMemoryUsageInBytes;
  sums[SumOfTotalMemory]          = isNodeMaster ? _localTotalMemoryInBytes : 0;
  sums[SumOfAvailableMemory]      = isNodeMaster ? _localAvailableMemoryInBytes : 0;

  minima[MinimumRankLoad]          = localCells;
  minima[MinimumLevel]             = static_cast<std::uint64_t>(statistics.minimumLevel);
  minima[MinimumCellWidth]         = toBits(localMinimumCellWidth);
  minima[MinimumStationaryRegrids] = biasedStationary;
  minima[MinimumRankBoundaryFaces] = localRankBoundaryFaces;

  maxima[MaximumRankLoad]          = localCells;
  maxima[MaximumLevel]             = static_cast<std::uint64_t>(statistics.maximumLevel);
  maxima[MaximumRankBoundaryFaces] = localRankBoundaryFaces;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    MPI_Datatype recordType = MPI_DATATYPE_NULL;
    const MPI_Op reduction  = getStatisticsReduction(recordType);
    tarch::mpi::allreduce(
      MPI_IN_PLACE,
      values.data(),
      1,
      recordType,
      reduction,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }

  _globalNumberOfInnerUnrefinedCells = sums[SumOfCells];
  _globalNumberOfInnerRefinedCells   = sums[SumOfRefinedCells];
  // The two ranks next to a rank boundary face both count it, so the grid holds half of their sum.
  assertion(sums[SumOfRankBoundaryFaces] % 2 == 0, sums[SumOfRankBoundaryFaces]);
  assertion(sums[SumOfRankBoundaryFaces] / 2 <= sums[SumOfFaces], sums[SumOfRankBoundaryFaces], sums[SumOfFaces]);
  _globalNumberOfRankBoundaryFaces = sums[SumOfRankBoundaryFaces] / 2;
  _globalNumberOfFaces             = sums[SumOfFaces] - _globalNumberOfRankBoundaryFaces;
  _globalNumberOfHangingFaces      = sums[SumOfHangingFaces];
  _globalPartitionHostBytes        = sums[SumOfPartitionHostBytes];
  _globalPartitionDeviceBytes      = sums[SumOfPartitionDeviceBytes];
  _globalTreeBytes                 = sums[SumOfTreeBytes];
  _globalSolverHostBytes           = sums[SumOfSolverHostBytes];
  _globalSolverDeviceBytes         = sums[SumOfSolverDeviceBytes];
  _globalNumberOfCellDoFs          = sums[SumOfCellDoFs];
  _globalProcessMemoryUsageInBytes = sums[SumOfProcessMemory];
  _globalTotalMemoryInBytes        = sums[SumOfTotalMemory];
  _globalAvailableMemoryInBytes    = sums[SumOfAvailableMemory];
  _globalMinRankLoad               = minima[MinimumRankLoad];
  _globalMinimumLevel              = static_cast<int>(minima[MinimumLevel]);
  _globalMinimumCellWidth          = fromBits(minima[MinimumCellWidth]);
  _globalNumberOfStationaryRegrids = static_cast<int>(fromOrderedBits(minima[MinimumStationaryRegrids]));
  _globalMinRankBoundaryFaces      = minima[MinimumRankBoundaryFaces];
  _globalMaxRankLoad               = maxima[MaximumRankLoad];
  _globalMaximumLevel              = static_cast<int>(maxima[MaximumLevel]);
  _globalMaxRankBoundaryFaces      = maxima[MaximumRankBoundaryFaces];
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

/** Returns the percentage of the faces of the grid that lie on a rank boundary. */
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
exahype2::runner::GlobalStatistics::getGlobalPartitionHostBytes() const {
  return _globalPartitionHostBytes;
}

std::uint64_t
exahype2::runner::GlobalStatistics::getGlobalPartitionDeviceBytes() const {
  return _globalPartitionDeviceBytes;
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
 * Returns the percentage of the ideal speed-up the partition allows across the ranks. A rank is
 * one unit of work here, so the mean rank load is the even load and the heaviest rank sets the
 * makespan.
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
 * Reads the global grid statistics into one snapshot. The MPI rows stay empty on a single rank,
 * and the GPU rows stay empty in a build without a GPU backend.
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

  entries[GridSnapshot::MemoryUsed] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalProcessMemoryUsageInBytes()),
    "",
    ""
  };
  entries[GridSnapshot::MemoryCPUGrid] = {
    exahype2::runner::Format::getMemory(statistics.getGlobalPartitionHostBytes() + statistics.getGlobalTreeBytes()),
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
    exahype2::runner::Format::getMemory(statistics.getGlobalPartitionDeviceBytes()),
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

/** Renders every row that carries a value under its section title. */
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
 * Renders every row that carries a value after the transition, with the suffix and the detail of
 * after. A changed value reads "before → after".
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
