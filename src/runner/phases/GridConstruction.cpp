// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridConstruction.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace {
  std::size_t groupedWidth(unsigned long bound) {
    return runner::Format::grouped(std::min<unsigned long>(bound, std::numeric_limits<int>::max())).size();
  }

  // 3^(DIMENSIONS * maxLevel); aPowI() returns int, so cap the exponent (3^20 overflows int).
  unsigned long fullyRefinedLeaves(int maxLevel) {
    const int exponent = DIMENSIONS * maxLevel;
    return exponent <= 19 ? static_cast<unsigned long>(tarch::la::aPowI(exponent, 3))
                          : static_cast<unsigned long>(std::numeric_limits<int>::max());
  }
} // namespace

runner::GridConstruction::GridConstruction(PhaseContext& context, int createGridStep, int loadBalancingStep) :
  _context(context),
  _createGridStep(createGridStep),
  _loadBalancingStep(loadBalancingStep) {
  const int maxLevel = _context.parameters.getMaxGridLevels();

  // Size columns for the finest-level counts so streamed rows stay aligned.
  const auto leafBound    = fullyRefinedLeaves(maxLevel);
  const int  perRankTrees = _context.parameters.getMaxNumberOfTreesPerRank() > 0
                              ? _context.parameters.getMaxNumberOfTreesPerRank()
                              : tarch::multicore::Core::getInstance().getNumberOfThreads();
  const auto treeBound    = std::max(
    1UL,
    static_cast<unsigned long>(tarch::mpi::Rank::getInstance().getNumberOfRanks()) *
      static_cast<unsigned long>(std::max(1, perRankTrees))
  );

  const auto levelSample = Format::number(maxLevel) + " / " + Format::number(maxLevel);
  _widths.level          = static_cast<int>(std::max(levelSample.size(), std::string("Level").size()));
  _widths.leaves         = static_cast<int>(std::max(groupedWidth(leafBound), std::string("Leaves").size()));
  _widths.trees          = static_cast<int>(std::max(groupedWidth(treeBound), std::string("Trees").size()));
  _widths.efficiency     = static_cast<int>(std::max(std::string("Efficiency").size(), std::string("100.00 %").size()));
  _widths.overhead       = static_cast<int>(std::max(std::string("Overhead").size(), std::string("100.00 %").size()));
}

std::string runner::GridConstruction::row(
  const std::string& level,
  const std::string& leaves,
  const std::string& trees,
  const std::string& efficiency,
  const std::string& overhead,
  const std::string& load
) const {
  std::ostringstream out;
  out << std::string(2 * BlockIndent, ' ') << std::left << std::setw(_widths.level) << level << "  " << std::right
      << std::setw(_widths.leaves) << leaves << "  " << std::right << std::setw(_widths.trees) << trees << "  "
      << std::right << std::setw(_widths.efficiency) << efficiency << "  " << std::right << std::setw(_widths.overhead)
      << overhead << "   " << load;
  return out.str();
}

std::vector<std::string> runner::GridConstruction::widthSamples() const {
  // Widest row, so the box fits the data and not just the header.
  const std::string leavesFill(static_cast<std::size_t>(_widths.leaves), '9');
  const std::string treesFill(static_cast<std::size_t>(_widths.trees), '9');
  const std::string levelFill(static_cast<std::size_t>(_widths.level), '9');
  return {
    row("Level", "Leaves", "Trees", "Efficiency", "Overhead", "Load (min .. max)"),
    row(levelFill, leavesFill, treesFill, "100.00 %", "100.00 %", leavesFill + " .. " + leavesFill)
  };
}

void runner::GridConstruction::logRow(
  int           level,
  int           maxLevel,
  unsigned long leaves,
  unsigned long outer,
  unsigned long trees,
  unsigned long minLoad,
  unsigned long maxLoad
) {
  if (not _tableStarted) {
    _context.box.line(std::string(BlockIndent, ' ') + "Grid Construction");
    _context.box.line(row("Level", "Leaves", "Trees", "Efficiency", "Overhead", "Load (min .. max)"));
    _tableStarted = true;
  }

  const double efficiency = maxLoad > 0 ? static_cast<double>(minLoad) / maxLoad * 100.0 : 100.0;
  const double overhead   = leaves > 0 ? static_cast<double>(outer) / leaves * 100.0 : 0.0;
  const auto   load       = trees > 1 ? Format::grouped(minLoad) + " .. " + Format::grouped(maxLoad) : std::string();

  _context.box.line(row(
    Format::number(level) + " / " + Format::number(maxLevel),
    Format::grouped(leaves),
    Format::grouped(trees),
    Format::percentage(efficiency),
    Format::percentage(overhead),
    load
  ));
}

void runner::GridConstruction::run() {
  const double minAdmissibleH = _context.parameters.getMinAdmissiblePatchSize(_context.domainSize);
  const int    maxLevel       = _context.parameters.getMaxGridLevels();

  double currentMinH         = std::numeric_limits<double>::max();
  int    currentTrees        = 0;   // tree count we have already reacted to
  double constructionSeconds = 0.0; // wall time summed over all construction sweeps

  // Each level gets exactly one row, showing its converged state. Rows are
  // therefore buffered: every settled sweep overwrites the snapshot of the
  // current level, and the snapshot is emitted once the construction has
  // refined past the level (or converged on the final one).
  struct LevelSnapshot {
    int           level = -1;
    unsigned long leaves{};
    unsigned long outer{};
    unsigned long trees{};
    unsigned long minLoad{};
    unsigned long maxLoad{};
  };
  LevelSnapshot pendingRow;

  // The grid is built by alternating refining sweeps (CreateGrid) with refinement-paused settle
  // sweeps (LoadBalancing): after any sweep that changed the grid - a refinement (minH dropped) or a
  // split (tree count rose) - the next sweep refines nothing, so the load balancer decides on and
  // deploys its splits while the grid is static. Without this the grid would refine underneath an
  // in-flight split and the cell-count request would be stale on arrival.
  bool refine                       = true; // the first sweep refines
  bool finishConditionSeenLastSweep = false;

  while (true) {
    const bool didRefineThisSweep = refine;
    if (refine) {
      SCOREP_USER_REGION(PhaseName::GridConstruction, SCOREP_USER_REGION_TYPE_COMMON)
      constructionSeconds += _context.engine.runStep(_createGridStep);
    } else {
      SCOREP_USER_REGION(PhaseName::LoadBalancing, SCOREP_USER_REGION_TYPE_COMMON)
      constructionSeconds += _context.engine.runStep(_loadBalancingStep);
    }

    const auto& s     = loadBalancer().getStatistics();
    const auto  trees = static_cast<int>(s.getGlobalNumberOfTrees());

    // A sweep's statistics are trustworthy only when no fork started this
    // sweep (tree count stable) and none is still settling.
    const bool cleanSweep = (trees == currentTrees) and s.getGlobalNumberOfStationarySweeps() >= 0;
    if (cleanSweep) {
      const int level = s.getGlobalMaxDepth();
      if (pendingRow.level >= 0 and level > pendingRow.level) {
        logRow(
          pendingRow.level,
          maxLevel,
          pendingRow.leaves,
          pendingRow.outer,
          pendingRow.trees,
          pendingRow.minLoad,
          pendingRow.maxLoad
        );
      }
      if (level >= pendingRow.level) {
        pendingRow.level   = level;
        pendingRow.leaves  = static_cast<unsigned long>(s.getGlobalNumberOfInnerUnrefinedCells());
        pendingRow.outer   = static_cast<unsigned long>(s.getGlobalNumberOfOuterUnrefinedCells());
        pendingRow.trees   = static_cast<unsigned long>(trees);
        pendingRow.minLoad = static_cast<unsigned long>(s.getGlobalMinTreeLoad());
        pendingRow.maxLoad = static_cast<unsigned long>(s.getGlobalMaxTreeLoad());
      }
    }

    if (tarch::la::smaller(s.getGlobalMinH(), currentMinH)) {
      currentMinH                  = s.getGlobalMinH(); // refined: settle, let the grid take effect
      finishConditionSeenLastSweep = false;
      refine                       = false;
    } else if (trees > currentTrees) {
      currentTrees                 = trees; // split: settle, let the load balancer deploy/settle it
      finishConditionSeenLastSweep = false;
      refine                       = false;
    } else {
      // Stationary. Converged once it is admissibly fine, the load balancer has settled, and the sweep
      // that just ran was a refining one. Require one confirmed stationary refining sweep before stop.
      const bool converged =
        tarch::la::smallerEquals(currentMinH, minAdmissibleH) and loadBalancer().isStagnating() and didRefineThisSweep;
      if (converged and finishConditionSeenLastSweep) {
        if (pendingRow.level >= 0) {
          logRow(
            pendingRow.level,
            maxLevel,
            pendingRow.leaves,
            pendingRow.outer,
            pendingRow.trees,
            pendingRow.minLoad,
            pendingRow.maxLoad
          );
        }
        break;
      }
      finishConditionSeenLastSweep = converged;
      refine                       = true;
    }
  }

  _context.box.blank();
  _context.box.line(std::string(BlockIndent, ' ') + "Grid Constructed " + Format::wallClock(constructionSeconds));
}
