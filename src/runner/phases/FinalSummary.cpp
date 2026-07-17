// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FinalSummary.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GridSnapshot.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"

namespace {
  std::string displayLabel(const std::string& observerName) {
    if (observerName == runner::StepName::CreateGrid) {
      return runner::PhaseName::GridConstruction;
    }
    if (observerName == runner::StepName::LoadBalancing) {
      return runner::PhaseName::LoadBalancing;
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
    return observerName;
  }

  std::string seconds(const double value) {
    std::ostringstream result;
    result << std::fixed << std::setprecision(2) << std::setw(12) << value << " s";
    return result.str();
  }
} // namespace

runner::FinalSummary::FinalSummary(PhaseContext& context) :
  _context(context) {}

void runner::FinalSummary::run() {
  loadBalancer().synchroniseGlobalView();
  writeGridSnapshot(_context.box, _context.repositories);

  const auto& measurements = _context.measurements;

  std::vector<std::pair<std::string, std::string>> entries;
  for (const auto& [name, measurement] : measurements.stepTimes) {
    const auto         count = measurement.getNumberOfMeasurements();
    std::ostringstream value;
    value << seconds(measurement.getAccumulatedValue()) << "  (" << count << (count == 1 ? " sweep)" : " sweeps)");
    entries.emplace_back("Total Time Spent in " + displayLabel(name), value.str());
  }

  if (measurements.timeStep.getNumberOfMeasurements() > 0) {
    entries.emplace_back("Average Time per Time Step", seconds(measurements.timeStep.getAverageValue()));
    const auto leaves = static_cast<double>(loadBalancer().getStatistics().getGlobalNumberOfInnerUnrefinedCells());
    const auto dt     = measurements.timeStep.getAverageValue();
    if (dt > 0.0) {
      std::ostringstream value;
      value << Format::rate(leaves / dt, "cell");
      if (_context.repositories.getNumberOfDoFs) {
        value << " · "
              << Format::rate(static_cast<double>(_context.repositories.getNumberOfDoFs()) * leaves / dt, "dof");
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
    std::ostringstream out;
    out << std::string(BlockIndent, ' ') << std::left << std::setw(static_cast<int>(labelWidth)) << label << "  "
        << value;
    box.line(out.str());
  }
  box.rule();
}
