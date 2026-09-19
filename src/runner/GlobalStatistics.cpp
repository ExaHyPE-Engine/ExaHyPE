// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GlobalStatistics.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/topology/TopologyManager.h"
#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/la/VectorOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

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

  /**
   * Slots of one reduction record, in the three contiguous segments the custom
   * operation sums, minimises and maximises.
   */
  enum SumSlot {
    SumOfLeaves,
    SumOfRefinedCells,
    SumOfVertices,
    SumOfHangingVertices,
    SumOfGridMemory,
    SumOfProcessMemory,
    SumOfTotalMemory,
    SumOfFreeMemory,
    SumOfSubtreeTasks,
    SumOfSubtreeTaskCells,
    SumOfReplicatedCells,
    SumOfThreads,
    SumSlotCount
  };

  enum MinimumSlot {
    MinimumRankLoad,
    MinimumDepth,
    MinimumMeshWidth,
    MinimumStationarySweeps,
    MinimumRankSubtreeTasks,
    MinimumSubtreeTaskCells,
    MinimumRankReplicatedCells,
    MinimumSlotCount
  };

  enum MaximumSlot {
    MaximumRankLoad,
    MaximumDepth,
    MaximumRankSubtreeTasks,
    MaximumSubtreeTaskCells,
    MaximumRankReplicatedCells,
    MaximumSlotCount
  };

  constexpr int NumberOfSums   = SumSlotCount;
  constexpr int NumberOfMinima = MinimumSlotCount;
  constexpr int NumberOfMaxima = MaximumSlotCount;
  constexpr int NumberOfValues = NumberOfSums + NumberOfMinima + NumberOfMaxima;

  MPI_Datatype StatisticsRecordType         = MPI_DATATYPE_NULL;
  MPI_Op       StatisticsReductionOperation = MPI_OP_NULL;

  /** Applies sum, minimum and maximum to the fixed segments of each record. */
  void reduceStatisticsRecords(
    void*                          incoming,
    void*                          accumulating,
    int*                           numberOfRecords,
    [[maybe_unused]] MPI_Datatype* datatype
  ) {
    const auto* const incomingValues = static_cast<const std::uint64_t*>(incoming);
    auto* const       accumulator    = static_cast<std::uint64_t*>(accumulating);
    for (int record = 0; record < *numberOfRecords; record++) {
      const auto* const source = incomingValues + record * NumberOfValues;
      auto* const       target = accumulator + record * NumberOfValues;
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
   * Returns the reduction operation over whole statistics records and hands
   * out the committed record datatype, creating both on first use. A record
   * datatype keeps the operation's segment offsets valid even where the
   * library pipelines a reduction, because it never splits inside an
   * element. The shutdown routine releases both handles before MPI is
   * finalised.
   */
  MPI_Op statisticsReduction(MPI_Datatype& recordType) {
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
} // namespace

runner::GlobalStatistics& runner::globalStatistics() {
  static runner::GlobalStatistics instance;
  return instance;
}

void runner::shutdownGlobalStatistics() {
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
 * Reduces the local grid numbers into the global view with one blocking
 * allreduce. Host memory enters through the node master alone, so the memory of
 * one node is counted once per node. The mesh width travels as its bit pattern
 * and the stationary counter with a sign bias, so both stay order-preserving
 * under an unsigned minimum.
 */
void runner::GlobalStatistics::synchronise() {
  const auto& topologyManager = peano4::topology::TopologyManager::getInstance();
  const auto& statistics      = topologyManager.getGridStatistics();

  assertion(statistics.numberOfLocalUnrefinedCells >= 0, statistics.numberOfLocalUnrefinedCells);
  assertion(statistics.numberOfRemoteUnrefinedCells >= 0, statistics.numberOfRemoteUnrefinedCells);
  assertion(statistics.numberOfLocalRefinedCells >= 0, statistics.numberOfLocalRefinedCells);
  assertion(statistics.numberOfVertices >= 0, statistics.numberOfVertices);
  assertion(statistics.numberOfHangingVertices >= 0, statistics.numberOfHangingVertices);
  assertion(std::isfinite(tarch::la::max(statistics.minH)), statistics.minH);
  assertion(tarch::la::max(statistics.minH) >= 0.0, statistics.minH);
  const auto subtreeTaskLoad = topologyManager.getSubtreeTaskLoad();
  assertion(subtreeTaskLoad.numberOfTasks >= 0, subtreeTaskLoad.numberOfTasks);
  assertion(subtreeTaskLoad.minimumCells >= 0, subtreeTaskLoad.minimumCells);
  assertion(subtreeTaskLoad.maximumCells >= 0, subtreeTaskLoad.maximumCells);

  const auto isNodeMaster = tarch::mpi::Rank::getInstance().isNodeMaster();
  const auto now          = std::chrono::steady_clock::now();
  if (
    _lastMemorySample == std::chrono::steady_clock::time_point{} or now - _lastMemorySample >= MemorySamplingInterval
  ) {
    _lastMemorySample               = now;
    _localProcessMemoryUsageInBytes = tarch::getMemoryUsageInBytes();
    _localTotalMemoryInBytes        = isNodeMaster ? tarch::getTotalMemoryInBytes() : 0;
    _localFreeMemoryInBytes         = isNodeMaster ? tarch::getFreeMemoryInBytes() : 0;
  }

  const auto localLeaves           = static_cast<std::uint64_t>(statistics.numberOfLocalUnrefinedCells);
  const auto localGridMemory       = static_cast<std::uint64_t>(statistics.getMemoryUsageInBytes());
  const auto localMinH             = tarch::la::max(statistics.minH);
  const auto localStationarySweeps = static_cast<std::int64_t>(statistics.numberOfStationarySweeps);
  const auto biasedStationary      = toOrderedBits(localStationarySweeps);
  // Cells a rank visits without owning them. A sweep skips a foreign subtree
  // below the spawn level outright, so what remains is the coarse spine above
  // it, which every rank traverses in full.
  const auto localReplicatedCells = static_cast<std::uint64_t>(statistics.numberOfRemoteRefinedCells)
    + static_cast<std::uint64_t>(statistics.numberOfRemoteUnrefinedCells);
  const auto localSubtreeTasks     = static_cast<std::uint64_t>(subtreeTaskLoad.numberOfTasks);
  const auto localSubtreeTaskCells = static_cast<std::uint64_t>(subtreeTaskLoad.totalCells);
  // A rank that traverses serially owns no task, so it must not pull the smallest
  // task down to zero.
  const auto localMinimumTaskCells = subtreeTaskLoad.numberOfTasks > 0
    ? static_cast<std::uint64_t>(subtreeTaskLoad.minimumCells)
    : std::numeric_limits<std::uint64_t>::max();
  const auto localMaximumTaskCells = static_cast<std::uint64_t>(subtreeTaskLoad.maximumCells);
  const auto localNumberOfThreads =
    static_cast<std::uint64_t>(std::max(1, tarch::tasking::Runtime::getInstance().getNumberOfThreads()));
  std::array<std::uint64_t, NumberOfValues> values{};
  auto* const                               sums   = values.data();
  auto* const                               minima = values.data() + NumberOfSums;
  auto* const                               maxima = values.data() + NumberOfSums + NumberOfMinima;

  sums[SumOfLeaves]           = localLeaves;
  sums[SumOfRefinedCells]     = static_cast<std::uint64_t>(statistics.numberOfLocalRefinedCells);
  sums[SumOfVertices]         = static_cast<std::uint64_t>(statistics.numberOfVertices);
  sums[SumOfHangingVertices]  = static_cast<std::uint64_t>(statistics.numberOfHangingVertices);
  sums[SumOfGridMemory]       = localGridMemory;
  sums[SumOfProcessMemory]    = _localProcessMemoryUsageInBytes;
  sums[SumOfTotalMemory]      = isNodeMaster ? _localTotalMemoryInBytes : 0;
  sums[SumOfFreeMemory]       = isNodeMaster ? _localFreeMemoryInBytes : 0;
  sums[SumOfSubtreeTasks]     = localSubtreeTasks;
  sums[SumOfSubtreeTaskCells] = localSubtreeTaskCells;
  sums[SumOfReplicatedCells]  = localReplicatedCells;
  sums[SumOfThreads]          = localNumberOfThreads;

  minima[MinimumRankLoad]            = localLeaves;
  minima[MinimumDepth]               = static_cast<std::uint64_t>(statistics.minDepth);
  minima[MinimumMeshWidth]           = toBits(localMinH);
  minima[MinimumStationarySweeps]    = biasedStationary;
  minima[MinimumRankSubtreeTasks]    = localSubtreeTasks;
  minima[MinimumSubtreeTaskCells]    = localMinimumTaskCells;
  minima[MinimumRankReplicatedCells] = localReplicatedCells;

  maxima[MaximumRankLoad]            = localLeaves;
  maxima[MaximumDepth]               = static_cast<std::uint64_t>(statistics.maxDepth);
  maxima[MaximumRankSubtreeTasks]    = localSubtreeTasks;
  maxima[MaximumSubtreeTaskCells]    = localMaximumTaskCells;
  maxima[MaximumRankReplicatedCells] = localReplicatedCells;

  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    MPI_Datatype recordType = MPI_DATATYPE_NULL;
    const auto   reduction  = statisticsReduction(recordType);
    tarch::mpi::allreduce(
      MPI_IN_PLACE,
      values.data(),
      1,
      recordType,
      reduction,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }

  _globalNumberOfInnerUnrefinedCells = sums[SumOfLeaves];
  _globalNumberOfInnerRefinedCells   = sums[SumOfRefinedCells];
  _globalNumberOfVertices            = sums[SumOfVertices];
  _globalNumberOfHangingVertices     = sums[SumOfHangingVertices];
  _globalMemoryUsageInBytes          = sums[SumOfGridMemory];
  _globalProcessMemoryUsageInBytes   = sums[SumOfProcessMemory];
  _globalTotalMemoryInBytes          = sums[SumOfTotalMemory];
  _globalFreeMemoryInBytes           = sums[SumOfFreeMemory];
  _globalNumberOfSubtreeTasks        = sums[SumOfSubtreeTasks];
  _globalSubtreeTaskCells            = sums[SumOfSubtreeTaskCells];
  _globalNumberOfReplicatedCells     = sums[SumOfReplicatedCells];
  _globalNumberOfThreads             = sums[SumOfThreads];
  _globalMinRankLoad                 = minima[MinimumRankLoad];
  _globalMinDepth                    = static_cast<int>(minima[MinimumDepth]);
  _globalMinH                        = fromBits(minima[MinimumMeshWidth]);
  _globalNumberOfStationarySweeps    = static_cast<int>(fromOrderedBits(minima[MinimumStationarySweeps]));
  _globalMinRankSubtreeTasks         = minima[MinimumRankSubtreeTasks];
  _globalMinSubtreeTaskCells         = _globalNumberOfSubtreeTasks > 0 ? minima[MinimumSubtreeTaskCells] : 0;
  _globalMinRankReplicatedCells      = minima[MinimumRankReplicatedCells];
  _globalMaxRankLoad                 = maxima[MaximumRankLoad];
  _globalMaxDepth                    = static_cast<int>(maxima[MaximumDepth]);
  _globalMaxRankSubtreeTasks         = maxima[MaximumRankSubtreeTasks];
  _globalMaxSubtreeTaskCells         = maxima[MaximumSubtreeTaskCells];
  _globalMaxRankReplicatedCells      = maxima[MaximumRankReplicatedCells];
}

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfInnerUnrefinedCells() const {
  return _globalNumberOfInnerUnrefinedCells;
}

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfInnerRefinedCells() const {
  return _globalNumberOfInnerRefinedCells;
}

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfVertices() const { return _globalNumberOfVertices; }

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfHangingVertices() const {
  return _globalNumberOfHangingVertices;
}

std::uint64_t runner::GlobalStatistics::getGlobalMinRankLoad() const { return _globalMinRankLoad; }

std::uint64_t runner::GlobalStatistics::getGlobalMaxRankLoad() const { return _globalMaxRankLoad; }

std::uint64_t runner::GlobalStatistics::getGlobalMemoryUsageInBytes() const { return _globalMemoryUsageInBytes; }

std::uint64_t runner::GlobalStatistics::getGlobalProcessMemoryUsageInBytes() const {
  return _globalProcessMemoryUsageInBytes;
}

std::uint64_t runner::GlobalStatistics::getGlobalTotalMemoryInBytes() const { return _globalTotalMemoryInBytes; }

std::uint64_t runner::GlobalStatistics::getGlobalFreeMemoryInBytes() const { return _globalFreeMemoryInBytes; }

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfSubtreeTasks() const { return _globalNumberOfSubtreeTasks; }

std::uint64_t runner::GlobalStatistics::getGlobalMinRankSubtreeTasks() const { return _globalMinRankSubtreeTasks; }

std::uint64_t runner::GlobalStatistics::getGlobalMaxRankSubtreeTasks() const { return _globalMaxRankSubtreeTasks; }

std::uint64_t runner::GlobalStatistics::getGlobalMinSubtreeTaskCells() const { return _globalMinSubtreeTaskCells; }

std::uint64_t runner::GlobalStatistics::getGlobalMaxSubtreeTaskCells() const { return _globalMaxSubtreeTaskCells; }

std::uint64_t runner::GlobalStatistics::getGlobalSubtreeTaskCells() const { return _globalSubtreeTaskCells; }

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfThreads() const { return _globalNumberOfThreads; }

/**
 * Returns how many threads the subtree decomposition can keep busy. No thread
 * splits a task, so a sweep lasts at least as long as its largest task.
 */
std::uint64_t runner::GlobalStatistics::getGlobalThreadCeiling() const {
  if (_globalMaxSubtreeTaskCells == 0 or _globalNumberOfThreads == 0) {
    return 0;
  }
  return std::min(_globalNumberOfThreads, _globalSubtreeTaskCells / _globalMaxSubtreeTaskCells);
}

std::uint64_t runner::GlobalStatistics::getGlobalNumberOfReplicatedCells() const {
  return _globalNumberOfReplicatedCells;
}

std::uint64_t runner::GlobalStatistics::getGlobalMinRankReplicatedCells() const {
  return _globalMinRankReplicatedCells;
}

std::uint64_t runner::GlobalStatistics::getGlobalMaxRankReplicatedCells() const {
  return _globalMaxRankReplicatedCells;
}

/**
 * Returns the percentage the cells the ranks visit without owning them make of
 * the cells they own. A rank drops a foreign subtree below the spawn level
 * before it enters, so what it repeats is the coarse spine above that level.
 */
double runner::GlobalStatistics::getGlobalReplicationOverhead() const {
  const auto ownedCells = _globalNumberOfInnerUnrefinedCells + _globalNumberOfInnerRefinedCells;
  return ownedCells > 0 ? 100.0 * static_cast<double>(_globalNumberOfReplicatedCells) / ownedCells : 0.0;
}

/**
 * Returns the percentage of the ideal speed-up the partition allows across the
 * ranks. A rank is one unit of work here, so the mean rank load is the even
 * load and the heaviest rank sets the makespan.
 */
double runner::GlobalStatistics::getGlobalInterRankBalance() const {
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (_globalMaxRankLoad == 0 or numberOfRanks <= 1) {
    return 100.0;
  }
  const auto meanRankLoad = static_cast<double>(_globalNumberOfInnerUnrefinedCells) / numberOfRanks;
  return 100.0 * meanRankLoad / static_cast<double>(_globalMaxRankLoad);
}

int runner::GlobalStatistics::getGlobalMinDepth() const { return _globalMinDepth; }

int runner::GlobalStatistics::getGlobalMaxDepth() const { return _globalMaxDepth; }

double runner::GlobalStatistics::getGlobalMinH() const { return _globalMinH; }

int runner::GlobalStatistics::getGlobalNumberOfStationarySweeps() const { return _globalNumberOfStationarySweeps; }
