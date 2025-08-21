// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Grid-main.h"

#include "Constants.h"
#include "CreateGrid.h"
#include "EmptyObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/grid/TraversalVTKPlotter.h"
#include "peano4/peano4.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

namespace {
  tarch::logging::Log _log("");

  tarch::timing::Measurement gridConstructionMeasurement;
  tarch::timing::Measurement emptyTraversalMeasurement;
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

  peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);

  peano4::grid::Spacetree spacetree(DomainOffset, DomainSize, 0);

  CreateGrid createGrid;
  int        depth = 0;
  while (spacetree.getGridStatistics().getMinH(0) > MinH) {
    depth++;
    watch.start();
    spacetree.traverse(createGrid, true);
    watch.stop();
    logInfo(
      "main()",
      "Grid construction for depth="
        << depth << " with number of cells=" << spacetree.getGridStatistics().getNumberOfLocalRefinedCells() << "/"
        << spacetree.getGridStatistics().getNumberOfLocalUnrefinedCells() << " [refined/unrefined]"
        << " took " << watch.getCalendarTime() << " seconds"
    );
    gridConstructionMeasurement.setValue(watch.getCalendarTime());
  }

  EmptyObserver emptyObserver;
  for (int t = 0; t < 5; t++) {
    watch.start();
    spacetree.traverse(emptyObserver, true);
    watch.stop();
    logInfo(
      "main()",
      "Empty traversal on grid for depth="
        << depth << " with number of cells=" << spacetree.getGridStatistics().getNumberOfLocalRefinedCells() << "/"
        << spacetree.getGridStatistics().getNumberOfLocalUnrefinedCells() << " [refined/unrefined]"
        << " took " << watch.getCalendarTime() << " seconds"
    );
    emptyTraversalMeasurement.setValue(watch.getCalendarTime());
  }

  logInfo(
    "main()",
    "Grid construction took " << gridConstructionMeasurement.getAccumulatedValue() << " seconds in total"
  );
  logInfo(
    "main()",
    "Empty traversals on grid took " << emptyTraversalMeasurement.getAccumulatedValue() << " seconds in total"
  );

  // peano4::grid::TraversalVTKPlotter plotterObserver("Grid");
  // spacetree.traverse(plotterObserver);

  peano4::shutdown();

  return EXIT_SUCCESS;
}
