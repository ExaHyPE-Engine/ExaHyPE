// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshot.h"

#include "runner/Format.h"
#include "runner/LoadBalancer.h"
#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "runner/RuntimeParameters.h"
#include "tarch/MemoryUsage.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

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
  return Format::grouped(numberOfLeaves) + "  (Cells " + Format::grouped(numberOfCells) + ")";
}

void runner::writeGridSnapshot(ReportBox& box, const RepositoryCallbacks& callbacks) {
  const auto& statistics           = loadBalancer().getStatistics();
  const auto  numberOfLeaves       = statistics.getGlobalNumberOfInnerUnrefinedCells();
  const auto  numberOfRefinedCells = statistics.getGlobalNumberOfInnerRefinedCells();
  Table       report;

  {
    const auto     numberOfVertices        = statistics.getGlobalNumberOfVertices();
    const auto     numberOfHangingVertices = statistics.getGlobalNumberOfHangingVertices();
    constexpr auto MaximumCount            = std::numeric_limits<std::uint64_t>::max();
    const auto     numberOfAllVertices     = numberOfHangingVertices > MaximumCount - numberOfVertices
              ? MaximumCount
              : numberOfVertices + numberOfHangingVertices;

    std::ostringstream vertices;
    vertices << Format::grouped(numberOfAllVertices) << "  (Local " << Format::grouped(numberOfVertices)
             << " · Hanging " << Format::grouped(numberOfHangingVertices) << ")";
    std::ostringstream levels;
    levels << statistics.getGlobalMinDepth() << " .. " << statistics.getGlobalMaxDepth();

    report.add("Grid", "Number of Leaves", gridCellSummary(numberOfLeaves, numberOfRefinedCells));
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
    treeText << Format::grouped(numberOfTrees) << "  (on " << Format::grouped(numberOfRanks) << " "
             << (numberOfRanks == 1 ? "rank)" : "ranks)");
    std::ostringstream efficiencyText;
    efficiencyText << Format::percentage(treeEfficiency) << "  (" << Format::grouped(minimumTreeLoad) << " .. "
                   << Format::grouped(maximumTreeLoad) << ")";
    std::ostringstream overheadText;
    overheadText << Format::overheadPercentage(overhead) << "  (" << Format::grouped(minimumOuterTreeLoad) << " .. "
                 << Format::grouped(maximumOuterTreeLoad) << ")";

    report.add("Load Balancing", "Number of Trees", treeText.str());
    // Rank efficiency compares accumulated owned leaf cells between populated ranks, so it needs at least two ranks.
    if (numberOfRanks > 1) {
      std::ostringstream rankText;
      rankText << Format::percentage(rankEfficiency) << "  (" << Format::grouped(minimumRankLoad) << " .. "
               << Format::grouped(maximumRankLoad) << ")";
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
    system << Format::memory(totalMemory) << " RAM · " << Format::memory(freeMemory) << " free · "
           << Format::memory(residentSetSize) << " RSS";
    report.add("Memory", "System", system.str());
    report.add("Memory", "Grid", Format::memory(gridMemory));
    report.add(
      "Memory",
      "Heap",
      "Cell " + Format::memory(cellMemory) + " · Face " + Format::memory(faceMemory) + " · Vertex "
        + Format::memory(vertexMemory)
    );
    report.add("Memory", "Total Data", Format::memory(dataTotal) + " tracked");
  }

  box.rule();
  for (const auto& line : report.render()) {
    box.line(line);
  }
  box.rule();
}

std::vector<std::string> runner::gridSnapshotWidthSamples() {
  const auto& parameters             = RuntimeParameters::getInstance();
  const auto  leafBound              = boundedFullyRefinedLeaves(parameters.getMaxGridLevels());
  const auto  numberOfRanks          = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const auto  numberOfNodes          = tarch::mpi::Rank::getInstance().getNumberOfNodes();
  const auto  configuredTreesPerRank = parameters.getMaxNumberOfTreesPerRank();
  const auto  maximumTreesPerRank =
    configuredTreesPerRank > 0 ? configuredTreesPerRank : tarch::multicore::Core::getInstance().getNumberOfThreads();
  const auto saturatingMultiply = [](const std::uint64_t left, const std::uint64_t right) {
    constexpr auto MaximumValue = std::numeric_limits<std::uint64_t>::max();
    return right > 0 and left > MaximumValue / right ? MaximumValue : left * right;
  };
  const auto treeBound = saturatingMultiply(
    static_cast<std::uint64_t>(std::max(1, numberOfRanks)),
    static_cast<std::uint64_t>(std::max(1, maximumTreesPerRank))
  );

  // Per-tree replicas can increase aggregate grid counts, while summed process RSS can count shared pages per rank.
  constexpr auto VerticesPerCell     = std::uint64_t{1} << DIMENSIONS;
  const auto     replicatedCellBound = saturatingMultiply(leafBound, treeBound);
  const auto     vertexBound         = saturatingMultiply(replicatedCellBound, VerticesPerCell);
  const auto     memoryPerNode       = tarch::getTotalMemoryInBytes();
  const auto     maximumMemoryInBytes =
    saturatingMultiply(memoryPerNode, static_cast<std::uint64_t>(std::max({1, numberOfRanks, numberOfNodes})));

  const auto maximumCount    = Format::grouped(vertexBound);
  const auto leaves          = Format::grouped(leafBound);
  const auto trees           = Format::grouped(treeBound);
  const auto maximumMemory   = Format::memory(maximumMemoryInBytes);
  const auto maximumLevel    = Format::number(parameters.getMaxGridLevels());
  const auto fullEfficiency  = Format::percentage(100.0);
  const auto maximumOverhead = Format::overheadPercentageWidthSample();

  Table report;
  report.add("Grid", "Number of Leaves", gridCellSummary(leafBound, replicatedCellBound));
  report
    .add("Grid", "Number of Vertices", maximumCount + "  (Local " + maximumCount + " · Hanging " + maximumCount + ")");
  report.add("Grid", "Grid Levels", maximumLevel + " .. " + maximumLevel);
  report.add(
    "Load Balancing",
    "Number of Trees",
    trees + "  (on " + Format::grouped(numberOfRanks) + (numberOfRanks == 1 ? " rank)" : " ranks)")
  );
  report.add("Load Balancing", "Rank Efficiency", fullEfficiency + "  (" + leaves + " .. " + leaves + ")");
  report.add("Load Balancing", "Global Efficiency", fullEfficiency + "  (" + leaves + " .. " + leaves + ")");
  report.add("Load Balancing", "Global Overhead", maximumOverhead + "  (" + maximumCount + " .. " + maximumCount + ")");

  report.add("Memory", "System", maximumMemory + " RAM · " + maximumMemory + " free · " + maximumMemory + " RSS");
  report.add("Memory", "Grid", maximumMemory);
  report.add("Memory", "Heap", "Cell " + maximumMemory + " · Face " + maximumMemory + " · Vertex " + maximumMemory);
  report.add("Memory", "Total Data", maximumMemory + " tracked");
  return report.render();
}
