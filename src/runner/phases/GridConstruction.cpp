// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridConstruction.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GridSnapshot.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Termination.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

namespace {
  std::size_t groupedWidth(const std::uint64_t bound) { return runner::Format::grouped(bound).size(); }
} // namespace

runner::GridConstruction::GridConstruction(
  runner::PhaseContext& context,
  const int             createGridStep,
  const int             loadBalancingStep
) :
  _context(context),
  _createGridStep(createGridStep),
  _loadBalancingStep(loadBalancingStep) {
  const auto maximumLevel = _context.parameters.getMaxGridLevels();

  const auto leafBound              = runner::boundedFullyRefinedLeaves(maximumLevel);
  const auto configuredTreesPerRank = _context.parameters.getMaxNumberOfTreesPerRank();
  const auto maximumTreesPerRank =
    configuredTreesPerRank > 0 ? configuredTreesPerRank : tarch::multicore::Core::getInstance().getNumberOfThreads();
  const auto treeBound = std::max(
    1UL,
    static_cast<unsigned long>(tarch::mpi::Rank::getInstance().getNumberOfRanks())
      * static_cast<unsigned long>(std::max(1, maximumTreesPerRank))
  );

  const auto levelSample = runner::Format::number(maximumLevel) + " / " + runner::Format::number(maximumLevel);
  _widths.level          = static_cast<int>(std::max(levelSample.size(), std::string("Level").size()));
  _widths.leaves         = static_cast<int>(std::max(groupedWidth(leafBound), std::string("Leaves").size()));
  _widths.trees          = static_cast<int>(std::max(groupedWidth(treeBound), std::string("Trees").size()));
  _widths.efficiency     = static_cast<int>(std::max(std::string("Efficiency").size(), std::string("100.00 %").size()));
  _widths.overhead =
    static_cast<int>(std::max(std::string("Overhead").size(), runner::Format::overheadPercentageWidthSample().size()));
}

std::string runner::GridConstruction::row(
  const std::string& level,
  const std::string& leaves,
  const std::string& trees,
  const std::string& efficiency,
  const std::string& overhead,
  const std::string& load
) const {
  std::ostringstream result;
  result << std::string(2 * runner::BlockIndent, ' ') << std::left << std::setw(_widths.level) << level << "  "
         << std::right << std::setw(_widths.leaves) << leaves << "  " << std::setw(_widths.trees) << trees << "  "
         << std::setw(_widths.efficiency) << efficiency << "  " << std::setw(_widths.overhead) << overhead << "   "
         << load;
  return result.str();
}

void runner::GridConstruction::logRow(
  const int           level,
  const int           maximumLevel,
  const std::uint64_t numberOfLeaves,
  const std::uint64_t numberOfOuterCells,
  const unsigned long numberOfTrees,
  const unsigned long minimumTreeLoad,
  const unsigned long maximumTreeLoad
) {
  if (not _tableStarted) {
    _context.box.line(std::string(runner::BlockIndent, ' ') + "Grid Construction");
    _context.box.line(row("Level", "Leaves", "Trees", "Efficiency", "Overhead", "Load (min .. max)"));
    _tableStarted = true;
  }

  const auto efficiency = maximumTreeLoad > 0 ? static_cast<double>(minimumTreeLoad) / maximumTreeLoad * 100.0 : 100.0;
  const auto overhead   = numberOfLeaves > 0 ? static_cast<double>(numberOfOuterCells) / numberOfLeaves * 100.0 : 0.0;
  const auto load       = runner::Format::grouped(minimumTreeLoad) + " .. " + runner::Format::grouped(maximumTreeLoad);

  _context.box.line(row(
    runner::Format::number(level) + " / " + runner::Format::number(maximumLevel),
    runner::Format::grouped(numberOfLeaves),
    runner::Format::grouped(numberOfTrees),
    runner::Format::percentage(efficiency),
    runner::Format::overheadPercentage(overhead),
    load
  ));
}

/**
 * Refinement passes through levels that cannot support the requested
 * decomposition and holds a level only while a fork can start or remains
 * active. Two converged refinement sweeps complete the handover. Forked trees
 * can continue removing transient replicas during solver traversals, so
 * snapshot overhead can decrease during time stepping.
 */
void runner::GridConstruction::run() {
  tarch::instrumentation::Watch constructionWatch("runner::GridConstruction", "run", false, false);
  constructionWatch.start();

  // The coarsest admissible patch is the resolution the grid has to reach before it may stop. The
  // finest level is a cap the refinement criterion may ask for, not a target the construction has
  // to hit: a scenario that seeds nothing has to converge on the coarsest level alone.
  const auto coarsestAdmissiblePatchSize = _context.parameters.getMaxAdmissiblePatchSize(_context.domainSize);
  const auto maximumLevel                = _context.parameters.getMaxGridLevels();
  auto&      loadBalancerInstance        = runner::loadBalancer();

  auto currentMinimumPatchSize = std::numeric_limits<double>::max();
  int  currentNumberOfTrees    = 0;

  struct LevelSnapshot {
    int           level = -1;
    std::uint64_t numberOfLeaves{};
    std::uint64_t numberOfOuterCells{};
    unsigned long numberOfTrees{};
    unsigned long minimumTreeLoad{};
    unsigned long maximumTreeLoad{};
  };
  LevelSnapshot pendingRow;

  // Fork requests need a stationary mesh until their multi-sweep protocol completes.
  bool refine                       = true;
  bool finishConditionSeenLastSweep = false;

  while (true) {
    const bool didRefineThisSweep = refine;
    if (refine) {
      SCOREP_USER_REGION(runner::PhaseName::GridConstruction, SCOREP_USER_REGION_TYPE_COMMON)
      static_cast<void>(_context.engine.runStep(_createGridStep));
    } else {
      SCOREP_USER_REGION(runner::PhaseName::LoadBalancing, SCOREP_USER_REGION_TYPE_COMMON)
      static_cast<void>(_context.engine.runStep(_loadBalancingStep));
    }

    const auto& statistics    = loadBalancerInstance.getStatistics();
    const auto  numberOfTrees = static_cast<int>(statistics.getGlobalNumberOfTrees());

    // Hanging vertices prevent faithful forks after adaptive refinement starts, so the load
    // balancer finalises the initial decomposition with the last uniform cells. Completion tests
    // the coarsest leaf through the minimum depth: the minimum mesh width tracks the finest cell,
    // which reaches the floor while coarser regions still refine towards it.
    if (statistics.getGlobalNumberOfInnerUnrefinedCells() > 0
        and statistics.getGlobalMinDepth() >= _context.parameters.getGridLevels()) {
      loadBalancerInstance.notifyUniformGridComplete();
    }

    // A clean sweep snapshots a row, which is emitted after refinement advances so late split statistics
    // are included once.
    const bool cleanSweep =
      numberOfTrees == currentNumberOfTrees and statistics.getGlobalNumberOfStationarySweeps() >= 0;
    if (cleanSweep) {
      const auto level = statistics.getGlobalMaxDepth();
      if (pendingRow.level >= 0 and level > pendingRow.level) {
        logRow(
          pendingRow.level,
          maximumLevel,
          pendingRow.numberOfLeaves,
          pendingRow.numberOfOuterCells,
          pendingRow.numberOfTrees,
          pendingRow.minimumTreeLoad,
          pendingRow.maximumTreeLoad
        );
      }
      if (level >= pendingRow.level) {
        pendingRow.level              = level;
        pendingRow.numberOfLeaves     = statistics.getGlobalNumberOfInnerUnrefinedCells();
        pendingRow.numberOfOuterCells = statistics.getGlobalNumberOfOuterUnrefinedCells();
        pendingRow.numberOfTrees      = static_cast<unsigned long>(numberOfTrees);
        pendingRow.minimumTreeLoad    = static_cast<unsigned long>(statistics.getGlobalMinTreeLoad());
        pendingRow.maximumTreeLoad    = static_cast<unsigned long>(statistics.getGlobalMaxTreeLoad());
      }
    }

    if (tarch::la::smaller(statistics.getGlobalMinH(), currentMinimumPatchSize)) {
      currentMinimumPatchSize      = statistics.getGlobalMinH();
      finishConditionSeenLastSweep = false;
    }
    if (numberOfTrees > currentNumberOfTrees) {
      currentNumberOfTrees         = numberOfTrees;
      finishConditionSeenLastSweep = false;
    }

    if (loadBalancerInstance.needsGridToStayStationary()) {
      finishConditionSeenLastSweep = false;
      refine                       = false;
    } else {
      // Converged once the finest cell has reached the coarsest admissible resolution, the criterion
      // has stopped asking for more, the load balancer has settled, and the sweep that just ran was
      // a refining one. The first term alone only says some cell got there; the spacetree clears its
      // stationary counter on every triggered refinement, so the counter is what separates "the
      // criterion wants nothing further" from "the finest cell happens to have stopped shrinking
      // while other cells still refine". The whole predicate has to hold on two consecutive sweeps.
      const bool converged = tarch::la::smallerEquals(currentMinimumPatchSize, coarsestAdmissiblePatchSize)
        and statistics.getGlobalNumberOfStationarySweeps() >= 1 and loadBalancerInstance.isStagnating()
        and didRefineThisSweep;
      if (converged and finishConditionSeenLastSweep) {
        if (pendingRow.level >= 0) {
          logRow(
            pendingRow.level,
            maximumLevel,
            pendingRow.numberOfLeaves,
            pendingRow.numberOfOuterCells,
            pendingRow.numberOfTrees,
            pendingRow.minimumTreeLoad,
            pendingRow.maximumTreeLoad
          );
        }
        break;
      }
      finishConditionSeenLastSweep = converged;
      refine                       = true;
    }
  }

  const auto finalNumberOfTrees      = static_cast<int>(loadBalancerInstance.getStatistics().getGlobalNumberOfTrees());
  const auto configuredNumberOfTrees = loadBalancerInstance.getConfiguredNumberOfTrees();
  if (finalNumberOfTrees != configuredNumberOfTrees) {
    std::ostringstream errorMessage;
    errorMessage << "Grid construction finished with " << finalNumberOfTrees << " spacetrees, but "
                 << configuredNumberOfTrees
                 << " are configured. Reduce the number of trees per rank or refine the grid further so that every "
                    "tree receives cells.";
    tarch::terminateWithError(errorMessage.str());
  }
  constructionWatch.stop();

  _context.box.blank();
  _context.box.line(
    std::string(runner::BlockIndent, ' ') + "Grid Constructed "
    + runner::Format::wallClock(constructionWatch.getCalendarTime())
  );
}
