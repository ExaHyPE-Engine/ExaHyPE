// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Spacetree-main.h"

#include "Constants.h"
#include "CreateGridObserver.h"
#include "EmptyTraversalObserver.h"
#include "peano4/grid/Spacetree.h"
#include "peano4/peano4.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/instrumentation/Measurement.h"
#include "tarch/instrumentation/Watch.h"

using namespace tests::peano4::grid;

int main(int argc, char** argv) {
  static tarch::instrumentation::Watch watch("::", "step()", false);

#if DIMENSIONS == 2
  const auto DomainOffset = {0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0};
#else
  const auto DomainOffset = {0.0, 0.0, 0.0};
  const auto DomainSize   = {1.0, 1.0, 1.0};
#endif

  ::peano4::init(&argc, &argv, DomainOffset, DomainSize, 0);

  ::peano4::grid::Spacetree spacetree(DomainOffset, DomainSize, 0);

  CreateGridObserver createGridObserver;
  while (spacetree.getGridStatistics().minH(0) > MinH) {
    watch.start();
    spacetree.traverse(createGridObserver, true);
    watch.stop();
    logInfo("Grid construction took " << watch.getCalendarTime() << "s: " << spacetree.getGridStatistics());
  }

  EmptyTraversalObserver emptyTraversalObserver;
  watch.start();
  spacetree.traverse(emptyTraversalObserver, true);
  watch.stop();
  logInfo("Empty traversal took " << watch.getCalendarTime() << "s: " << spacetree.getGridStatistics());

  ::peano4::shutdown();

  return EXIT_SUCCESS;
}
