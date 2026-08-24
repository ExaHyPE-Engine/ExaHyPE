// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMR.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "tarch/Termination.h"

runner::DynamicAMR::DynamicAMR(runner::PhaseContext& context, const int dynamicAMRStep) :
  _context(context),
  _dynamicAMRStep(dynamicAMRStep) {}

bool runner::DynamicAMR::isEnabled() const { return _dynamicAMRStep >= 0; }

bool runner::DynamicAMR::hasPendingMeshUpdates() const {
  return _context.repositories.hasPendingMeshUpdates and _context.repositories.hasPendingMeshUpdates();
}

/**
 * Returns whether the last refinement criterion left its verdict open.
 *
 * A criterion that is skipped right after a mesh change reports no requests
 * without having judged the mesh. A caller that adapts until the criterion falls
 * silent has to keep going while this holds, or it settles for a mesh the
 * criterion never approved.
 */
bool runner::DynamicAMR::hasUndecidedRefinementCriterion() const {
  return _context.repositories.hasUndecidedRefinementCriterion
     and _context.repositories.hasUndecidedRefinementCriterion();
}

std::string runner::DynamicAMR::describe(const runner::DynamicAMR::Activity& activity) {
  std::ostringstream leaves;
  if (activity.leavesBefore == activity.leavesAfter) {
    leaves << runner::Format::grouped(activity.leavesAfter);
  } else {
    leaves << runner::Format::grouped(activity.leavesBefore) << " → " << runner::Format::grouped(activity.leavesAfter);
  }

  std::ostringstream line;
  line << std::string(runner::BlockIndent, ' ') << runner::PhaseName::DynamicAMR << "  "
       << runner::Format::wallClock(activity.wallClock) << " · Leaves = " << leaves.str()
       << " · Levels = " << activity.minLevel << " .. " << activity.maxLevel;
  return line.str();
}

void runner::DynamicAMR::startActivity() {
  _activity = runner::DynamicAMR::Activity{};
  _activity.leavesBefore =
    static_cast<unsigned long>(runner::loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
}

void runner::DynamicAMR::reportActivity() {
  const auto& statistics = runner::loadBalancer().getStatistics();
  _activity.leavesAfter  = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
  _activity.minLevel     = statistics.getGlobalMinDepth();
  _activity.maxLevel     = statistics.getGlobalMaxDepth();
  // Plain line: it stacks with the plots and stays outside the time step's block.
  _context.box.line(runner::DynamicAMR::describe(_activity));
  runner::DynamicAMR::startActivity();
}

void runner::DynamicAMR::reportPendingSweeps() {
  if (_activity.sweeps > 0) {
    runner::DynamicAMR::reportActivity();
  }
}

void runner::DynamicAMR::run() {
  SCOREP_USER_REGION(runner::PhaseName::DynamicAMR, SCOREP_USER_REGION_TYPE_PHASE)

  // Two quiet sweeps cover the one-sweep publication lag of the global grid statistics
  // under MPI.
  constexpr int RequiredStationarySweeps = 2;

  if (_activity.sweeps == 0) {
    startActivity();
  }

  // Both terms are rank-uniform and cheap to re-evaluate, so the abort below can ask the
  // mesh itself whether it settled.
  const auto meshHasNotSettled = [this]() {
    return hasPendingMeshUpdates()
      or runner::loadBalancer().getStatistics().getGlobalNumberOfStationarySweeps() < RequiredStationarySweeps;
  };

  int sweeps = 0;
  while (meshHasNotSettled() and sweeps < MaxSweeps) {
    const double elapsedSeconds = _context.engine.runStep(_dynamicAMRStep);
    _activity.wallClock += elapsedSeconds;
    _context.measurements.dynamicAMR.setValue(elapsedSeconds);
    _activity.sweeps++;
    sweeps++;

    // A slow drain writes a line every so many sweeps, which is what tells it from a hang.
    if (sweeps % SweepsPerReport == 0) {
      reportActivity();
    }
  }

  if (meshHasNotSettled()) {
    reportPendingSweeps();
    const auto message = "The dynamic AMR phase did not settle within " + std::to_string(MaxSweeps)
      + " sweeps. The grid cannot realise the committed refinement/erase events.";
    tarch::terminateWithError(message);
  }
}
