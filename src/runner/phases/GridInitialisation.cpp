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

runner::GridInitialisation::GridInitialisation(PhaseContext& context, int initGridStep) :
  _context(context),
  _initGridStep(initGridStep) {}

std::vector<std::string> runner::GridInitialisation::widthSamples() const { return gridSnapshotWidthSamples(); }

void runner::GridInitialisation::run() {
  SCOREP_USER_REGION(PhaseName::GridInitialisation, SCOREP_USER_REGION_TYPE_PHASE)

  const double elapsedSeconds = _context.engine.runStep(_initGridStep);
  _context.box.line(std::string(BlockIndent, ' ') + "Grid Initialised " + Format::wallClock(elapsedSeconds));
  loadBalancer().synchroniseGlobalView();
  writeGridSnapshot(_context.box, _context.repositories);
}
