// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParallelRegularGrid-main.h"

#include "Constants.h"
#include "CreateGridObserver.h"
#include "EmptyTraversalObserver.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/peano4.h"
#include "tarch/logging/Log.h"
#include "tarch/logging/LogFilter.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"
#include "tarch/multicore/Core.h"

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

  static bool hasSplit = false;
  const int numberOfSpacetreesToSplitInto = tarch::multicore::Core::getInstance().getNumberOfThreads();

  CreateGridObserver createGridObserver;
  while (::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().minH(0) > MinH) {
    const auto numberOfCells = ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().numberOfLocalUnrefinedCells;
    if (not hasSplit and numberOfCells > numberOfSpacetreesToSplitInto) {
      const auto numberOfCellsPerSpacetree = numberOfCells / numberOfSpacetreesToSplitInto;
      for (int i = 0; i < numberOfSpacetreesToSplitInto; i++) {
        ::peano4::parallel::SpacetreeSet::getInstance().split(0, {numberOfCellsPerSpacetree}, 0);
      }
      hasSplit = true;
    }
    watch.start();
    ::peano4::parallel::SpacetreeSet::getInstance().traverse(createGridObserver);
    watch.stop();
    logInfo("main()", "Grid construction took " << watch.getCalendarTime() << "s: " << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics());
  }

  EmptyTraversalObserver emptyTraversalObserver;
  watch.start();
  ::peano4::parallel::SpacetreeSet::getInstance().traverse(emptyTraversalObserver);
  watch.stop();
  logInfo("main()", "Empty traversal took " << watch.getCalendarTime() << "s: " << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics());
  logInfo(
    "main()",
    "Average cell updates per second: "
      << ::peano4::parallel::SpacetreeSet::getInstance().getGridStatistics().numberOfLocalUnrefinedCells / watch.getCalendarTime()
  );

  std::cout << ::peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size() << std::endl;

  ::peano4::shutdown();

  return EXIT_SUCCESS;
}
