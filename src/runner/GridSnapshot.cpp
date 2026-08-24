// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshot.h"

#include "runner/Format.h"
#include "runner/LoadBalancer.h"
#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"

/** Configured levels can exceed a representable cell count, so the display bound saturates. */
std::uint64_t runner::boundedFullyRefinedLeaves(const int maximumLevel) {
  if (maximumLevel < 0) {
    throw std::invalid_argument("The maximum grid level must not be negative.");
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
 * Cells cover owned leaves and refined parents. Outer and halo cells are excluded, and the sum saturates on overflow.
 */
std::string runner::gridCellSummary(const std::uint64_t numberOfLeaves, const std::uint64_t numberOfRefinedCells) {
  constexpr auto MaximumCount = std::numeric_limits<std::uint64_t>::max();
  const auto     numberOfCells =
    numberOfRefinedCells > MaximumCount - numberOfLeaves ? MaximumCount : numberOfLeaves + numberOfRefinedCells;
  return runner::Format::grouped(numberOfLeaves) + "  (Cells " + runner::Format::grouped(numberOfCells) + ")";
}

void runner::writeGridSnapshot(runner::ReportBox& box, const runner::RepositoryCallbacks& callbacks) {
  const auto&   statistics           = runner::loadBalancer().getStatistics();
  const auto    numberOfLeaves       = statistics.getGlobalNumberOfInnerUnrefinedCells();
  const auto    numberOfRefinedCells = statistics.getGlobalNumberOfInnerRefinedCells();
  runner::Table report;

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

    report.add("Grid", "Number of Leaves", runner::gridCellSummary(numberOfLeaves, numberOfRefinedCells));
    report.add("Grid", "Number of Vertices", vertices.str());
    report.add("Grid", "Grid Levels", levels.str());
  }

  {
    const auto numberOfOuterCells   = statistics.getGlobalNumberOfOuterUnrefinedCells();
    const auto numberOfTrees        = static_cast<unsigned long>(statistics.getGlobalNumberOfTrees());
    const auto numberOfRanks        = static_cast<unsigned long>(statistics.getGlobalNumberOfRanksWithAtLeastOneTree());
    const auto minimumTreeLoad      = static_cast<unsigned long>(statistics.getGlobalMinTreeLoad());
    const auto maximumTreeLoad      = static_cast<unsigned long>(statistics.getGlobalMaxTreeLoad());
    const auto minimumOuterTreeLoad = static_cast<unsigned long>(statistics.getGlobalMinTreeOuterLoad());
    const auto maximumOuterTreeLoad = static_cast<unsigned long>(statistics.getGlobalMaxTreeOuterLoad());
    const auto minimumRankLoad      = statistics.getGlobalMinRankLoad();
    const auto maximumRankLoad      = statistics.getGlobalMaxRankLoad();

    const auto treeEfficiency =
      maximumTreeLoad > 0 ? static_cast<double>(minimumTreeLoad) / maximumTreeLoad * 100.0 : 100.0;
    const auto rankEfficiency =
      maximumRankLoad > 0 ? static_cast<double>(minimumRankLoad) / maximumRankLoad * 100.0 : 100.0;
    const auto overhead = numberOfLeaves > 0 ? static_cast<double>(numberOfOuterCells) / numberOfLeaves * 100.0 : 0.0;

    std::ostringstream treeText;
    treeText << runner::Format::grouped(numberOfTrees) << "  (on " << runner::Format::grouped(numberOfRanks) << " "
             << (numberOfRanks == 1 ? "rank)" : "ranks)");
    std::ostringstream efficiencyText;
    efficiencyText << runner::Format::percentage(treeEfficiency) << "  (" << runner::Format::grouped(minimumTreeLoad)
                   << " .. " << runner::Format::grouped(maximumTreeLoad) << ")";
    std::ostringstream overheadText;
    overheadText << runner::Format::overheadPercentage(overhead) << "  ("
                 << runner::Format::grouped(minimumOuterTreeLoad) << " .. "
                 << runner::Format::grouped(maximumOuterTreeLoad) << ")";

    report.add("Load Balancing", "Number of Trees", treeText.str());
    // Rank efficiency compares accumulated owned leaf cells between populated ranks, so it needs at least two ranks.
    if (numberOfRanks > 1) {
      std::ostringstream rankText;
      rankText << runner::Format::percentage(rankEfficiency) << "  (" << runner::Format::grouped(minimumRankLoad)
               << " .. " << runner::Format::grouped(maximumRankLoad) << ")";
      report.add("Load Balancing", "Rank Efficiency", rankText.str());
    }
    report.add("Load Balancing", "Global Efficiency", efficiencyText.str());
    report.add("Load Balancing", "Global Overhead", overheadText.str());
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

    std::ostringstream system;
    system << runner::Format::memory(totalMemory) << " RAM · " << runner::Format::memory(freeMemory) << " free · "
           << runner::Format::memory(residentSetSize) << " RSS";
    report.add("Memory", "System", system.str());
    report.add("Memory", "Grid", runner::Format::memory(gridMemory));
    report.add(
      "Memory",
      "Heap",
      "Cell " + runner::Format::memory(cellMemory) + " · Face " + runner::Format::memory(faceMemory) + " · Vertex "
        + runner::Format::memory(vertexMemory)
    );
    report.add("Memory", "Total Data", runner::Format::memory(dataTotal) + " tracked");
  }

  box.rule();
  for (const auto& line : report.render()) {
    box.line(line);
  }
  box.rule();
}
