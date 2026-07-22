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

runner::GridInitialisation::GridInitialisation(PhaseContext& context, int initGridStep, DynamicAMR& dynamicAMR) :
  _context(context),
  _initGridStep(initGridStep),
  _dynamicAMR(dynamicAMR) {}

std::vector<std::string> runner::GridInitialisation::widthSamples() const { return gridSnapshotWidthSamples(); }

/**
 * Initialise the solution and adapt the mesh before time stepping.
 *
 * Each completed mesh update is followed by another initialisation sweep, which re-applies
 * the analytic initial condition, so every cell the drain created holds analytically
 * sampled data. The loop ends once the criterion asks for nothing further, and a run that
 * still has events after MaxRounds rounds stops, because the time loop would drain them
 * without a following initialisation sweep.
 */
void runner::GridInitialisation::run() {
  SCOREP_USER_REGION(PhaseName::GridInitialisation, SCOREP_USER_REGION_TYPE_PHASE)

  double elapsedSeconds = _context.engine.runStep(_initGridStep);
  int    rounds         = 1;
  while (_dynamicAMR.isEnabled() and _dynamicAMR.hasPendingMeshUpdates() and rounds < MaxRounds) {
    _dynamicAMR.run();
    elapsedSeconds += _context.engine.runStep(_initGridStep);
    rounds++;
  }
  if (_dynamicAMR.isEnabled() and _dynamicAMR.hasPendingMeshUpdates()) {
    _dynamicAMR.reportPendingSweeps();
    const auto message = "The refinement criterion still requests mesh changes after " + std::to_string(MaxRounds)
      + " initialisation rounds. Continuing would hand those events to the time loop, whose drain fills the "
        "cells it creates by prolongation, so the run would start from interpolated data where the analytic "
        "initial condition is required. The criterion has to settle on the initial condition: Check that it "
        "does not alternate between refining and erasing the same cells, and that the resolution it asks "
        "for is reachable within the levels the run allows. Raising -lmax helps only in the second case.";
    tarch::terminateWithError(message);
  }
  _dynamicAMR.reportPendingSweeps();
  _context.box.line(std::string(BlockIndent, ' ') + "Grid Initialised " + Format::wallClock(elapsedSeconds));
  loadBalancer().synchroniseGlobalView();
  writeGridSnapshot(_context.box, _context.repositories);
}
