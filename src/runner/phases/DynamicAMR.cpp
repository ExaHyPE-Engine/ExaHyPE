// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMR.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GlobalStatistics.h"
#include "runner/GridSnapshot.h"
#include "runner/Instrumentation.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Runtime.h"

runner::DynamicAMR::DynamicAMR(runner::PhaseContext& context, const int dynamicAMRStep) :
  _context(context),
  _dynamicAMRStep(dynamicAMRStep) {}

/** Reports the sweeps collected so far, if any. */
void runner::DynamicAMR::reportPendingSweeps() {
  if (_activity.sweeps > 0) {
    reportActivity();
  }
}

bool runner::DynamicAMR::isEnabled() const { return _dynamicAMRStep >= 0; }

bool runner::DynamicAMR::hasPendingMeshUpdates() const {
  return runner::DynamicAMR::resolvePendingMeshUpdates(_context.repositories);
}

/**
 * Returns whether the repository reports mesh updates that wait for a drain.
 * Without the callback nothing waits.
 */
bool runner::DynamicAMR::resolvePendingMeshUpdates(const runner::RepositoryCallbacks& repositories) {
  return repositories.hasPendingMeshUpdates and repositories.hasPendingMeshUpdates();
}

/**
 * Returns whether the last refinement criterion left its verdict open. A criterion skipped right
 * after a mesh change reports no requests without having judged the mesh, so a caller that adapts
 * until the criterion falls silent keeps going while this holds.
 */
bool runner::DynamicAMR::hasUndecidedRefinementCriterion() const {
  return _context.repositories.hasUndecidedRefinementCriterion
    and _context.repositories.hasUndecidedRefinementCriterion();
}

/**
 * Renders the summary of the drains since the previous report with the grid,
 * MPI and OpenMP state. Every arrow runs from the value before the first of
 * those drains to the value after the last one.
 */
std::vector<std::string> runner::DynamicAMR::describe(const runner::DynamicAMR::Activity& activity) {
  const auto transition = [](const std::string& before, const std::string& after) { return before + " → " + after; };
  const auto counts     = [&transition](const unsigned long before, const unsigned long after) {
    return transition(runner::Format::grouped(before), runner::Format::grouped(after));
  };
  const auto percentages = [&transition](const double before, const double after) {
    return transition(runner::Format::percentage(before), runner::Format::percentage(after));
  };
  const std::string indent(runner::BlockIndent, ' ');

  constexpr int LabelWidth = 8;
  const auto    label      = [&indent](const std::string& name) {
    std::ostringstream text;
    text << indent << std::left << std::setw(LabelWidth) << name;
    return text.str();
  };

  std::ostringstream summary;
  summary << indent << runner::PhaseName::DynamicAMR << "  " << runner::Format::wallClock(activity.wallClock) << " · "
          << runner::Format::grouped(static_cast<unsigned long>(activity.regrids))
          << (activity.regrids == 1 ? " Regrid" : " Regrids") << " · "
          << runner::Format::grouped(static_cast<unsigned long>(activity.sweeps))
          << (activity.sweeps == 1 ? " Sweep" : " Sweeps");

  std::ostringstream grid;
  grid << label("Grid") << "Leaves = " << counts(activity.leavesBefore, activity.leavesAfter)
       << " · Levels = " << activity.minimumLevel << " .. " << activity.maximumLevel;

  std::ostringstream tasks;
  tasks << label("OpenMP") << "Tasks = " << counts(activity.subtreeTasksBefore, activity.subtreeTasksAfter)
        << " · Cells/Task = " << runner::Format::grouped(activity.minimumTaskCells) << " .. "
        << runner::Format::grouped(activity.maximumTaskCells)
        << " · Utilisation = " << runner::Format::percentage(activity.threadUtilisation);

  // A single rank owns the whole grid.
  if (activity.numberOfRanks <= 1) {
    return {summary.str(), grid.str(), tasks.str()};
  }

  std::ostringstream mpiLoad;
  mpiLoad << label("MPI")
          << "Balance = " << percentages(activity.interRankBalanceBefore, activity.interRankBalanceAfter)
          << " · Leaves/Rank = " << runner::Format::grouped(activity.minimumRankLeaves) << " .. "
          << runner::Format::grouped(activity.maximumRankLeaves)
          << " · Overhead = " << runner::Format::overheadPercentage(activity.replicationOverhead);

  return {summary.str(), grid.str(), mpiLoad.str(), tasks.str()};
}

/** Returns the task time in seconds the adaptivity step accumulated over the run so far. */
double runner::DynamicAMR::accumulatedTaskSeconds() const {
  for (const auto& [stepName, taskSeconds] : _context.measurements.stepTaskSeconds) {
    if (stepName == runner::StepName::DynamicAMR) {
      return taskSeconds;
    }
  }
  return 0.0;
}

void runner::DynamicAMR::startActivity() {
  const auto& statistics           = runner::globalStatistics();
  _activity                        = runner::DynamicAMR::Activity{};
  _taskSecondsAtStart              = accumulatedTaskSeconds();
  _activity.leavesBefore           = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
  _activity.interRankBalanceBefore = statistics.getGlobalInterRankBalance();
  _activity.subtreeTasksBefore     = static_cast<unsigned long>(statistics.getGlobalNumberOfSubtreeTasks());
}

void runner::DynamicAMR::reportActivity() {
  const auto& statistics          = runner::globalStatistics();
  _activity.leavesAfter           = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
  _activity.minimumLevel          = statistics.getGlobalMinDepth();
  _activity.maximumLevel          = statistics.getGlobalMaxDepth();
  _activity.numberOfRanks         = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  _activity.interRankBalanceAfter = statistics.getGlobalInterRankBalance();
  _activity.minimumRankLeaves     = static_cast<unsigned long>(statistics.getGlobalMinRankLoad());
  _activity.maximumRankLeaves     = static_cast<unsigned long>(statistics.getGlobalMaxRankLoad());
  _activity.replicationOverhead   = statistics.getGlobalReplicationOverhead();
  _activity.subtreeTasksAfter     = static_cast<unsigned long>(statistics.getGlobalNumberOfSubtreeTasks());
  _activity.minimumTaskCells      = static_cast<unsigned long>(statistics.getGlobalMinSubtreeTaskCells());
  _activity.maximumTaskCells      = static_cast<unsigned long>(statistics.getGlobalMaxSubtreeTaskCells());
  // Cumulative task time makes the utilisation cover every sweep in the drain.
  const auto numberOfThreads  = tarch::tasking::Runtime::getInstance().getNumberOfThreads();
  _activity.threadUtilisation = _activity.wallClock > 0.0 and numberOfThreads > 1
    ? 100.0 * (accumulatedTaskSeconds() - _taskSecondsAtStart) / (numberOfThreads * _activity.wallClock)
    : 0.0;
  for (const auto& line : runner::DynamicAMR::describe(_activity)) {
    _context.box.line(line);
  }
  startActivity();
}

void runner::DynamicAMR::run() {
  SCOREP_USER_REGION(runner::PhaseName::DynamicAMR, SCOREP_USER_REGION_TYPE_PHASE)

  // Two quiet sweeps cover the one-sweep delay with which the global grid statistics
  // become visible under MPI.
  constexpr int RequiredStationarySweeps = 2;

  if (_activity.sweeps == 0) {
    startActivity();
  }

  // Both terms are rank-uniform, so every rank leaves the loop in the same sweep.
  const auto meshHasNotSettled = [this]() {
    return hasPendingMeshUpdates()
      or runner::globalStatistics().getGlobalNumberOfStationarySweeps() < RequiredStationarySweeps;
  };

  int    sweepsInThisDrain{0};
  double secondsSinceLastReport{0.0};
  while (meshHasNotSettled() and sweepsInThisDrain < runner::DynamicAMR::MaximumSweeps) {
    // A report inside the drain resets the activity, so the drain counts
    // itself on its first sweep.
    if (sweepsInThisDrain == 0) {
      _activity.regrids++;
    }
    const auto elapsedSeconds = _context.engine.runStep(_dynamicAMRStep);
    _activity.wallClock += elapsedSeconds;
    secondsSinceLastReport += elapsedSeconds;
    _context.measurements.dynamicAMR.setValue(elapsedSeconds);
    _activity.sweeps++;
    sweepsInThisDrain++;

    if (secondsSinceLastReport >= runner::DynamicAMR::SecondsBetweenReports) {
      reportActivity();
      secondsSinceLastReport = 0.0;
    }
  }

  if (hasPendingMeshUpdates()) {
    reportPendingSweeps();
    std::ostringstream message;
    message << "The dynamic AMR phase did not settle within " << runner::DynamicAMR::MaximumSweeps
            << " sweeps: The solvers still report pending mesh updates.";
    tarch::terminateWithError(message.str());
  }
}
