// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeLoop.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/RuntimeParameters.h"
#include "runner/TextLayout.h"
#include "runner/phases/DynamicAMR.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"

namespace {
  using SolverList = std::vector<std::pair<std::string, std::vector<runner::SolverStatistic>>>;

  std::size_t labelColumnWidth(const SolverList& solvers) {
    std::size_t width = std::max(runner::displayWidth("Time Step"), runner::displayWidth("Performance"));
    for (const auto& [name, statistics] : solvers) {
      width = std::max(width, runner::displayWidth(name));
    }
    return width;
  }

  std::string labelledRow(const std::string& label, const std::string& value, const std::size_t labelWidth) {
    std::ostringstream out;
    out << std::string(runner::BlockIndent, ' ') << std::left << std::setw(static_cast<int>(labelWidth)) << label
        << "  " << value;
    return out.str();
  }

  std::string renderStatistics(const std::vector<runner::SolverStatistic>& statistics) {
    std::ostringstream out;
    for (const auto& statistic : statistics) {
      if (not out.str().empty()) {
        out << " · ";
      }
      out << statistic.label << " = ";
      if (statistic.kind == runner::SolverStatistic::Kind::Count) {
        out << statistic.count;
      } else if (statistic.minimum == statistic.maximum) {
        out << runner::Format::scientific(statistic.minimum);
      } else {
        out << "[" << runner::Format::scientific(statistic.minimum) << ", "
            << runner::Format::scientific(statistic.maximum) << "]";
      }
    }
    return out.str();
  }
} // namespace

runner::TimeLoop::TimeLoop(
  runner::PhaseContext& context,
  const int             timeStepStep,
  const int             plotStep,
  runner::DynamicAMR&   dynamicAMR
) :
  _context(context),
  _timeStepStep(timeStepStep),
  _plotStep(plotStep),
  _dynamicAMR(dynamicAMR) {}

bool runner::TimeLoop::terminatedGracefully() const { return _terminatedGracefully; }

std::string runner::TimeLoop::formatETA(const double fraction) const {
  // Allocation-heavy first-step timing is retained, while initialisation drains are excluded from
  // recurring-work extrapolation.
  const auto drainSeconds = _context.measurements.dynamicAMR.getAccumulatedValue() - _dynamicAMRSecondsBeforeLoop;
  const auto elapsed      = _timeSteppingSeconds + _context.measurements.plot.getAccumulatedValue() + drainSeconds;
  if (elapsed <= 0.0 or fraction <= 0.0 or fraction >= 1.0) {
    return "";
  }
  const double remainingSeconds = elapsed * (1.0 - fraction) / fraction;
  if (remainingSeconds >= std::numeric_limits<double>::max() / 10.0) {
    return "";
  }
  return runner::Format::duration(remainingSeconds);
}

void runner::TimeLoop::logTimeStep(
  const double        minimumTimeStamp,
  const double        maximumTimeStamp,
  const double        terminalTime,
  const double        wallClockSeconds,
  const unsigned long completedSteps,
  const int           maximumSteps
) {
  auto& box = _context.box;
  _dynamicAMR.reportPendingSweeps();
  box.rule();

  const bool timeBounded = terminalTime > 0.0 and terminalTime < std::numeric_limits<double>::max() / 10.0;
  double     fraction    = -1.0;
  if (maximumSteps > 0) {
    fraction = std::clamp(static_cast<double>(completedSteps) / maximumSteps, 0.0, 1.0);
  } else if (timeBounded) {
    // The minimum stamp is the time the whole domain has reached; it drives progress and completion.
    fraction = std::clamp(minimumTimeStamp / terminalTime, 0.0, 1.0);
  }
  const std::string estimatedTimeRemaining = fraction > 0.0 ? formatETA(fraction) : "";

  const bool showPerformance = _context.measurements.timeStep.getNumberOfMeasurements() > 0 and wallClockSeconds > 0.0;

  SolverList solvers;
  if (_context.repositories.getSolverStatistics) {
    solvers = _context.repositories.getSolverStatistics();
  }
  const auto labelWidth = labelColumnWidth(solvers);

  std::ostringstream timeStep;
  timeStep << runner::Format::wallClock(wallClockSeconds) << " · t = " << runner::Format::scientific(minimumTimeStamp);
  // Local time stepping spans a range; globally stepped cells share one stamp.
  if (maximumTimeStamp > minimumTimeStamp) {
    timeStep << " .. " << runner::Format::scientific(maximumTimeStamp);
  }
  timeStep << " / " << runner::Format::scientific(terminalTime);
  box.line(labelledRow("Time Step", timeStep.str(), labelWidth));

  for (const auto& [name, statistics] : solvers) {
    box.line(labelledRow(name, renderStatistics(statistics), labelWidth));
  }

  // Grid-wide throughput; skipped on the warm-up step, whose wall time is allocation-dominated.
  if (showPerformance) {
    const auto leaves =
      static_cast<double>(runner::loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    std::ostringstream performance;
    performance << runner::Format::rate(leaves / wallClockSeconds, "cell");
    if (_context.repositories.getNumberOfDoFs) {
      performance << " · "
                  << runner::Format::rate(
                       static_cast<double>(_context.repositories.getNumberOfDoFs()) * leaves / wallClockSeconds,
                       "dof"
                     );
    }
    box.line(labelledRow("Performance", performance.str(), labelWidth));
  }

  if (fraction >= 0.0) {
    const int          percent = std::clamp(static_cast<int>(fraction * 100.0 + 0.5), 0, 100);
    std::ostringstream progress;
    progress << percent << " %";
    if (not estimatedTimeRemaining.empty()) {
      progress << "   ·   ETA " << estimatedTimeRemaining;
    }
    box.line(labelledRow("Progress", progress.str(), labelWidth));
  }

  box.rule();
}

void runner::TimeLoop::logPlot(
  const int    snapshot,
  const int    numberOfSnapshots,
  const double timeStamp,
  const double wallClockSeconds
) {
  std::ostringstream label;
  if (snapshot <= 0) {
    label << "Initial Plot";
  } else {
    label << "Plot " << snapshot;
    if (numberOfSnapshots > 0) {
      label << "/" << numberOfSnapshots;
    }
  }
  // Plain line: keeps the rules' padding, but consecutive plots stack without a blank between them.
  _context.box.line(
    std::string(runner::BlockIndent, ' ') + label.str() + "  " + runner::Format::wallClock(wallClockSeconds)
    + " · t = " + runner::Format::scientific(timeStamp)
  );
}

void runner::TimeLoop::run() {
  SCOREP_USER_REGION(runner::PhaseName::TimeLoop, SCOREP_USER_REGION_TYPE_PHASE)

  const auto& parameters   = _context.parameters;
  const auto& repositories = _context.repositories;
  auto&       measurements = _context.measurements;
  auto&       engine       = _context.engine;

  _dynamicAMRSecondsBeforeLoop = measurements.dynamicAMR.getAccumulatedValue();

  const auto    firstPlotTimeStamp = parameters.getFirstPlotTimeStamp();
  double        nextMinPlot        = firstPlotTimeStamp;
  double        nextMaxPlot        = firstPlotTimeStamp;
  double        nextPlotAtStep     = 0.0;
  double        logNextTimeStep    = 0.0;
  unsigned long completedTimeSteps = 0;
  int           completedPlots     = 0;
  // Tracks the last reported step so shutdown does not print it twice.
  unsigned long lastLoggedStep     = 0;
  double        currentStepElapsed = 0.0;
  double        lastStepElapsed    = 0.0;
  bool          justWroteSnapshot  = false;
  bool          timeStepInFlight   = false;

  const int  maxTimeSteps = parameters.getNumberOfTimeSteps();
  const bool stepLimited  = maxTimeSteps > 0;

  enum class Action { TimeStep, Plot, Stop };

  while (true) {
    // A completed time step may have committed refine/coarsen events. Realise them on the
    // dedicated DynamicAMR observer before the next plot or time step, so both always run
    // on a settled topology. A run that is terminating on a non-critical assertion skips
    // the regrid and dumps its final state on the grid it has.
    if (_dynamicAMR.isEnabled() and _dynamicAMR.hasPendingMeshUpdates()
        and not tarch::hasNonCriticalAssertionBeenViolated()) {
      _dynamicAMR.run();
    }

    const double minTerminal  = parameters.getMinTerminalTime();
    const double maxTerminal  = parameters.getMaxTerminalTime();
    const double plotEnd      = maxTerminal < std::numeric_limits<double>::max() / 10.0 ? maxTerminal : minTerminal;
    const int    snapshots    = parameters.getNumberOfSnapshots();
    const double snapInterval = snapshots <= 0 ? 0.0 : plotEnd / snapshots;
    // A run bounded by a fixed number of time steps ends at that step budget, so its snapshots
    // spread over the steps.
    const double stepPlotInterval =
      (stepLimited and snapshots > 0) ? static_cast<double>(maxTimeSteps) / snapshots : 0.0;
    const double minStamp             = repositories.getMinTimeStamp();
    const double maxStamp             = repositories.getMaxTimeStamp();
    const bool   firstPlotTimeReached = minStamp >= firstPlotTimeStamp or maxStamp >= firstPlotTimeStamp;
    if (stepLimited and not firstPlotTimeReached) {
      nextPlotAtStep = static_cast<double>(completedTimeSteps) + 1.0;
    }

    const bool keepTimeStepping = stepLimited ? completedTimeSteps < static_cast<unsigned long>(maxTimeSteps)
                                              : (minStamp < minTerminal and maxStamp < maxTerminal);

    Action action;
    if (tarch::hasNonCriticalAssertionBeenViolated()) {
      if (not _terminatedGracefully) {
        _terminatedGracefully = true;
        logError("Non-critical assertion has been triggered in code. Dump final state and terminate.");
        action = Action::Plot;
      } else {
        action = Action::Stop;
      }
    } else if (stepLimited and stepPlotInterval > 0.0 and keepTimeStepping and firstPlotTimeReached
               and static_cast<double>(completedTimeSteps) >= nextPlotAtStep and repositories.mayPlot()) {
      nextPlotAtStep += stepPlotInterval;
      if (nextPlotAtStep <= static_cast<double>(completedTimeSteps)) {
        logWarning("Asked to plot more often than once per time step; postponing the next plot.");
        nextPlotAtStep = static_cast<double>(completedTimeSteps) + 1.0;
      }
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (not stepLimited and snapInterval > 0.0 and keepTimeStepping and completedPlots < snapshots - 1
               and (minStamp >= nextMinPlot or maxStamp >= nextMaxPlot) and repositories.mayPlot()) {
      // The final-state dump at termination completes the numbered snapshot count, so the
      // interval trigger stops one short. Solvers advancing at different rates spread the
      // stamps: minStamp belongs to the slowest solver, maxStamp to the fastest, and with
      // subcycling or local time stepping a plot target can sit between the two for many
      // sweeps. Without the bound the fastest stamp crosses surplus targets before the
      // slowest one ends the run. The snapshot spacing follows the fastest solver; testing
      // only the slowest stamp would space the snapshots evenly in terminal time, at the
      // price of plotting nothing until the slowest solver reaches each target.
      nextMinPlot += snapInterval;
      nextMaxPlot += snapInterval;
      if (nextMinPlot < minStamp) {
        logWarning("Asked to plot more often than the time step size allows; postponing the next plot.");
        nextMinPlot = minStamp + snapInterval;
      } else if (nextMaxPlot < maxStamp) {
        logWarning("Asked to plot more often than the time step size allows; postponing the next plot.");
        nextMaxPlot = maxStamp + snapInterval;
      }
      nextMaxPlot       = std::max(nextMaxPlot, nextMinPlot);
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (keepTimeStepping) {
      justWroteSnapshot = false;
      action            = Action::TimeStep;
    } else if (timeStepInFlight and not repositories.mayPlot()) {
      // Terminal reached mid logical step: finish it before stopping (a step spans several sweeps),
      // regardless of whether plotting is enabled - otherwise we would stop in an inconsistent state.
      // A plot sweep also clears mayPlot(), so only an actually in-flight time step warrants
      // another sweep; otherwise a run ending on its final plot would compute one surplus step.
      action = Action::TimeStep;
    } else if (not justWroteSnapshot and (snapInterval > 0.0 or stepPlotInterval > 0.0)) {
      justWroteSnapshot = true;
      nextMinPlot       = std::numeric_limits<double>::max();
      nextMaxPlot       = std::numeric_limits<double>::max();
      nextPlotAtStep    = std::numeric_limits<double>::max();
      action            = Action::Plot;
    } else {
      action = Action::Stop;
    }

    if (action == Action::Stop) {
      // After a non-critical assertion we only dump the final state; no closing time-step summary.
      if (not _terminatedGracefully and completedTimeSteps > 0 and completedTimeSteps != lastLoggedStep) {
        logTimeStep(
          repositories.getMinTimeStamp(),
          repositories.getMaxTimeStamp(),
          minTerminal,
          lastStepElapsed,
          completedTimeSteps,
          maxTimeSteps
        );
      } else {
        _dynamicAMR.reportPendingSweeps();
      }
      break;
    }

    if (action == Action::Plot) {
      double elapsedSeconds = 0.0;
      {
        SCOREP_USER_REGION(runner::PhaseName::Plot, SCOREP_USER_REGION_TYPE_COMMON)
        elapsedSeconds = engine.runStep(_plotStep);
      }
      measurements.plot.setValue(elapsedSeconds);
      // The initial condition (plotted before any step ran) is not one of the numbered snapshots.
      if (completedTimeSteps == 0) {
        logPlot(0, 0, repositories.getMinTimeStamp(), elapsedSeconds);
      } else {
        ++completedPlots;
        logPlot(completedPlots, snapshots, repositories.getMinTimeStamp(), elapsedSeconds);
      }
      continue;
    }

    // A logical time step is complete only when mayPlot becomes true after its final sweep.
    double elapsedSeconds = 0.0;
    {
      SCOREP_USER_REGION(runner::PhaseName::TimeStep, SCOREP_USER_REGION_TYPE_COMMON)
      elapsedSeconds = engine.runStep(_timeStepStep);
    }
    currentStepElapsed += elapsedSeconds;
    timeStepInFlight = not repositories.mayPlot();
    if (repositories.mayPlot()) {
      _timeSteppingSeconds += currentStepElapsed;
      if (completedTimeSteps > 0) { // Skip the first (warm-up) step for throughput.
        measurements.timeStep.setValue(currentStepElapsed);
      }
      lastStepElapsed    = currentStepElapsed;
      currentStepElapsed = 0.0;
      ++completedTimeSteps;

      // A step that tripped a non-critical assertion carries invalid data, such as a time-step size
      // from a zero eigenvalue; skip its report and let the next iteration dump the final state and terminate.
      if (not tarch::hasNonCriticalAssertionBeenViolated()
          and measurements.timeStep.getAccumulatedValue() >= logNextTimeStep) {
        logTimeStep(
          repositories.getMinTimeStamp(),
          repositories.getMaxTimeStamp(),
          minTerminal,
          lastStepElapsed,
          completedTimeSteps,
          maxTimeSteps
        );
        lastLoggedStep = completedTimeSteps;
        logNextTimeStep += parameters.getTimeStepLogInterval();
      }
    }
  }
}
