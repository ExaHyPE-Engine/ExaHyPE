// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Orchestrator.h"

#include "runner/Phase.h"
#include "runner/ReportBox.h"
#include "runner/TextLayout.h"
#include "runner/phases/Configuration.h"
#include "runner/phases/FinalSummary.h"
#include "runner/phases/GridConstruction.h"
#include "runner/phases/GridInitialisation.h"
#include "runner/phases/TimeLoop.h"

runner::Orchestrator::Orchestrator(const std::vector<Step>& steps) {
  for (const auto& step : steps) {
    if (step.name == StepName::CreateGrid) {
      _createGridStep = step.programStep;
    } else if (step.name == StepName::LoadBalancing) {
      _loadBalancingStep = step.programStep;
    } else if (step.name == StepName::InitGrid) {
      _initGridStep = step.programStep;
    } else if (step.name == StepName::PlotSolution) {
      _plotStep = step.programStep;
    } else if (step.name == StepName::TimeStep) {
      _timeStepStep = step.programStep;
    }
  }
}

bool runner::Orchestrator::run(PhaseContext& context) {
  Configuration      configuration(context);
  GridConstruction   gridConstruction(context, _createGridStep, _loadBalancingStep);
  GridInitialisation gridInitialisation(context, _initGridStep);
  TimeLoop           timeLoop(context, _timeStepStep, _plotStep);

  std::vector<Phase*> phases;
  phases.push_back(&configuration);
  if (_createGridStep >= 0) {
    phases.push_back(&gridConstruction);
  }
  if (_initGridStep >= 0) {
    phases.push_back(&gridInitialisation);
  }
  if (_timeStepStep >= 0) {
    phases.push_back(&timeLoop);
  }

  // Size the box to the widest line any phase can produce, so its width is fixed before the first
  // line is drawn and no line is ever truncated.
  std::size_t width = 0;
  for (const auto* phase : phases) {
    for (const auto& sample : phase->widthSamples()) {
      width = std::max(width, displayWidth(sample));
    }
  }
  context.box.setWidth(static_cast<int>(width));

  for (auto* phase : phases) {
    phase->run();
  }

  const bool terminatedGracefully = (_timeStepStep >= 0) and timeLoop.terminatedGracefully();
  if (not terminatedGracefully) {
    FinalSummary(context).run();
  }

  context.box.rule();
  context.box.line(
    terminatedGracefully ? "Run terminated gracefully after a noncritical assertion" : "Run terminated successfully"
  );
  context.box.line("Goodbye");
  context.box.rule();

  return terminatedGracefully;
}
