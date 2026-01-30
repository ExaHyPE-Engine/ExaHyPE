// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridStatisticsTest.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridVertex.h"

peano4::grid::tests::GridStatisticsTest::GridStatisticsTest():
  TestCase("peano4::grid::tests::GridStatisticsTest") {}

void peano4::grid::tests::GridStatisticsTest::run() {
  testMethod(testStatistics);
  testMethod(testClear);
  testMethod(testOperators);
  testMethod(testMemoryUsage);
}

void peano4::grid::tests::GridStatisticsTest::testStatistics() {
  GridStatistics statistics;

  validateEquals(statistics.minDepth, std::numeric_limits<int>::max());
  validateEquals(statistics.maxDepth, -1);
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

  // std::cout << statistics << std::endl;
}

void peano4::grid::tests::GridStatisticsTest::testClear() {
  GridStatistics statistics;

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
  validateEquals(statistics.maxDepth, -1);
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
  GridStatistics a, b;

  a.minDepth                     = 3;
  a.maxDepth                     = 5;
  a.numberOfLocalUnrefinedCells  = 1;
  a.numberOfRemoteUnrefinedCells = 2;
  a.numberOfLocalRefinedCells    = 3;
  a.numberOfRemoteRefinedCells   = 4;
  a.numberOfVertices             = 10;
  a.numberOfHangingVertices      = 20;
  a.numberOfStationarySweeps     = 5;
  a.setCoarseningHasBeenVetoed(true);
  a.setRemovedEmptySubtree(false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    a.minH[d] = 0.1 * (d + 2); // 0.2, 0.3, ...
  }

  b.minDepth                     = 2;
  b.maxDepth                     = 8;
  b.numberOfLocalUnrefinedCells  = 10;
  b.numberOfRemoteUnrefinedCells = 20;
  b.numberOfLocalRefinedCells    = 30;
  b.numberOfRemoteRefinedCells   = 40;
  b.numberOfVertices             = 100;
  b.numberOfHangingVertices      = 200;
  b.numberOfStationarySweeps     = 50;
  b.setCoarseningHasBeenVetoed(false);
  b.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    b.minH[d] = 0.05 * (d + 1); // 0.05, 0.10, ...
  }

  GridStatistics c = a + b;

  validateEquals(c.minDepth, 2);
  validateEquals(c.maxDepth, 8);
  validateEquals(c.numberOfLocalUnrefinedCells, 11);
  validateEquals(c.numberOfRemoteUnrefinedCells, 22);
  validateEquals(c.numberOfLocalRefinedCells, 33);
  validateEquals(c.numberOfRemoteRefinedCells, 44);
  validateEquals(c.numberOfVertices, 110);
  validateEquals(c.numberOfHangingVertices, 220);
  validateEquals(c.numberOfStationarySweeps, 5);

  // flags are OR'ed: a(vetoed=true, removed=false), b(vetoed=false, removed=true) => both true
  validateEquals(c.getCoarseningHasBeenVetoed(), true);
  validateEquals(c.getRemovedEmptySubtree(), true);

  // minH is element-wise min
  for (int d = 0; d < DIMENSIONS; ++d) {
    const double exp = std::min(0.1 * (d + 2), 0.05 * (d + 1));
    validateTrue(std::fabs(c.minH[d] - exp) < 1e-12);
  }

  GridStatistics e = a;
  e += b;
  validateEquals(e.minDepth, c.minDepth);
  validateEquals(e.maxDepth, c.maxDepth);
  validateEquals(e.numberOfLocalUnrefinedCells, c.numberOfLocalUnrefinedCells);
  validateEquals(e.numberOfRemoteUnrefinedCells, c.numberOfRemoteUnrefinedCells);
  validateEquals(e.numberOfLocalRefinedCells, c.numberOfLocalRefinedCells);
  validateEquals(e.numberOfRemoteRefinedCells, c.numberOfRemoteRefinedCells);
  validateEquals(e.numberOfVertices, c.numberOfVertices);
  validateEquals(e.numberOfHangingVertices, c.numberOfHangingVertices);
  validateEquals(e.numberOfStationarySweeps, c.numberOfStationarySweeps);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateTrue(std::fabs(e.minH[d] - c.minH[d]) < 1e-12);
  }
  validateEquals(e.getCoarseningHasBeenVetoed(), c.getCoarseningHasBeenVetoed());
  validateEquals(e.getRemovedEmptySubtree(), c.getRemovedEmptySubtree());
}

void peano4::grid::tests::GridStatisticsTest::testMemoryUsage() {
  GridStatistics statistics;

  validateEquals(statistics.getMemoryUsageInBytes(), 0);

  statistics.numberOfVertices = 10;

  const std::size_t expectedBytes = 10 * sizeof(GridVertex);
  validateEquals(statistics.getMemoryUsageInBytes(), expectedBytes);
}
