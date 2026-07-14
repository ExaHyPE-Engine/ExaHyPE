// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridSnapshot.h"

#include "runner/Format.h"
#include "runner/LoadBalancer.h"
#include "runner/ReportBox.h"
#include "runner/RepositoryCallbacks.h"
#include "tarch/MemoryUsage.h"
#include "tarch/mpi/Rank.h"

void runner::writeGridSnapshot(ReportBox& box, const RepositoryCallbacks& repositories) {
  const auto& statistics = loadBalancer().getStatistics();
  Table       report;

  {
    const auto leaves  = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
    const auto local   = static_cast<unsigned long>(statistics.getGlobalNumberOfVertices());
    const auto hanging = static_cast<unsigned long>(statistics.getGlobalNumberOfHangingVertices());

    std::ostringstream vertices;
    vertices << Format::grouped(local + hanging) << "  (Local " << Format::grouped(local) << " · Hanging "
             << Format::grouped(hanging) << ")";
    std::ostringstream levels;
    levels << statistics.getGlobalMinDepth() << " .. " << statistics.getGlobalMaxDepth();

    report.add("Grid", "Number of Leaves", Format::grouped(leaves));
    report.add("Grid", "Number of Vertices", vertices.str());
    report.add("Grid", "Grid Levels", levels.str());
  }

  {
    const auto inner    = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
    const auto outer    = static_cast<unsigned long>(statistics.getGlobalNumberOfOuterUnrefinedCells());
    const auto trees    = static_cast<unsigned long>(statistics.getGlobalNumberOfTrees());
    const auto ranks    = static_cast<unsigned long>(statistics.getGlobalNumberOfRanksWithAtLeastOneTree());
    const auto minLoad  = static_cast<unsigned long>(statistics.getGlobalMinTreeLoad());
    const auto maxLoad  = static_cast<unsigned long>(statistics.getGlobalMaxTreeLoad());
    const auto minOuter = static_cast<unsigned long>(statistics.getGlobalMinTreeOuterLoad());
    const auto maxOuter = static_cast<unsigned long>(statistics.getGlobalMaxTreeOuterLoad());
    const auto minRank  = static_cast<unsigned long>(statistics.getGlobalMinRankLoad());
    const auto maxRank  = static_cast<unsigned long>(statistics.getGlobalMaxRankLoad());

    const auto efficiency     = (maxLoad > 0) ? (static_cast<double>(minLoad) / maxLoad) * 100.0 : 100.0;
    const auto rankEfficiency = (maxRank > 0) ? (static_cast<double>(minRank) / maxRank) * 100.0 : 100.0;
    const auto overhead       = (inner > 0) ? (static_cast<double>(outer) / inner) * 100.0 : 0.0;

    std::ostringstream treeText;
    treeText << Format::grouped(trees) << "  (on " << ranks << " Rank(s))";
    std::ostringstream efficiencyText;
    efficiencyText << Format::percentage(efficiency) << "  (" << Format::grouped(minLoad) << " .. "
                   << Format::grouped(maxLoad) << ")";
    std::ostringstream overheadText;
    overheadText << Format::percentage(overhead) << "  (" << Format::grouped(minOuter) << " .. "
                 << Format::grouped(maxOuter) << ")";

    report.add("Load Balancing", "Number of Trees", treeText.str());
    // Inter-rank balance (accumulated cells per rank) is only meaningful across more than one rank.
    if (ranks > 1) {
      std::ostringstream rankText;
      rankText << Format::percentage(rankEfficiency) << "  (" << Format::grouped(minRank) << " .. "
               << Format::grouped(maxRank) << ")";
      report.add("Load Balancing", "Rank Efficiency", rankText.str());
    }
    report.add("Load Balancing", "Global Efficiency", efficiencyText.str());
    report.add("Load Balancing", "Global Overhead", overheadText.str());
  }

  {
    const auto bytesFrom = [](const std::function<std::size_t()>& callback) -> unsigned long long {
      return callback ? callback() : 0;
    };
    const unsigned long long gridMemory   = statistics.getGlobalMemoryUsageInBytes();
    const unsigned long long cellMemory   = bytesFrom(repositories.getGlobalCellDataMemoryUsageInBytes);
    const unsigned long long faceMemory   = bytesFrom(repositories.getGlobalFaceDataMemoryUsageInBytes);
    const unsigned long long vertexMemory = bytesFrom(repositories.getGlobalVertexDataMemoryUsageInBytes);
    const unsigned long long dataTotal    = gridMemory + cellMemory + faceMemory + vertexMemory;
    const unsigned long long rss          = statistics.getGlobalProcessMemoryUsageInBytes();

    const unsigned long long totalRam = statistics.getGlobalTotalMemoryInBytes();
    const unsigned long long freeRam  = statistics.getGlobalFreeMemoryInBytes();
    const int                nodes    = tarch::mpi::Rank::getInstance().getNumberOfNodes();

    std::ostringstream system;
    system << Format::memory(totalRam) << " RAM · " << Format::memory(freeRam) << " free · " << Format::memory(rss)
           << " resident (RSS)";
    if (nodes > 1) {
      system << " · " << nodes << " nodes";
    }
    report.add("Memory", "System", system.str());
    report.add("Memory", "Grid", Format::memory(gridMemory));
    report.add(
      "Memory",
      "Heap",
      "Cell " + Format::memory(cellMemory) + " · Face " + Format::memory(faceMemory) + " · Vertex " +
        Format::memory(vertexMemory)
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
  // Mirror writeGridSnapshot's rows with worst-case values (counts and memory are int/GiB-bounded); the
  // node count is already known, so single-rank runs stay narrow.
  const auto        count = Format::grouped(std::numeric_limits<int>::max());
  const auto        mem   = Format::memory(9999ull * tarch::GiB);
  const std::string pct   = "100.00 %";
  const int         nodes = tarch::mpi::Rank::getInstance().getNumberOfNodes();

  Table report;
  report.add("Grid", "Number of Leaves", count);
  report.add("Grid", "Number of Vertices", count + "  (Local " + count + " · Hanging " + count + ")");
  report.add("Grid", "Grid Levels", "99 .. 99");
  report.add("Load Balancing", "Number of Trees", count + "  (on " + count + " Rank(s))");
  report.add("Load Balancing", "Rank Efficiency", pct + "  (" + count + " .. " + count + ")");
  report.add("Load Balancing", "Global Efficiency", pct + "  (" + count + " .. " + count + ")");
  report.add("Load Balancing", "Global Overhead", pct + "  (" + count + " .. " + count + ")");

  std::ostringstream system;
  system << mem << " RAM · " << mem << " free · " << mem << " resident (RSS)";
  if (nodes > 1) {
    system << " · " << nodes << " nodes";
  }
  report.add("Memory", "System", system.str());
  report.add("Memory", "Grid", mem);
  report.add("Memory", "Heap", "Cell " + mem + " · Face " + mem + " · Vertex " + mem);
  report.add("Memory", "Total Data", mem + " tracked");
  return report.render();
}
