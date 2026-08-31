// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Orchestrator.h"

#include "runner/Phase.h"
#include "runner/ReportBox.h"
#include "runner/phases/Configuration.h"
#include "runner/phases/DynamicAMR.h"
#include "runner/phases/FinalSummary.h"
#include "runner/phases/GridConstruction.h"
#include "runner/phases/GridInitialisation.h"
#include "runner/phases/TimeLoop.h"

runner::Orchestrator::Orchestrator(const std::vector<runner::Step>& steps) {
  for (const auto& step : steps) {
    if (step.name == runner::StepName::CreateGrid) {
      _createGridStep = step.programStep;
    } else if (step.name == runner::StepName::InitGrid) {
      _initGridStep = step.programStep;
    } else if (step.name == runner::StepName::PlotSolution) {
      _plotStep = step.programStep;
    } else if (step.name == runner::StepName::TimeStep) {
      _timeStepStep = step.programStep;
    } else if (step.name == runner::StepName::DynamicAMR) {
      _dynamicAMRStep = step.programStep;
    }
  }
}

/** Returns whether a non-critical assertion ended the run early. */
bool runner::Orchestrator::run(runner::PhaseContext& context) {
  runner::Configuration      configuration(context);
  runner::GridConstruction   gridConstruction(context, _createGridStep);
  runner::DynamicAMR         dynamicAMR(context, _dynamicAMRStep);
  runner::GridInitialisation gridInitialisation(context, _initGridStep, dynamicAMR);
  runner::TimeLoop           timeLoop(context, _timeStepStep, _plotStep, dynamicAMR);

  std::vector<runner::Phase*> phases;
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

  for (auto* const phase : phases) {
    phase->run();
  }

  const bool stoppedAfterNonCriticalAssertion = (_timeStepStep >= 0) and timeLoop.stoppedAfterNonCriticalAssertion();
  if (not stoppedAfterNonCriticalAssertion) {
    runner::FinalSummary(context).run();
  }

  context.box.rule();
  context.box.line(
    stoppedAfterNonCriticalAssertion ? "Run terminated gracefully after a non-critical assertion"
                                     : "Run terminated successfully"
  );
  context.box.line("Goodbye");
  context.box.rule();

  return stoppedAfterNonCriticalAssertion;
}
