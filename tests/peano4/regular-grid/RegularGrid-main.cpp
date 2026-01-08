// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RegularGrid-main.h"

#include "Constants.h"
#include "CreateGridObserver.h"
#include "EmptyTraversalObserver.h"
#include "VTUPlotterObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/peano4.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

namespace {
  tarch::logging::Log _log("");
} // namespace

using namespace tests::peano4::grid;

int main(int argc, char** argv) {
  static tarch::timing::Watch watch("::", "step()", false);

#if DIMENSIONS == 2
  const auto DomainOffset = {0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0};
#else
  const auto DomainOffset = {0.0, 0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0, 1.0};
#endif

  ::peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);
  ::peano4::writeCopyrightMessage();

  ::peano4::grid::Spacetree spacetree(DomainOffset, DomainSize, 0);

  CreateGridObserver createGridObserver;
  while (spacetree.getGridStatistics().minH(0) > MinH) {
    watch.start();
    spacetree.traverse(createGridObserver, true);
    watch.stop();
    logInfo("main()", "Grid construction took " << watch.getCalendarTime() << "s: " << spacetree.getGridStatistics());
  }

  EmptyTraversalObserver emptyTraversalObserver;
  watch.start();
  spacetree.traverse(emptyTraversalObserver, true);
  watch.stop();
  logInfo("main()", "Empty traversal took " << watch.getCalendarTime() << "s: " << spacetree.getGridStatistics());
  logInfo(
    "main()",
    "Average cell updates per second: "
      << spacetree.getGridStatistics().numberOfLocalUnrefinedCells / watch.getCalendarTime()
  );

  VTUPlotterObserver plotterObserver(0);
  watch.start();
  VTUPlotterObserver::prepareTraversal();
  spacetree.traverse(plotterObserver);
  VTUPlotterObserver::unprepareTraversal();
  watch.stop();
  logInfo("main()", "Grid plotting took " << watch.getCalendarTime() << "s: " << spacetree.getGridStatistics());

  ::peano4::shutdown();

  return EXIT_SUCCESS;
}
