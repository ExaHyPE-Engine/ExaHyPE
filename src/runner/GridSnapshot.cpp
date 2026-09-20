// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshot.h"

#include "peano4/sfc/ActiveCurve.h"
#include "runner/Format.h"
#include "runner/GlobalStatistics.h"
#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"

namespace {
  struct RowLabel {
    const char* section{nullptr};
    const char* label{nullptr};
  };

  constexpr std::array<RowLabel, runner::GridSnapshot::RowCount> RowLabels{{
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
    {"Memory", "Solver Pools"},
  }};

  std::string formatEntry(const runner::GridSnapshot::Entry& entry) {
    return entry.detail.empty() ? entry.value : entry.value + "  (" + entry.detail + ")";
  }

  /** The memory rows stay in a transition report whether or not their value changed. */
  constexpr bool staysInTransition(const int row) {
    return row == runner::GridSnapshot::MemoryProcess or row == runner::GridSnapshot::MemorySolverHeap;
  }

  runner::GridSnapshot::Entry gridCellsEntry(
    const std::uint64_t numberOfLeaves,
    const std::uint64_t numberOfRefinedCells
  ) {
    constexpr auto MaximumCount = std::numeric_limits<std::uint64_t>::max();
    const auto     numberOfCells =
      numberOfRefinedCells > MaximumCount - numberOfLeaves ? MaximumCount : numberOfLeaves + numberOfRefinedCells;
    return {runner::Format::grouped(numberOfLeaves), "Cells " + runner::Format::grouped(numberOfCells)};
  }
} // namespace

/** Configured levels can exceed a representable cell count, so the display bound saturates. */
std::uint64_t runner::boundedFullyRefinedLeaves(const int maximumLevel) {
  if (maximumLevel < 0) {
    tarch::terminateWithError("The maximum grid level has to be non-negative.");
  }

  constexpr auto MaximumCount      = std::numeric_limits<std::uint64_t>::max();
  constexpr auto SubdivisionFactor = static_cast<std::uint64_t>(peano4::sfc::ActiveCurve::SubdivisionFactor);
  auto           numberOfLeaves    = std::uint64_t{1};
  for (int level = 0; level < maximumLevel; ++level) {
    for (int axis = 0; axis < DIMENSIONS; ++axis) {
      if (numberOfLeaves > MaximumCount / SubdivisionFactor) {
        return MaximumCount;
      }
      numberOfLeaves *= SubdivisionFactor;
    }
  }
  return numberOfLeaves;
}

/**
 * Returns the number of leaves with the total number of cells as detail. The
 * sum of the two counts saturates on overflow.
 */
std::string runner::gridCellSummary(const std::uint64_t numberOfLeaves, const std::uint64_t numberOfRefinedCells) {
  return formatEntry(gridCellsEntry(numberOfLeaves, numberOfRefinedCells));
}

/**
 * Reads the global grid statistics and the repository's memory counters into
 * one snapshot. The MPI rows stay empty on a single rank.
 */
runner::GridSnapshot runner::collectGridSnapshot(const runner::RepositoryCallbacks& callbacks) {
  const auto&          statistics = runner::globalStatistics();
  runner::GridSnapshot snapshot;
  auto&                entries = snapshot.entries;

  {
    const auto     numberOfVertices        = statistics.getGlobalNumberOfVertices();
    const auto     numberOfHangingVertices = statistics.getGlobalNumberOfHangingVertices();
    constexpr auto MaximumCount            = std::numeric_limits<std::uint64_t>::max();
    const auto     numberOfAllVertices     = numberOfHangingVertices > MaximumCount - numberOfVertices
      ? MaximumCount
      : numberOfVertices + numberOfHangingVertices;

    entries[GridSnapshot::GridLeaves] = gridCellsEntry(
      statistics.getGlobalNumberOfInnerUnrefinedCells(),
      statistics.getGlobalNumberOfInnerRefinedCells()
    );
    entries[GridSnapshot::GridVertices] = {
      runner::Format::grouped(numberOfAllVertices),
      "Local " + runner::Format::grouped(numberOfVertices) + " · Hanging "
        + runner::Format::grouped(numberOfHangingVertices)
    };
    entries[GridSnapshot::GridLevels] = {
      std::to_string(statistics.getGlobalMinDepth()) + " .. " + std::to_string(statistics.getGlobalMaxDepth()),
      ""
    };
  }

  {
    const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();

    // A single rank owns everything it visits.
    if (numberOfRanks > 1) {
      entries[GridSnapshot::MpiRanks]       = {runner::Format::grouped(static_cast<std::uint64_t>(numberOfRanks)), ""};
      entries[GridSnapshot::MpiLoadBalance] = {
        runner::Format::percentage(statistics.getGlobalInterRankBalance()),
        runner::Format::grouped(statistics.getGlobalMinRankLoad()) + " .. "
          + runner::Format::grouped(statistics.getGlobalMaxRankLoad()) + " Leaves/Rank"
      };
      entries[GridSnapshot::MpiSpineReplication] = {
        runner::Format::overheadPercentage(statistics.getGlobalReplicationOverhead()),
        runner::Format::grouped(statistics.getGlobalMinRankReplicatedCells()) + " .. "
          + runner::Format::grouped(statistics.getGlobalMaxRankReplicatedCells()) + " Spine Cells/Rank"
      };
    }

    // A rank traverses serially when it has one thread, when the grid is too
    // flat to decompose, or when a container cannot be split across tasks, and
    // then it owns no task at all.
    const auto numberOfSubtreeTasks = statistics.getGlobalNumberOfSubtreeTasks();
    if (numberOfSubtreeTasks == 0) {
      entries[GridSnapshot::OpenMpSubtreeTasks]  = {"None", "Ranks traverse serially"};
      entries[GridSnapshot::OpenMpCellsPerTask]  = {"Not applicable", ""};
      entries[GridSnapshot::OpenMpThreadCeiling] = {"Not applicable", ""};
    } else {
      const auto threadCeiling                  = statistics.getGlobalThreadCeiling();
      const auto numberOfThreads                = statistics.getGlobalNumberOfThreads();
      entries[GridSnapshot::OpenMpSubtreeTasks] = {
        runner::Format::grouped(numberOfSubtreeTasks),
        runner::Format::grouped(statistics.getGlobalMinRankSubtreeTasks()) + " .. "
          + runner::Format::grouped(statistics.getGlobalMaxRankSubtreeTasks()) + " Tasks/Rank"
      };
      entries[GridSnapshot::OpenMpCellsPerTask] = {
        runner::Format::grouped(statistics.getGlobalMinSubtreeTaskCells()) + " .. "
          + runner::Format::grouped(statistics.getGlobalMaxSubtreeTaskCells()) + " Cells",
        "Mean " + runner::Format::grouped(statistics.getGlobalSubtreeTaskCells() / numberOfSubtreeTasks)
      };
      entries[GridSnapshot::OpenMpThreadCeiling] = {
        runner::Format::grouped(threadCeiling) + " of " + runner::Format::grouped(numberOfThreads) + " Threads",
        threadCeiling < numberOfThreads
          ? "Largest Task " + runner::Format::grouped(statistics.getGlobalMaxSubtreeTaskCells()) + " Cells"
          : ""
      };
    }
  }

  {
    const auto bytesFrom = [](const std::function<std::size_t()>& callback) -> unsigned long long {
      return callback ? callback() : 0;
    };
    const auto cellMemory   = bytesFrom(callbacks.getGlobalCellDataMemoryUsageInBytes);
    const auto faceMemory   = bytesFrom(callbacks.getGlobalFaceDataMemoryUsageInBytes);
    const auto vertexMemory = bytesFrom(callbacks.getGlobalVertexDataMemoryUsageInBytes);

    entries[GridSnapshot::MemorySystem] = {
      runner::Format::memory(statistics.getGlobalFreeMemoryInBytes()) + " free",
      runner::Format::memory(statistics.getGlobalTotalMemoryInBytes()) + " RAM"
    };
    entries[GridSnapshot::MemoryProcess] = {
      runner::Format::memory(statistics.getGlobalProcessMemoryUsageInBytes()),
      "Sum over Ranks"
    };
    entries[GridSnapshot::MemoryGridVertices] = {
      runner::Format::memory(statistics.getGlobalMemoryUsageInBytes()),
      "Estimated"
    };
    entries[GridSnapshot::MemorySolverHeap] = {
      runner::Format::memory(cellMemory + faceMemory + vertexMemory),
      "Cell " + runner::Format::memory(cellMemory) + " · Face " + runner::Format::memory(faceMemory) + " · Vertex "
        + runner::Format::memory(vertexMemory)
    };
  }

  return snapshot;
}

/** Renders every row that carries a value under its section title. */
std::vector<std::string> runner::describeGridSnapshot(const runner::GridSnapshot& snapshot) {
  runner::Table report;
  for (int row = 0; row < GridSnapshot::RowCount; row++) {
    const auto& entry = snapshot.entries[row];
    if (not entry.value.empty()) {
      report.add(RowLabels[row].section, RowLabels[row].label, formatEntry(entry));
    }
  }
  return report.render();
}

/**
 * Renders the rows whose value changed between the two snapshots as
 * "before → after" with the detail of @p after, and the memory rows in every
 * case.
 */
std::vector<std::string> runner::describeGridTransition(
  const runner::GridSnapshot& before,
  const runner::GridSnapshot& after
) {
  runner::Table report;
  for (int row = 0; row < GridSnapshot::RowCount; row++) {
    const auto& entryAfter  = after.entries[row];
    const auto& valueBefore = before.entries[row].value;
    if (entryAfter.value.empty()) {
      continue;
    }
    if (valueBefore != entryAfter.value) {
      report.add(
        RowLabels[row].section,
        RowLabels[row].label,
        formatEntry({valueBefore + " → " + entryAfter.value, entryAfter.detail})
      );
    } else if (staysInTransition(row)) {
      report.add(RowLabels[row].section, RowLabels[row].label, formatEntry(entryAfter));
    }
  }
  return report.render();
}

void runner::writeGridSnapshot(runner::ReportBox& box, const runner::RepositoryCallbacks& callbacks) {
  box.rule();
  for (const auto& line : runner::describeGridSnapshot(runner::collectGridSnapshot(callbacks))) {
    box.line(line);
  }
  box.rule();
}
