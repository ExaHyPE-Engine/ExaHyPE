// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridConstruction.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GlobalStatistics.h"
#include "runner/GridSnapshot.h"
#include "runner/Instrumentation.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/instrumentation/Watch.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Rank.h"

namespace {
  std::size_t groupedWidth(const std::uint64_t bound) { return runner::Format::grouped(bound).size(); }
} // namespace

runner::GridConstruction::GridConstruction(runner::PhaseContext& context, const int createGridStep) :
  _context(context),
  _createGridStep(createGridStep) {
  const auto maximumLevel = _context.parameters.getMaxGridLevels();
  const auto leafBound    = runner::boundedFullyRefinedLeaves(maximumLevel);

  const auto levelSample = runner::Format::number(maximumLevel) + " / " + runner::Format::number(maximumLevel);
  _widths.level          = static_cast<int>(std::max(levelSample.size(), std::string("Level").size()));
  _widths.leaves         = static_cast<int>(std::max(groupedWidth(leafBound), std::string("Leaves").size()));
  _widths.efficiency     = static_cast<int>(std::max(std::string("Balance").size(), std::string("100.00 %").size()));
  _widths.overhead =
    static_cast<int>(std::max(std::string("Spine").size(), runner::Format::overheadPercentageWidthSample().size()));
}

std::string runner::GridConstruction::row(
  const std::string& level,
  const std::string& leaves,
  const std::string& efficiency,
  const std::string& overhead,
  const std::string& load
) const {
  std::ostringstream result;
  result << std::string(2 * runner::BlockIndent, ' ') << std::left << std::setw(_widths.level) << level << "  "
         << std::right << std::setw(_widths.leaves) << leaves << "  " << std::setw(_widths.efficiency) << efficiency
         << "  " << std::setw(_widths.overhead) << overhead << "   " << load;
  return result.str();
}

void runner::GridConstruction::logRow(
  const int           level,
  const int           maximumLevel,
  const std::uint64_t numberOfLeaves,
  const double        replicationOverhead,
  const std::uint64_t minimumRankLoad,
  const std::uint64_t maximumRankLoad
) {
  if (not _tableStarted) {
    _context.box.line(std::string(runner::BlockIndent, ' ') + "Grid Construction");
    _context.box.line(row("Level", "Leaves", "Balance", "Spine", "Load (min .. max)"));
    _tableStarted = true;
  }

  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const auto balance       = maximumRankLoad > 0 and numberOfRanks > 1
    ? static_cast<double>(numberOfLeaves) / (numberOfRanks * static_cast<double>(maximumRankLoad)) * 100.0
    : 100.0;
  const auto load = runner::Format::grouped(minimumRankLoad) + " .. " + runner::Format::grouped(maximumRankLoad);

  _context.box.line(row(
    runner::Format::number(level) + " / " + runner::Format::number(maximumLevel),
    runner::Format::grouped(numberOfLeaves),
    runner::Format::percentage(balance),
    runner::Format::overheadPercentage(replicationOverhead),
    load
  ));
}

/**
 * The first sweep executes the scheduled construction plan and builds the
 * complete base grid; further sweeps only run while the refinement criterion
 * asks for adaptive levels beyond it. One row summarises each reached depth.
 */
void runner::GridConstruction::run() {
  tarch::instrumentation::Watch constructionWatch("runner::GridConstruction", "run", false, false);
  constructionWatch.start();

  // The coarsest admissible patch is the resolution the grid has to reach before it may stop. The
  // finest level is a cap on what the refinement criterion may ask for: A scenario that seeds
  // nothing converges on the base grid alone.
  const auto  coarsestAdmissiblePatchSize = _context.parameters.getMaxAdmissiblePatchSize(_context.domainSize);
  const auto  maximumLevel                = _context.parameters.getMaxGridLevels();
  const auto& statistics                  = runner::globalStatistics();

  auto currentMinimumPatchSize = std::numeric_limits<double>::max();

  struct LevelSnapshot {
    int           level{-1};
    std::uint64_t numberOfLeaves{};
    double        replicationOverhead{};
    std::uint64_t minimumRankLoad{};
    std::uint64_t maximumRankLoad{};
  };
  LevelSnapshot pendingRow;

  bool finishConditionSeenLastSweep = false;

  while (true) {
    {
      SCOREP_USER_REGION(runner::PhaseName::GridConstruction, SCOREP_USER_REGION_TYPE_COMMON)
      static_cast<void>(_context.engine.runStep(_createGridStep));
    }
    const auto level = statistics.getGlobalMaxDepth();
    if (pendingRow.level >= 0 and level > pendingRow.level) {
      logRow(
        pendingRow.level,
        maximumLevel,
        pendingRow.numberOfLeaves,
        pendingRow.replicationOverhead,
        pendingRow.minimumRankLoad,
        pendingRow.maximumRankLoad
      );
    }
    if (level >= pendingRow.level) {
      pendingRow.level               = level;
      pendingRow.numberOfLeaves      = statistics.getGlobalNumberOfInnerUnrefinedCells();
      pendingRow.replicationOverhead = statistics.getGlobalReplicationOverhead();
      pendingRow.minimumRankLoad     = statistics.getGlobalMinRankLoad();
      pendingRow.maximumRankLoad     = statistics.getGlobalMaxRankLoad();
    }

    if (tarch::la::smaller(statistics.getGlobalMinH(), currentMinimumPatchSize)) {
      currentMinimumPatchSize      = statistics.getGlobalMinH();
      finishConditionSeenLastSweep = false;
    }

    // The grid converges once the finest cell reaches the coarsest admissible resolution and the
    // stationary-sweep counter, which resets on every grid change, shows that the criterion asks
    // for nothing further. The predicate has to hold on two consecutive sweeps.
    const bool converged = tarch::la::smallerEquals(currentMinimumPatchSize, coarsestAdmissiblePatchSize)
      and statistics.getGlobalNumberOfStationarySweeps() >= 1;
    if (converged and finishConditionSeenLastSweep) {
      if (pendingRow.level >= 0) {
        logRow(
          pendingRow.level,
          maximumLevel,
          pendingRow.numberOfLeaves,
          pendingRow.replicationOverhead,
          pendingRow.minimumRankLoad,
          pendingRow.maximumRankLoad
        );
      }
      break;
    }
    finishConditionSeenLastSweep = converged;
  }

  constructionWatch.stop();

  _context.box.blank();
  _context.box.line(
    std::string(runner::BlockIndent, ' ') + "Grid Constructed "
    + runner::Format::wallClock(constructionWatch.getCalendarTime())
  );
}
