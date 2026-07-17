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
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Error.h"
#include "tarch/instrumentation/Warning.h"

namespace {
  using SolverList = std::vector<std::pair<std::string, std::vector<runner::SolverStatistic>>>;

  // Widest label in the Time Step box; every row's value starts one column past it.
  std::size_t labelColumnWidth(const SolverList& solvers) {
    std::size_t width = std::max(runner::displayWidth("Time Step"), runner::displayWidth("Performance"));
    for (const auto& [name, statistics] : solvers) {
      width = std::max(width, runner::displayWidth(name));
    }
    return width;
  }

  std::string labeledRow(const std::string& label, const std::string& value, std::size_t labelWidth) {
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

runner::TimeLoop::TimeLoop(PhaseContext& context, int timeStepStep, int plotStep) :
  _context(context),
  _timeStepStep(timeStepStep),
  _plotStep(plotStep) {}

std::vector<std::string> runner::TimeLoop::widthSamples() const {
  SolverList solvers;
  if (_context.repositories.getSolverStatistics) {
    solvers = _context.repositories.getSolverStatistics();
  }
  const auto labelWidth = labelColumnWidth(solvers);

  std::vector<std::string> samples;

  // Widest Time Step form: the local-time-stepping range "min .. max" plus the terminal.
  const double       sentinel = std::numeric_limits<double>::max();
  std::ostringstream timeStep;
  timeStep << Format::wallClock(9999.99) << " · t = " << Format::scientific(sentinel) << " .. "
           << Format::scientific(sentinel) << " / " << Format::scientific(sentinel);
  samples.push_back(labeledRow("Time Step", timeStep.str(), labelWidth));

  for (const auto& [name, statistics] : solvers) {
    samples.push_back(labeledRow(name, renderStatistics(statistics), labelWidth));
  }

  samples.push_back(
    labeledRow("Performance", Format::rate(999.9e9, "cell") + " · " + Format::rate(999.9e9, "dof"), labelWidth)
  );

  // Progress row: full percent and the widest ETA the formatter emits.
  samples.push_back(labeledRow("Progress", "100 %   ·   ETA " + Format::duration(100.0 * 24.0 * 3600.0), labelWidth));

  return samples;
}

std::string runner::TimeLoop::formatETA(double fraction) const {
  const auto elapsed =
    _context.measurements.timeStep.getAccumulatedValue() + _context.measurements.plot.getAccumulatedValue();
  if (elapsed <= 0.0 or fraction <= 0.0 or fraction >= 1.0) {
    return "";
  }
  const double remaining = elapsed * (1.0 - fraction) / fraction;
  if (remaining >= std::numeric_limits<double>::max() / 10.0) {
    return "";
  }
  return Format::duration(remaining);
}

void runner::TimeLoop::logTimeStep(
  double        minTimeStamp,
  double        maxTimeStamp,
  double        terminalTime,
  double        wallClockSeconds,
  unsigned long completedSteps,
  int           maxSteps
) {
  auto& box = _context.box;
  box.rule();

  const bool timeBounded = terminalTime > 0.0 and terminalTime < std::numeric_limits<double>::max() / 10.0;
  double     fraction    = -1.0;
  if (maxSteps > 0) {
    fraction = std::clamp(static_cast<double>(completedSteps) / maxSteps, 0.0, 1.0);
  } else if (timeBounded) {
    // The minimum stamp is the time the whole domain has reached; it drives progress and completion.
    fraction = std::clamp(minTimeStamp / terminalTime, 0.0, 1.0);
  }
  const std::string eta = fraction > 0.0 ? formatETA(fraction) : "";

  const bool showPerformance = _context.measurements.timeStep.getNumberOfMeasurements() > 0 and wallClockSeconds > 0.0;

  SolverList solvers;
  if (_context.repositories.getSolverStatistics) {
    solvers = _context.repositories.getSolverStatistics();
  }
  const auto labelWidth = labelColumnWidth(solvers);

  std::ostringstream timeStep;
  timeStep << Format::wallClock(wallClockSeconds) << " · t = " << Format::scientific(minTimeStamp);
  // With local time stepping the cells span a range; show it. Globally min == max, so nothing extra.
  if (maxTimeStamp > minTimeStamp) {
    timeStep << " .. " << Format::scientific(maxTimeStamp);
  }
  timeStep << " / " << Format::scientific(terminalTime);
  box.line(labeledRow("Time Step", timeStep.str(), labelWidth));

  for (const auto& [name, statistics] : solvers) {
    box.line(labeledRow(name, renderStatistics(statistics), labelWidth));
  }

  // Grid-wide throughput; skipped on the warm-up step, whose wall time is allocation-dominated.
  if (showPerformance) {
    const auto leaves = static_cast<double>(loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    std::ostringstream performance;
    performance << Format::rate(leaves / wallClockSeconds, "cell");
    if (_context.repositories.getNumberOfDoFs) {
      performance << " · "
                  << Format::rate(
                       static_cast<double>(_context.repositories.getNumberOfDoFs()) * leaves / wallClockSeconds,
                       "dof"
                     );
    }
    box.line(labeledRow("Performance", performance.str(), labelWidth));
  }

  if (fraction >= 0.0) {
    const int          percent = std::clamp(static_cast<int>(fraction * 100.0 + 0.5), 0, 100);
    std::ostringstream progress;
    progress << percent << " %";
    if (not eta.empty()) {
      progress << "   ·   ETA " << eta;
    }
    box.line(labeledRow("Progress", progress.str(), labelWidth));
  }

  box.rule();
}

void runner::TimeLoop::logPlot(int snapshot, int numberOfSnapshots, double timeStamp, double wallClockSeconds) {
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
    std::string(BlockIndent, ' ') + label.str() + "  " + Format::wallClock(wallClockSeconds)
    + " · t = " + Format::scientific(timeStamp)
  );
}

void runner::TimeLoop::run() {
  SCOREP_USER_REGION(PhaseName::TimeLoop, SCOREP_USER_REGION_TYPE_PHASE)

  const auto& parameters   = _context.parameters;
  const auto& repositories = _context.repositories;
  auto&       measurements = _context.measurements;
  auto&       engine       = _context.engine;

  const auto    firstPlotTimeStamp = parameters.getFirstPlotTimeStamp();
  double        nextMinPlot        = firstPlotTimeStamp;
  double        nextMaxPlot        = firstPlotTimeStamp;
  double        nextPlotAtStep     = 0.0;
  double        logNextTimeStep    = 0.0;
  unsigned long completedTimeSteps = 0;
  int           completedPlots     = 0;
  unsigned long lastLoggedStep     = 0; // step already shown, so Stop does not log it again
  double        currentStepElapsed = 0.0;
  double        lastStepElapsed    = 0.0; // wall time of the most recent completed step
  bool          justWroteSnapshot  = false;
  bool          timeStepInFlight   = false;

  const int  maxTimeSteps = parameters.getNumberOfTimeSteps();
  const bool stepLimited  = maxTimeSteps > 0;

  enum class Action { TimeStep, Plot, Stop };

  while (true) {
    const double minTerminal  = parameters.getMinTerminalTime();
    const double maxTerminal  = parameters.getMaxTerminalTime();
    const double plotEnd      = maxTerminal < std::numeric_limits<double>::max() / 10.0 ? maxTerminal : minTerminal;
    const int    snapshots    = parameters.getNumberOfSnapshots();
    const double snapInterval = snapshots <= 0 ? 0.0 : plotEnd / snapshots;
    // A run bounded by a fixed number of time steps ends at that step budget, not at the terminal
    // time, so the snapshots spread over the steps instead of the time axis.
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
        logError("Noncritical assertion has been triggered in code. Dump final state and terminate.");
        action = Action::Plot;
      } else {
        action = Action::Stop;
      }
    } else if (stepLimited and stepPlotInterval > 0.0 and keepTimeStepping and firstPlotTimeReached
               and static_cast<double>(completedTimeSteps) >= nextPlotAtStep and repositories.mayPlot()) {
      nextPlotAtStep += stepPlotInterval;
      if (nextPlotAtStep <= static_cast<double>(completedTimeSteps)) {
        logWarning("Asked to plot more often than once per time step; postponing the next plot");
        nextPlotAtStep = static_cast<double>(completedTimeSteps) + 1.0;
      }
      justWroteSnapshot = true;
      action            = Action::Plot;
    } else if (not stepLimited and snapInterval > 0.0 and keepTimeStepping
               and (minStamp >= nextMinPlot or maxStamp >= nextMaxPlot) and repositories.mayPlot()) {
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
      // After a noncritical assertion we only dump the final state; no closing time-step summary.
      if (not _terminatedGracefully and completedTimeSteps > 0 and completedTimeSteps != lastLoggedStep) {
        logTimeStep(
          repositories.getMinTimeStamp(),
          repositories.getMaxTimeStamp(),
          minTerminal,
          lastStepElapsed,
          completedTimeSteps,
          maxTimeSteps
        );
      }
      break;
    }

    if (action == Action::Plot) {
      double dt = 0.0;
      {
        SCOREP_USER_REGION(PhaseName::Plot, SCOREP_USER_REGION_TYPE_COMMON)
        dt = engine.runStep(_plotStep);
      }
      measurements.plot.setValue(dt);
      // The initial condition (plotted before any step ran) is not one of the numbered snapshots.
      if (completedTimeSteps == 0) {
        logPlot(0, 0, repositories.getMinTimeStamp(), dt);
      } else {
        ++completedPlots;
        logPlot(completedPlots, snapshots, repositories.getMinTimeStamp(), dt);
      }
      continue;
    }

    // Action::TimeStep. A logical time step may span several sweeps; it is complete when mayPlot flips
    // back to true.
    double dt = 0.0;
    {
      SCOREP_USER_REGION(PhaseName::TimeStep, SCOREP_USER_REGION_TYPE_COMMON)
      dt = engine.runStep(_timeStepStep);
    }
    currentStepElapsed += dt;
    timeStepInFlight = not repositories.mayPlot();
    if (repositories.mayPlot()) {
      if (completedTimeSteps > 0) { // skip the first (warm-up) step for throughput
        measurements.timeStep.setValue(currentStepElapsed);
      }
      lastStepElapsed    = currentStepElapsed;
      currentStepElapsed = 0.0;
      ++completedTimeSteps;

      // A step that tripped a noncritical assertion carries invalid data (e.g. dt from a zero
      // eigenvalue); skip its report and let the next iteration dump the final state and terminate.
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
