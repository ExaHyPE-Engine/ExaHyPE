// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  struct GridSnapshot;
  struct RepositoryCallbacks;
  class ReportBox;

  std::uint64_t boundedFullyRefinedLeaves(int maximumLevel);
  std::string   gridCellSummary(std::uint64_t numberOfLeaves, std::uint64_t numberOfRefinedCells);

  GridSnapshot             collectGridSnapshot(const RepositoryCallbacks& callbacks);
  std::vector<std::string> describeGridSnapshot(const GridSnapshot& snapshot);
  std::vector<std::string> describeGridTransition(const GridSnapshot& before, const GridSnapshot& after);
  void                     writeGridSnapshot(ReportBox& box, const RepositoryCallbacks& callbacks);
} // namespace runner

/**
 * Holds one entry per row of the grid summary. A row's detail is the
 * bracketed text behind its value, and a row with an empty value stays out
 * of every report.
 */
struct runner::GridSnapshot {
  enum Row {
    GridLeaves,
    GridVertices,
    GridLevels,
    MpiRanks,
    MpiLoadBalance,
    MpiSpineReplication,
    OpenMpSubtreeTasks,
    OpenMpCellsPerTask,
    OpenMpThreadCeiling,
    MemorySystem,
    MemoryProcess,
    MemoryGridVertices,
    MemorySolverHeap,
    RowCount
  };

  struct Entry {
    std::string value;
    std::string detail;
  };

  std::array<Entry, RowCount> entries{{}};
};
