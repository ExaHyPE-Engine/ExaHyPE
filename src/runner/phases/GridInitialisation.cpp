// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridInitialisation.h"

#include "runner/Engine.h"
#include "runner/Format.h"
#include "runner/GridSnapshot.h"
#include "runner/Instrumentation.h"
#include "runner/LoadBalancer.h"
#include "runner/PhaseNames.h"
#include "runner/ReportBox.h"
#include "runner/phases/DynamicAMR.h"
#include "tarch/Termination.h"

runner::GridInitialisation::GridInitialisation(
  runner::PhaseContext& context,
  const int             initGridStep,
  runner::DynamicAMR&   dynamicAMR
) :
  _context(context),
  _initGridStep(initGridStep),
  _dynamicAMR(dynamicAMR) {}

/**
 * Initialises the solution and adapts the mesh before time stepping.
 *
 * Each completed mesh update is followed by another initialisation sweep, which re-applies
 * the analytic initial condition, so every cell the drain created holds analytically
 * sampled data. The loop ends once the criterion asks for nothing further, which a
 * criterion skipped right after a mesh change has not done: it reports no requests without
 * having looked, so the loop owes it one more round. A run that still has events after
 * MaxRounds rounds stops, because the time loop would drain them without a following
 * initialisation sweep.
 */
void runner::GridInitialisation::run() {
  SCOREP_USER_REGION(runner::PhaseName::GridInitialisation, SCOREP_USER_REGION_TYPE_PHASE)

  double elapsedSeconds = _context.engine.runStep(_initGridStep);
  int    rounds         = 1;
  while (_dynamicAMR.isEnabled()
         and (_dynamicAMR.hasPendingMeshUpdates() or _dynamicAMR.hasUndecidedRefinementCriterion())
         and rounds < MaxRounds) {
    _dynamicAMR.run();
    elapsedSeconds += _context.engine.runStep(_initGridStep);
    rounds++;
  }
  if (_dynamicAMR.isEnabled() and _dynamicAMR.hasPendingMeshUpdates()) {
    _dynamicAMR.reportPendingSweeps();
    const auto message = "The refinement criterion still requests mesh changes after " + std::to_string(MaxRounds)
      + " initialisation rounds. Continuing would hand those events to the time loop, whose drain fills the "
        "cells it creates by prolongation, so the run would start from interpolated data where the analytic "
        "initial condition is required. The criterion has to settle on the initial condition: check that it "
        "does not alternate between refining and erasing the same cells, and that the resolution it asks "
        "for is reachable within the levels the run allows. Raising -lmax helps only in the second case.";
    tarch::terminateWithError(message);
  }
  _dynamicAMR.reportPendingSweeps();
  _context.box.line(
    std::string(runner::BlockIndent, ' ') + "Grid Initialised " + runner::Format::wallClock(elapsedSeconds)
  );
  runner::loadBalancer().synchroniseGlobalView();
  runner::writeGridSnapshot(_context.box, _context.repositories);
}
