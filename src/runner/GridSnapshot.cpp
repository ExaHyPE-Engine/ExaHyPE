// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshot.h"

#include "runner/Format.h"
#include "runner/GlobalStatistics.h"
#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

namespace {
  enum SnapshotRowNumber {
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
    MemoryTrackedData,
    SnapshotRowCount
  };

  struct SnapshotRow {
    const char* section{nullptr};
    const char* label{nullptr};
  };

  constexpr std::array<SnapshotRow, SnapshotRowCount> SnapshotRows{{
    {"Grid", "Number of Leaves"},
    {"Grid", "Number of Vertices"},
    {"Grid", "Grid Levels"},
    {"MPI Decomposition", "Number of MPI Ranks"},
    {"MPI Decomposition", "MPI Load Balance"},
    {"MPI Decomposition", "Spine Replication"},
    {"OpenMP Decomposition", "Subtree Tasks"},
    {"OpenMP Decomposition", "Cells per Task"},
    {"OpenMP Decomposition", "Thread Ceiling"},
    {"Memory", "System Memory"},
    {"Memory", "Process RSS"},
    {"Memory", "Grid Vertices"},
    {"Memory", "Solver Heap"},
    {"Memory", "Tracked Data"},
  }};

  using SnapshotValues = std::array<std::string, SnapshotRowCount>;

  /** Renders the rows that carry a value; a row left empty stays out of the report. */
  std::vector<std::string> renderSnapshotRows(const SnapshotValues& values) {
    runner::Table report;
    for (int rowNumber = 0; rowNumber < SnapshotRowCount; rowNumber++) {
      if (not values[rowNumber].empty()) {
        report.add(SnapshotRows[rowNumber].section, SnapshotRows[rowNumber].label, values[rowNumber]);
      }
    }
    return report.render();
  }
} // namespace

/** Configured levels can exceed a representable cell count, so the display bound saturates. */
std::uint64_t runner::boundedFullyRefinedLeaves(const int maximumLevel) {
  if (maximumLevel < 0) {
    tarch::terminateWithError("The maximum grid level has to be non-negative.");
  }

  constexpr auto MaximumCount   = std::numeric_limits<std::uint64_t>::max();
  auto           numberOfLeaves = std::uint64_t{1};
  for (int level = 0; level < maximumLevel; ++level) {
    for (int axis = 0; axis < DIMENSIONS; ++axis) {
      if (numberOfLeaves > MaximumCount / 3) {
        return MaximumCount;
      }
      numberOfLeaves *= 3;
    }
  }
  return numberOfLeaves;
}

/**
 * Counts owned leaves and refined parents. Outer and halo cells are excluded,
 * and the sum saturates on overflow.
 */
std::string runner::gridCellSummary(const std::uint64_t numberOfLeaves, const std::uint64_t numberOfRefinedCells) {
  constexpr auto MaximumCount = std::numeric_limits<std::uint64_t>::max();
  const auto     numberOfCells =
    numberOfRefinedCells > MaximumCount - numberOfLeaves ? MaximumCount : numberOfLeaves + numberOfRefinedCells;
  return runner::Format::grouped(numberOfLeaves) + "  (Cells " + runner::Format::grouped(numberOfCells) + ")";
}

void runner::writeGridSnapshot(runner::ReportBox& box, const runner::RepositoryCallbacks& callbacks) {
  const auto&    statistics           = runner::globalStatistics();
  const auto     numberOfLeaves       = statistics.getGlobalNumberOfInnerUnrefinedCells();
  const auto     numberOfRefinedCells = statistics.getGlobalNumberOfInnerRefinedCells();
  SnapshotValues values;

  {
    const auto     numberOfVertices        = statistics.getGlobalNumberOfVertices();
    const auto     numberOfHangingVertices = statistics.getGlobalNumberOfHangingVertices();
    constexpr auto MaximumCount            = std::numeric_limits<std::uint64_t>::max();
    const auto     numberOfAllVertices     = numberOfHangingVertices > MaximumCount - numberOfVertices
              ? MaximumCount
              : numberOfVertices + numberOfHangingVertices;

    std::ostringstream vertices;
    vertices << runner::Format::grouped(numberOfAllVertices) << "  (Local " << runner::Format::grouped(numberOfVertices)
             << " · Hanging " << runner::Format::grouped(numberOfHangingVertices) << ")";
    std::ostringstream levels;
    levels << statistics.getGlobalMinDepth() << " .. " << statistics.getGlobalMaxDepth();

    values[GridLeaves]   = runner::gridCellSummary(numberOfLeaves, numberOfRefinedCells);
    values[GridVertices] = vertices.str();
    values[GridLevels]   = levels.str();
  }

  {
    const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    // A single rank owns everything it visits.
    if (numberOfRanks > 1) {
      std::ostringstream interRankText;
      interRankText << runner::Format::percentage(statistics.getGlobalInterRankBalance()) << "  ("
                    << runner::Format::grouped(statistics.getGlobalMinRankLoad()) << " .. "
                    << runner::Format::grouped(statistics.getGlobalMaxRankLoad()) << " Leaves/Rank)";
      std::ostringstream replicationText;
      replicationText << runner::Format::overheadPercentage(statistics.getGlobalReplicationOverhead()) << "  ("
                      << runner::Format::grouped(statistics.getGlobalMinRankReplicatedCells()) << " .. "
                      << runner::Format::grouped(statistics.getGlobalMaxRankReplicatedCells()) << " Spine Cells/Rank)";

      values[MpiRanks]            = runner::Format::grouped(static_cast<std::uint64_t>(numberOfRanks));
      values[MpiLoadBalance]      = interRankText.str();
      values[MpiSpineReplication] = replicationText.str();
    }

    // A rank traverses serially when it has one thread, when the grid is too
    // flat to decompose, or when a container cannot be split across tasks, and
    // then it owns no task at all.
    const auto         numberOfSubtreeTasks = statistics.getGlobalNumberOfSubtreeTasks();
    std::ostringstream subtreeTaskText;
    std::ostringstream taskLoadText;
    std::ostringstream threadCeilingText;
    if (numberOfSubtreeTasks == 0) {
      subtreeTaskText << "None  (Ranks traverse serially)";
      taskLoadText << "Not applicable";
      threadCeilingText << "Not applicable";
    } else {
      subtreeTaskText << runner::Format::grouped(numberOfSubtreeTasks) << "  ("
                      << runner::Format::grouped(statistics.getGlobalMinRankSubtreeTasks()) << " .. "
                      << runner::Format::grouped(statistics.getGlobalMaxRankSubtreeTasks()) << " Tasks/Rank)";
      taskLoadText << runner::Format::grouped(statistics.getGlobalMinSubtreeTaskCells()) << " .. "
                   << runner::Format::grouped(statistics.getGlobalMaxSubtreeTaskCells()) << " Cells  (Mean "
                   << runner::Format::grouped(statistics.getGlobalSubtreeTaskCells() / numberOfSubtreeTasks) << ")";
      const auto threadCeiling   = statistics.getGlobalThreadCeiling();
      const auto numberOfThreads = statistics.getGlobalNumberOfThreads();
      threadCeilingText << runner::Format::grouped(threadCeiling) << " of " << runner::Format::grouped(numberOfThreads)
                        << " Threads";
      if (threadCeiling < numberOfThreads) {
        threadCeilingText << "  (Largest Task " << runner::Format::grouped(statistics.getGlobalMaxSubtreeTaskCells())
                          << " Cells)";
      }
    }
    values[OpenMpSubtreeTasks]  = subtreeTaskText.str();
    values[OpenMpCellsPerTask]  = taskLoadText.str();
    values[OpenMpThreadCeiling] = threadCeilingText.str();
  }

  {
    const auto bytesFrom = [](const std::function<std::size_t()>& callback) -> unsigned long long {
      return callback ? callback() : 0;
    };
    const auto gridMemory      = statistics.getGlobalMemoryUsageInBytes();
    const auto cellMemory      = bytesFrom(callbacks.getGlobalCellDataMemoryUsageInBytes);
    const auto faceMemory      = bytesFrom(callbacks.getGlobalFaceDataMemoryUsageInBytes);
    const auto vertexMemory    = bytesFrom(callbacks.getGlobalVertexDataMemoryUsageInBytes);
    const auto dataTotal       = gridMemory + cellMemory + faceMemory + vertexMemory;
    const auto residentSetSize = statistics.getGlobalProcessMemoryUsageInBytes();

    const auto totalMemory = statistics.getGlobalTotalMemoryInBytes();
    const auto freeMemory  = statistics.getGlobalFreeMemoryInBytes();

    std::ostringstream systemMemory;
    systemMemory << runner::Format::memory(totalMemory) << " RAM · " << runner::Format::memory(freeMemory) << " free";
    values[MemorySystem]       = systemMemory.str();
    values[MemoryProcess]      = runner::Format::memory(residentSetSize) + "  (Sum over Ranks)";
    values[MemoryGridVertices] = runner::Format::memory(gridMemory) + "  (Estimated)";
    values[MemorySolverHeap]   = "Cell " + runner::Format::memory(cellMemory) + " · Face "
      + runner::Format::memory(faceMemory) + " · Vertex " + runner::Format::memory(vertexMemory);
    values[MemoryTrackedData] = runner::Format::memory(dataTotal);
  }

  box.rule();
  for (const auto& line : renderSnapshotRows(values)) {
    box.line(line);
  }
  box.rule();
}
