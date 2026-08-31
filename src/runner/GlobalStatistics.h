// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  class GlobalStatistics;

  GlobalStatistics& globalStatistics();
  void              shutdownGlobalStatistics();
} // namespace runner

/**
 * Global view of the grid across all ranks.
 *
 * The engine synchronises the view after every program step, so readers see
 * the metrics of the latest completed sweep.
 */
class runner::GlobalStatistics final {
public:
  GlobalStatistics() = default;

  void synchronise();

  std::uint64_t getGlobalNumberOfInnerUnrefinedCells() const;
  std::uint64_t getGlobalNumberOfInnerRefinedCells() const;
  std::uint64_t getGlobalNumberOfReplicatedCells() const;
  std::uint64_t getGlobalMinRankReplicatedCells() const;
  std::uint64_t getGlobalMaxRankReplicatedCells() const;
  double        getGlobalReplicationOverhead() const;
  std::uint64_t getGlobalNumberOfVertices() const;
  std::uint64_t getGlobalNumberOfHangingVertices() const;
  std::uint64_t getGlobalMinRankLoad() const;
  std::uint64_t getGlobalMaxRankLoad() const;
  std::uint64_t getGlobalMemoryUsageInBytes() const;
  std::uint64_t getGlobalProcessMemoryUsageInBytes() const;
  std::uint64_t getGlobalTotalMemoryInBytes() const;
  std::uint64_t getGlobalFreeMemoryInBytes() const;
  std::uint64_t getGlobalNumberOfSubtreeTasks() const;
  std::uint64_t getGlobalMinRankSubtreeTasks() const;
  std::uint64_t getGlobalMaxRankSubtreeTasks() const;
  std::uint64_t getGlobalMinSubtreeTaskCells() const;
  std::uint64_t getGlobalMaxSubtreeTaskCells() const;
  std::uint64_t getGlobalSubtreeTaskCells() const;
  std::uint64_t getGlobalNumberOfThreads() const;
  std::uint64_t getGlobalThreadCeiling() const;
  double        getGlobalInterRankBalance() const;
  int           getGlobalMinDepth() const;
  int           getGlobalMaxDepth() const;
  double        getGlobalMinH() const;
  int           getGlobalNumberOfStationarySweeps() const;

private:
  /**
   * The process and node memory figures come from the kernel and cost a file
   * read per call, so they refresh at most once per interval; a report reads a
   * figure at most that old.
   */
  static constexpr std::chrono::seconds MemorySamplingInterval{1};

  std::chrono::steady_clock::time_point _lastMemorySample{};
  std::uint64_t                         _localProcessMemoryUsageInBytes{0};
  std::uint64_t                         _localTotalMemoryInBytes{0};
  std::uint64_t                         _localFreeMemoryInBytes{0};

  std::uint64_t _globalNumberOfInnerUnrefinedCells{0};
  std::uint64_t _globalNumberOfInnerRefinedCells{0};
  std::uint64_t _globalNumberOfVertices{0};
  std::uint64_t _globalNumberOfHangingVertices{0};
  std::uint64_t _globalMinRankLoad{0};
  std::uint64_t _globalMaxRankLoad{0};
  std::uint64_t _globalMemoryUsageInBytes{0};
  std::uint64_t _globalProcessMemoryUsageInBytes{0};
  std::uint64_t _globalTotalMemoryInBytes{0};
  std::uint64_t _globalFreeMemoryInBytes{0};
  std::uint64_t _globalNumberOfSubtreeTasks{0};
  std::uint64_t _globalMinRankSubtreeTasks{0};
  std::uint64_t _globalMaxRankSubtreeTasks{0};
  std::uint64_t _globalMinSubtreeTaskCells{0};
  std::uint64_t _globalMaxSubtreeTaskCells{0};
  std::uint64_t _globalSubtreeTaskCells{0};
  std::uint64_t _globalNumberOfThreads{0};
  std::uint64_t _globalNumberOfReplicatedCells{0};
  std::uint64_t _globalMinRankReplicatedCells{0};
  std::uint64_t _globalMaxRankReplicatedCells{0};
  int           _globalMinDepth{0};
  int           _globalMaxDepth{0};
  double        _globalMinH{0.0};
  int           _globalNumberOfStationarySweeps{0};
};
