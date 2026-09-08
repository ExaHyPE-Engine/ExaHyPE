// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FinalSummary.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GlobalStatistics.h"
#include "runner/GridSnapshot.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "tarch/tasking/Runtime.h"

namespace {
  std::string displayLabel(const std::string& observerName) {
    if (observerName == runner::StepName::CreateGrid) {
      return runner::PhaseName::GridConstruction;
    }
    if (observerName == runner::StepName::InitGrid) {
      return runner::PhaseName::GridInitialisation;
    }
    if (observerName == runner::StepName::PlotSolution) {
      return runner::PhaseName::Plot;
    }
    if (observerName == runner::StepName::TimeStep) {
      return runner::PhaseName::TimeStep;
    }
    if (observerName == runner::StepName::DynamicAMR) {
      return runner::PhaseName::DynamicAMR;
    }
    return observerName;
  }

  std::string seconds(const double value) {
    std::ostringstream result;
    result << std::fixed << std::setprecision(2) << value << " s";
    return result.str();
  }
} // namespace

runner::FinalSummary::FinalSummary(runner::PhaseContext& context) :
  _context(context) {}

void runner::FinalSummary::run() {
  runner::writeGridSnapshot(_context.box, _context.repositories);

  const auto& measurements = _context.measurements;

  // The rows follow the phase order of a run; steps outside this list keep their first-use order.
  constexpr std::array<const char*, 5> StepOrder = {
    runner::StepName::CreateGrid,
    runner::StepName::InitGrid,
    runner::StepName::PlotSolution,
    runner::StepName::DynamicAMR,
    runner::StepName::TimeStep
  };
  const auto stepPosition = [&StepOrder](const std::string& name) {
    const auto* const position = std::find(StepOrder.begin(), StepOrder.end(), name);
    return std::distance(StepOrder.begin(), position);
  };
  auto stepTimes = measurements.stepTimes;
  std::stable_sort(stepTimes.begin(), stepTimes.end(), [&stepPosition](const auto& left, const auto& right) {
    return stepPosition(left.first) < stepPosition(right.first);
  });

  // Task time divided by wall time and thread count gives the fraction of the cores each step occupies.
  const auto taskSecondsOf = [&measurements](const std::string& name) {
    for (const auto& [stepName, taskSeconds] : measurements.stepTaskSeconds) {
      if (stepName == name) {
        return taskSeconds;
      }
    }
    return 0.0;
  };
  const auto numberOfThreads = tarch::tasking::Runtime::getInstance().getNumberOfThreads();

  std::vector<std::pair<std::string, std::string>> entries;
  for (const auto& [name, measurement] : stepTimes) {
    const auto         numberOfSweeps = measurement.getNumberOfMeasurements();
    const auto         wallClock      = measurement.getAccumulatedValue();
    std::ostringstream value;
    value << seconds(wallClock) << "  (" << numberOfSweeps << (numberOfSweeps == 1 ? " Sweep" : " Sweeps");
    if (numberOfThreads > 1 and wallClock > 0.0) {
      value << " · Cores " << runner::Format::percentage(100.0 * taskSecondsOf(name) / (numberOfThreads * wallClock));
    }
    value << ")";
    entries.emplace_back("Total Time Spent in " + displayLabel(name), value.str());
  }

  if (measurements.timeStep.getNumberOfMeasurements() > 0) {
    entries.emplace_back("Average Time per Time Step", seconds(measurements.timeStep.getAverageValue()));
    const auto leaves          = static_cast<double>(runner::globalStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto averageTimeStep = measurements.timeStep.getAverageValue();
    if (averageTimeStep > 0.0) {
      std::ostringstream value;
      value << runner::Format::rate(leaves / averageTimeStep, "cell");
      if (_context.repositories.getNumberOfDoFs) {
        value << " · "
              << runner::Format::rate(
                   static_cast<double>(_context.repositories.getNumberOfDoFs()) * leaves / averageTimeStep,
                   "dof"
                 );
      }
      entries.emplace_back("Average Performance", value.str());
    }
  }

  auto& box        = _context.box;
  auto  labelWidth = std::size_t{0};
  for (const auto& entry : entries) {
    labelWidth = std::max(labelWidth, entry.first.size());
  }

  box.rule();
  for (const auto& [label, value] : entries) {
    std::ostringstream line;
    line << std::string(runner::BlockIndent, ' ') << std::left << std::setw(static_cast<int>(labelWidth)) << label
         << "  " << value;
    box.line(line.str());
  }
  box.rule();
}
