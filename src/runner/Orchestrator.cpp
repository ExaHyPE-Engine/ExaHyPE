// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Orchestrator.h"

#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/la/ScalarOperations.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace {
  // Right-hand progress string for a time-stepping box:
  // "t = 1.31e-02 / 2.00e-01   [####------] 41%   ETA 0h 2m 13s". The time stamps are compact
  // scientific so the header is bounded and always fits the box.
  std::string timeProgress(double timeStamp, double terminalTime, const std::string& eta) {
    std::ostringstream progress;
    progress << std::scientific << std::setprecision(2) << "t = " << timeStamp << " / " << terminalTime;
    if (terminalTime > 0.0 and terminalTime < std::numeric_limits<double>::max() / 10.0) {
      const double fraction = timeStamp / terminalTime;
      progress << "   " << runner::formatProgressBar(fraction) << " " << static_cast<int>(fraction * 100.0 + 0.5)
               << "%";
    }
    if (not eta.empty()) {
      progress << "   " << eta;
    }
    return progress.str();
  }

  // Right-hand progress string for a grid-construction box: "Level 3 / 4   [####------]".
  std::string levelProgress(int currentLevel, int maxLevel) {
    std::ostringstream progress;
    progress << "Level " << currentLevel << " / " << maxLevel;
    if (maxLevel > 0) {
      progress << "   " << runner::formatProgressBar(static_cast<double>(currentLevel) / maxLevel);
    }
    return progress.str();
  }
} // namespace

runner::Orchestrator::Orchestrator(const std::vector<Step>& steps) {
  for (const auto& step : steps) {
    if (step.name == "CreateGrid") {
      _createGrid = {step.programStep, step.name};
    } else if (step.name == "LoadBalancing") {
      _loadBalancing = {step.programStep, step.name};
    } else if (step.name == "InitGrid") {
      _initGrid = {step.programStep, step.name};
    } else if (step.name == "PlotSolution") {
      _plot = {step.programStep, step.name};
    } else if (step.name == "TimeStep") {
      _timeStep = {step.programStep, step.name};
    }
  }
}

bool runner::Orchestrator::run(Runner& runner, RunContext& context) {
  if (_createGrid.exists()) {
    runGridConstruction(runner, context);
  }
  if (_initGrid.exists()) {
    runGridInitialisation(runner, context);
  }
  if (_timeStep.exists()) {
    return runTimeLoop(runner, context);
  }
  return false;
}

void runner::Orchestrator::runGridConstruction(Runner& runner, RunContext& context) {
  SCOREP_USER_REGION("GridConstruction", SCOREP_USER_REGION_TYPE_PHASE)

  const double minAdmissibleH = context.parameters.getMinAdmissiblePatchSize(context.domainSize);
  const int    maxLevel       = context.parameters.getMaxGridLevels();
  auto&        statistics     = context.statisticsLog;

  double currentMinH    = std::numeric_limits<double>::max();
  int    currentTrees   = 0;   // tree count we have already reacted to
  int    lastShownCells = -1;  // cells in the last box, so a box is emitted only when the grid changed
  double lastDt         = 0.0; // wall time of the most recent sweep, shown in the box header

  // The grid is built by alternating refining sweeps (CreateGrid) with refinement-paused settle
  // sweeps (LoadBalancing): after any sweep that changed the grid - a refinement (minH dropped) or
  // a split (tree count rose) - the next sweep refines nothing, so the load balancer decides on and
  // deploys its splits while the grid is static. Without this the grid would refine underneath an
  // in-flight split and the cell-count request would be stale on arrival.
  bool refine                       = true; // the first sweep refines
  bool finishConditionSeenLastSweep = false;

  while (true) {
    const bool didRefineThisSweep = refine;
    if (refine) {
      // A refining sweep: the CreateGrid step drives the actual mesh refinement.
      SCOREP_USER_REGION("CreateGrid", SCOREP_USER_REGION_TYPE_COMMON)
      lastDt = runner.runStep(_createGrid.programStep);
    } else {
      // A refinement-paused settle sweep: the LoadBalancing step lets the load balancer decide on
      // and deploy its splits while the grid is held static.
      SCOREP_USER_REGION("LoadBalancing", SCOREP_USER_REGION_TYPE_COMMON)
      lastDt = runner.runStep(_loadBalancing.programStep);
    }

    const auto& s     = loadBalancer().getStatistics();
    const int   trees = static_cast<int>(s.getGlobalNumberOfTrees());

    // Show each settled intermediate level once. A sweep's cell count is trustworthy only when no
    // fork started this sweep (tree count stable) and none is still settling (non-negative
    // stationary sweeps); the final level is left to the converged box below.
    const bool cleanSweep = (trees == currentTrees) and s.getGlobalNumberOfStationarySweeps() >= 0;
    if (cleanSweep and tarch::la::greater(s.getGlobalMinH(), minAdmissibleH) and
        s.getGlobalNumberOfInnerUnrefinedCells() != lastShownCells) {
      lastShownCells = s.getGlobalNumberOfInnerUnrefinedCells();
      statistics.emitSimulationBox(
        runner::displayLabel(_createGrid.name),
        lastDt,
        levelProgress(s.getGlobalMaxDepth(), maxLevel),
        false
      );
    }

    // Decide the next sweep
    if (tarch::la::smaller(s.getGlobalMinH(), currentMinH)) {
      currentMinH                  = s.getGlobalMinH(); // refined: settle, let the grid take effect
      finishConditionSeenLastSweep = false;
      refine                       = false;
    } else if (trees > currentTrees) {
      currentTrees                 = trees; // split: settle, let the load balancer deploy/settle it
      finishConditionSeenLastSweep = false;
      refine                       = false;
    } else {
      // The grid is stationary. It has converged once it is admissibly fine, the load balancer has
      // settled, and the sweep that just ran was a refining one (so the mesh had a chance to refine
      // further). Require one confirmed stationary refining sweep before stopping.
      const bool converged =
        tarch::la::smallerEquals(currentMinH, minAdmissibleH) and loadBalancer().isStagnating() and didRefineThisSweep;
      if (converged and finishConditionSeenLastSweep) {
        if (s.getGlobalNumberOfInnerUnrefinedCells() != lastShownCells) {
          statistics.emitSimulationBox(
            runner::displayLabel(_createGrid.name),
            lastDt,
            levelProgress(s.getGlobalMaxDepth(), maxLevel),
            false
          );
        }
        break;
      }
      finishConditionSeenLastSweep = converged;
      refine                       = true;
    }
  }
}

void runner::Orchestrator::runGridInitialisation(Runner& runner, RunContext& context) {
  SCOREP_USER_REGION("InitGrid", SCOREP_USER_REGION_TYPE_PHASE)

  const double dt = runner.runStep(_initGrid.programStep);
  context.statisticsLog.emitSimulationBox(runner::displayLabel(_initGrid.name), dt, "", false);
}

bool runner::Orchestrator::runTimeLoop(Runner& runner, RunContext& context) {
  const auto& parameters   = context.parameters;
  const auto& repositories = context.repositories;
  auto&       statistics   = context.statisticsLog;
  auto&       measurements = context.measurements;

  double        nextMinPlot          = parameters.getFirstPlotTimeStamp();
  double        nextMaxPlot          = parameters.getFirstPlotTimeStamp();
  double        logNextTimeStep      = 0.0;
  unsigned long completedTimeSteps   = 0;
  unsigned long plots                = 0;
  int           sweepsThisStep       = 0;
  double        currentStepElapsed   = 0.0;
  double        lastCompletedElapsed = 0.0; // wall time of the last completed step, for the box header
  bool          justWroteSnapshot    = false;
  bool          sawNonCriticalAssert = false;

  const int  maxTimeSteps = parameters.getNumberOfTimeSteps();
  const bool stepLimited  = maxTimeSteps > 0;

  enum class Action { TimeStep, Plot, Stop };

  while (true) {
    const double minTerminal  = parameters.getMinTerminalTime();
    const double maxTerminal  = parameters.getMaxTerminalTime();
    const double plotEnd      = maxTerminal < std::numeric_limits<double>::max() / 10.0 ? maxTerminal : minTerminal;
    const int    snapshots    = parameters.getNumberOfSnapshots();
    const double snapInterval = snapshots <= 0 ? 0.0 : plotEnd / snapshots;
    const double minStamp     = repositories.getMinTimeStamp();
    const double maxStamp     = repositories.getMaxTimeStamp();

    const bool keepTimeStepping = stepLimited ? completedTimeSteps < static_cast<unsigned long>(maxTimeSteps)
                                              : (minStamp < minTerminal and maxStamp < maxTerminal);

    // --- decide what runs next ---
    Action action;
    if (tarch::hasNonCriticalAssertionBeenViolated()) {
      if (not sawNonCriticalAssert) {
        sawNonCriticalAssert = true;
        logError("Noncritical assertion has been triggered in code. Dump final state and terminate.");
        action = Action::Plot;
      } else {
        action = Action::Stop;
      }
    } else if (snapInterval > 0.0 and keepTimeStepping and
               (minStamp >= nextMinPlot or maxStamp >= nextMaxPlot) and repositories.mayPlot()) {
      nextMinPlot += snapInterval;
      nextMaxPlot += snapInterval;
      if (nextMinPlot < minStamp) {
        logWarning("Asked to plot more often than the time step size allows; postponing the next plot");
        nextMinPlot = minStamp + snapInterval;
      } else if (nextMaxPlot < maxStamp) {
        logWarning("Asked to plot more often than the time step size allows; postponing the next plot");
        nextMaxPlot = maxStamp + snapInterval;
      }
      nextMaxPlot       = std::max(nextMaxPlot, nextMinPlot);
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (keepTimeStepping) {
      justWroteSnapshot = false;
      action            = Action::TimeStep;
    } else if (not justWroteSnapshot and snapInterval > 0.0 and repositories.mayPlot()) {
      justWroteSnapshot = true;
      nextMinPlot       = std::numeric_limits<double>::max();
      nextMaxPlot       = std::numeric_limits<double>::max();
      action            = Action::Plot;
    } else if (not justWroteSnapshot and snapInterval > 0.0 and not repositories.mayPlot()) {
      action = Action::TimeStep; // do not stop mid-step: let the in-flight step finish
    } else {
      action = Action::Stop;
    }

    // --- run it and emit the one simulation box ---
    if (action == Action::Stop) {
      // One last box so the final block before the summary carries the settled solver state.
      if (completedTimeSteps > 0) {
        statistics.emitSimulationBox(
          runner::displayLabel(_timeStep.name) + " " + std::to_string(completedTimeSteps),
          lastCompletedElapsed,
          timeProgress(repositories.getMinTimeStamp(), minTerminal, statistics.formatETA()),
          true
        );
      }
      break;
    }

    if (action == Action::Plot) {
      const double dt = runner.runStep(_plot.programStep);
      measurements.plot.setValue(dt);
      ++plots;
      // The first plot dumps the initial state before any time step, so the solver has not
      // computed a valid dt / mesh size / eigenvalue yet; show the solver sections only once
      // time stepping has produced them.
      statistics.emitSimulationBox(
        runner::displayLabel(_plot.name) + " " + std::to_string(plots),
        dt,
        timeProgress(repositories.getMinTimeStamp(), minTerminal, statistics.formatETA()),
        completedTimeSteps > 0
      );
      continue;
    }

    // Action::TimeStep. A logical time step may span several sweeps; it is complete when mayPlot
    // flips back to true.
    const double dt = runner.runStep(_timeStep.programStep);
    ++sweepsThisStep;
    currentStepElapsed += dt;
    if (repositories.mayPlot()) {
      measurements.gridSweep.setValue(static_cast<double>(sweepsThisStep));
      if (completedTimeSteps > 0) { // skip the first (warm-up) step for throughput
        measurements.timeStep.setValue(currentStepElapsed);
      }
      lastCompletedElapsed = currentStepElapsed;
      sweepsThisStep       = 0;
      currentStepElapsed   = 0.0;
      ++completedTimeSteps;

      if (measurements.timeStep.getAccumulatedValue() >= logNextTimeStep) {
        statistics.emitSimulationBox(
          runner::displayLabel(_timeStep.name) + " " + std::to_string(completedTimeSteps),
          lastCompletedElapsed,
          timeProgress(repositories.getMinTimeStamp(), minTerminal, statistics.formatETA()),
          true
        );
        logNextTimeStep += parameters.getLogIntervalInSeconds();
      }
    }
  }

  return sawNonCriticalAssert;
}
