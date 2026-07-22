// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DynamicAMR.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GridSnapshot.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "tarch/Termination.h"

runner::DynamicAMR::DynamicAMR(PhaseContext& context, int dynamicAMRStep) :
  _context(context),
  _dynamicAMRStep(dynamicAMRStep) {}

bool runner::DynamicAMR::isEnabled() const { return _dynamicAMRStep >= 0; }

bool runner::DynamicAMR::hasPendingMeshUpdates() const {
  return _context.repositories.hasPendingMeshUpdates and _context.repositories.hasPendingMeshUpdates();
}

std::string runner::DynamicAMR::describe(const Activity& activity) {
  std::ostringstream leaves;
  if (activity.leavesBefore == activity.leavesAfter) {
    leaves << Format::grouped(activity.leavesAfter);
  } else {
    leaves << Format::grouped(activity.leavesBefore) << " → " << Format::grouped(activity.leavesAfter);
  }

  std::ostringstream line;
  line << std::string(BlockIndent, ' ') << PhaseName::DynamicAMR << "  " << Format::wallClock(activity.wallClock)
       << " · Leaves = " << leaves.str() << " · Levels = " << activity.minLevel << " .. " << activity.maxLevel;
  return line.str();
}

std::vector<std::string> runner::DynamicAMR::widthSamples() const {
  const auto leaves = boundedFullyRefinedLeaves(_context.parameters.getMaxGridLevels());

  Activity worstCase;
  worstCase.wallClock = 9999.99;
  // Neither a power of three nor the saturated maximum is a power of ten, so one less than
  // the leaf count renders at the same width and the sample bounds both columns.
  worstCase.leavesBefore = leaves;
  worstCase.leavesAfter  = leaves - 1;
  worstCase.minLevel     = 99;
  worstCase.maxLevel     = 99;
  return {describe(worstCase)};
}

void runner::DynamicAMR::startActivity() {
  _activity = Activity{};
  _activity.leavesBefore =
    static_cast<unsigned long>(loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
}

void runner::DynamicAMR::reportActivity() {
  const auto& statistics = loadBalancer().getStatistics();
  _activity.leavesAfter  = static_cast<unsigned long>(statistics.getGlobalNumberOfInnerUnrefinedCells());
  _activity.minLevel     = statistics.getGlobalMinDepth();
  _activity.maxLevel     = statistics.getGlobalMaxDepth();
  // Plain line: it stacks with the plots and stays outside the time step's block.
  _context.box.line(describe(_activity));
  startActivity();
}

void runner::DynamicAMR::reportPendingSweeps() {
  if (_activity.sweeps > 0) {
    reportActivity();
  }
}

void runner::DynamicAMR::run() {
  SCOREP_USER_REGION(PhaseName::DynamicAMR, SCOREP_USER_REGION_TYPE_PHASE)

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
      or loadBalancer().getStatistics().getGlobalNumberOfStationarySweeps() < RequiredStationarySweeps;
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
