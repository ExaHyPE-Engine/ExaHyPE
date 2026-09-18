// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridStatisticsTest.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridVertex.h"

peano4::grid::tests::GridStatisticsTest::GridStatisticsTest() :
  TestCase("peano4::grid::tests::GridStatisticsTest") {}

void peano4::grid::tests::GridStatisticsTest::run() {
  testMethod(testStatistics);
  testMethod(testClear);
  testMethod(testOperators);
  testMethod(testMemoryUsage);
}

void peano4::grid::tests::GridStatisticsTest::testStatistics() {
  peano4::grid::GridStatistics statistics;

  validateEquals(statistics.minDepth, std::numeric_limits<int>::max());
  validateEquals(statistics.maxDepth, 0);
  validateEquals(statistics.numberOfLocalUnrefinedCells, 0);
  validateEquals(statistics.numberOfRemoteUnrefinedCells, 0);
  validateEquals(statistics.numberOfLocalRefinedCells, 0);
  validateEquals(statistics.numberOfRemoteRefinedCells, 0);
  validateEquals(statistics.numberOfVertices, 0);
  validateEquals(statistics.numberOfHangingVertices, 0);
  validateEquals(statistics.numberOfStationarySweeps, 0);
  validateEquals(statistics.getCoarseningHasBeenVetoed(), false);
  validateEquals(statistics.getRemovedEmptySubtree(), false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateEquals(statistics.minH[d], std::numeric_limits<double>::max());
  }

  statistics.numberOfLocalUnrefinedCells  = 3;
  statistics.numberOfRemoteUnrefinedCells = 4;
  statistics.numberOfLocalRefinedCells    = 5;
  statistics.numberOfRemoteRefinedCells   = 6;
  statistics.numberOfVertices             = 10;
  statistics.numberOfHangingVertices      = 20;
  statistics.numberOfStationarySweeps     = 7;
  statistics.setCoarseningHasBeenVetoed(true);
  statistics.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    statistics.minH[d] = 0.1 * (d + 1);
  }

  validateEquals(statistics.numberOfLocalUnrefinedCells, 3);
  validateEquals(statistics.numberOfRemoteUnrefinedCells, 4);
  validateEquals(statistics.numberOfLocalRefinedCells, 5);
  validateEquals(statistics.numberOfRemoteRefinedCells, 6);
  validateEquals(statistics.numberOfVertices, 10);
  validateEquals(statistics.numberOfHangingVertices, 20);
  validateEquals(statistics.numberOfStationarySweeps, 7);
  validateEquals(statistics.getCoarseningHasBeenVetoed(), true);
  validateEquals(statistics.getRemovedEmptySubtree(), true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateTrue(std::fabs(statistics.minH[d] - 0.1 * (d + 1)) < 1e-12);
  }
}

void peano4::grid::tests::GridStatisticsTest::testClear() {
  peano4::grid::GridStatistics statistics;

  statistics.numberOfLocalUnrefinedCells  = 1;
  statistics.numberOfRemoteUnrefinedCells = 2;
  statistics.numberOfLocalRefinedCells    = 3;
  statistics.numberOfRemoteRefinedCells   = 4;
  statistics.numberOfVertices             = 100;
  statistics.numberOfHangingVertices      = 200;
  statistics.numberOfStationarySweeps     = 9;
  statistics.setCoarseningHasBeenVetoed(true);
  statistics.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    statistics.minH[d] = 1.0;
  }

  statistics.clear(true);

  validateEquals(statistics.minDepth, std::numeric_limits<int>::max());
  validateEquals(statistics.maxDepth, 0);
  validateEquals(statistics.numberOfLocalUnrefinedCells, 0);
  validateEquals(statistics.numberOfRemoteUnrefinedCells, 0);
  validateEquals(statistics.numberOfLocalRefinedCells, 1);
  validateEquals(statistics.numberOfRemoteRefinedCells, 0);
  validateEquals(statistics.numberOfVertices, 0);
  validateEquals(statistics.numberOfHangingVertices, 0);
  validateEquals(statistics.numberOfStationarySweeps, 10);
  validateEquals(statistics.getCoarseningHasBeenVetoed(), false);
  validateEquals(statistics.getRemovedEmptySubtree(), false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateEquals(statistics.minH[d], std::numeric_limits<double>::max());
  }
}

void peano4::grid::tests::GridStatisticsTest::testOperators() {
  peano4::grid::GridStatistics first;
  peano4::grid::GridStatistics second;

  first.minDepth                     = 3;
  first.maxDepth                     = 5;
  first.numberOfLocalUnrefinedCells  = 1;
  first.numberOfRemoteUnrefinedCells = 2;
  first.numberOfLocalRefinedCells    = 3;
  first.numberOfRemoteRefinedCells   = 4;
  first.numberOfVertices             = 10;
  first.numberOfHangingVertices      = 20;
  first.numberOfStationarySweeps     = 5;
  first.setCoarseningHasBeenVetoed(true);
  first.setRemovedEmptySubtree(false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    first.minH[d] = 0.1 * (d + 2);
  }

  second.minDepth                     = 2;
  second.maxDepth                     = 8;
  second.numberOfLocalUnrefinedCells  = 10;
  second.numberOfRemoteUnrefinedCells = 20;
  second.numberOfLocalRefinedCells    = 30;
  second.numberOfRemoteRefinedCells   = 40;
  second.numberOfVertices             = 100;
  second.numberOfHangingVertices      = 200;
  second.numberOfStationarySweeps     = 50;
  second.setCoarseningHasBeenVetoed(false);
  second.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    second.minH[d] = 0.05 * (d + 1);
  }

  const peano4::grid::GridStatistics sum = first + second;

  validateEquals(sum.minDepth, 2);
  validateEquals(sum.maxDepth, 8);
  validateEquals(sum.numberOfLocalUnrefinedCells, 11);
  validateEquals(sum.numberOfRemoteUnrefinedCells, 22);
  validateEquals(sum.numberOfLocalRefinedCells, 33);
  validateEquals(sum.numberOfRemoteRefinedCells, 44);
  validateEquals(sum.numberOfVertices, 110);
  validateEquals(sum.numberOfHangingVertices, 220);
  validateEquals(sum.numberOfStationarySweeps, 5);

  // The sum takes the or of each flag, so a flag one summand sets is set in the sum.
  validateEquals(sum.getCoarseningHasBeenVetoed(), true);
  validateEquals(sum.getRemovedEmptySubtree(), true);

  for (int d = 0; d < DIMENSIONS; ++d) {
    const double expectedMinH = std::min(0.1 * (d + 2), 0.05 * (d + 1));
    validateTrue(std::fabs(sum.minH[d] - expectedMinH) < 1e-12);
  }

  peano4::grid::GridStatistics accumulated = first;
  accumulated += second;
  validateEquals(accumulated.minDepth, sum.minDepth);
  validateEquals(accumulated.maxDepth, sum.maxDepth);
  validateEquals(accumulated.numberOfLocalUnrefinedCells, sum.numberOfLocalUnrefinedCells);
  validateEquals(accumulated.numberOfRemoteUnrefinedCells, sum.numberOfRemoteUnrefinedCells);
  validateEquals(accumulated.numberOfLocalRefinedCells, sum.numberOfLocalRefinedCells);
  validateEquals(accumulated.numberOfRemoteRefinedCells, sum.numberOfRemoteRefinedCells);
  validateEquals(accumulated.numberOfVertices, sum.numberOfVertices);
  validateEquals(accumulated.numberOfHangingVertices, sum.numberOfHangingVertices);
  validateEquals(accumulated.numberOfStationarySweeps, sum.numberOfStationarySweeps);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateTrue(std::fabs(accumulated.minH[d] - sum.minH[d]) < 1e-12);
  }
  validateEquals(accumulated.getCoarseningHasBeenVetoed(), sum.getCoarseningHasBeenVetoed());
  validateEquals(accumulated.getRemovedEmptySubtree(), sum.getRemovedEmptySubtree());
}

void peano4::grid::tests::GridStatisticsTest::testMemoryUsage() {
  peano4::grid::GridStatistics statistics;

  validateEquals(statistics.getMemoryUsageInBytes(), 0);

  statistics.numberOfVertices = 10;

  const std::size_t expectedBytes = 10 * sizeof(peano4::grid::GridVertex);
  validateEquals(statistics.getMemoryUsageInBytes(), expectedBytes);
}
