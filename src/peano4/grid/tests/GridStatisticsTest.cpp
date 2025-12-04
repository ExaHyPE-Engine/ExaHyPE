// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridStatisticsTest.h"

#include "peano4/grid/AutomatonState.h"
#include "peano4/grid/GridStatistics.h"
#include "peano4/grid/GridVertex.h"

peano4::grid::tests::GridStatisticsTest::GridStatisticsTest():
  TestCase("peano4::grid::tests::GridStatisticsTest") {}

void peano4::grid::tests::GridStatisticsTest::run() {
  testMethod(testStatistics);
  testMethod(testClear);
  testMethod(testOperators);
  testMethod(testMemoryUsage);
  testMethod(testMPIDatatype);
}

void peano4::grid::tests::GridStatisticsTest::testStatistics() {
  GridStatistics statistics;

  validateEquals(statistics.numberOfLocalUnrefinedCells, 0);
  validateEquals(statistics.numberOfRemoteUnrefinedCells, 0);
  validateEquals(statistics.numberOfLocalRefinedCells, 0);
  validateEquals(statistics.numberOfRemoteRefinedCells, 0);
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
  statistics.numberOfStationarySweeps     = 9;
  statistics.setCoarseningHasBeenVetoed(true);
  statistics.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    statistics.minH[d] = 1.0;
  }

  statistics.clear(true);

  validateEquals(statistics.numberOfLocalUnrefinedCells, 0);
  validateEquals(statistics.numberOfRemoteUnrefinedCells, 0);
  validateEquals(statistics.numberOfLocalRefinedCells, 1);
  validateEquals(statistics.numberOfRemoteRefinedCells, 0);
  validateEquals(statistics.numberOfStationarySweeps, 10);
  validateEquals(statistics.getCoarseningHasBeenVetoed(), false);
  validateEquals(statistics.getRemovedEmptySubtree(), false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateEquals(statistics.minH[d], std::numeric_limits<double>::max());
  }
}

void peano4::grid::tests::GridStatisticsTest::testOperators() {
  GridStatistics a, b;

  a.numberOfLocalUnrefinedCells  = 1;
  a.numberOfRemoteUnrefinedCells = 2;
  a.numberOfLocalRefinedCells    = 3;
  a.numberOfRemoteRefinedCells   = 4;
  a.numberOfStationarySweeps     = 5;
  a.setCoarseningHasBeenVetoed(true);
  a.setRemovedEmptySubtree(false);
  for (int d = 0; d < DIMENSIONS; ++d) {
    a.minH[d] = 0.1 * (d + 2); // 0.2, 0.3, ...
  }

  b.numberOfLocalUnrefinedCells  = 10;
  b.numberOfRemoteUnrefinedCells = 20;
  b.numberOfLocalRefinedCells    = 30;
  b.numberOfRemoteRefinedCells   = 40;
  b.numberOfStationarySweeps     = 50;
  b.setCoarseningHasBeenVetoed(false);
  b.setRemovedEmptySubtree(true);
  for (int d = 0; d < DIMENSIONS; ++d) {
    b.minH[d] = 0.05 * (d + 1); // 0.05, 0.10, ...
  }

  GridStatistics c = a + b;

  validateEquals(c.numberOfLocalUnrefinedCells, 11);
  validateEquals(c.numberOfRemoteUnrefinedCells, 22);
  validateEquals(c.numberOfLocalRefinedCells, 33);
  validateEquals(c.numberOfRemoteRefinedCells, 44);
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
  validateEquals(e.numberOfLocalUnrefinedCells, c.numberOfLocalUnrefinedCells);
  validateEquals(e.numberOfRemoteUnrefinedCells, c.numberOfRemoteUnrefinedCells);
  validateEquals(e.numberOfLocalRefinedCells, c.numberOfLocalRefinedCells);
  validateEquals(e.numberOfRemoteRefinedCells, c.numberOfRemoteRefinedCells);
  validateEquals(e.numberOfStationarySweeps, c.numberOfStationarySweeps);
  for (int d = 0; d < DIMENSIONS; ++d) {
    validateTrue(std::fabs(e.minH[d] - c.minH[d]) < 1e-12);
  }
  validateEquals(e.getCoarseningHasBeenVetoed(), c.getCoarseningHasBeenVetoed());
  validateEquals(e.getRemovedEmptySubtree(), c.getRemovedEmptySubtree());
}

void peano4::grid::tests::GridStatisticsTest::testMemoryUsage() {
  GridStatistics statistics;

  // Case 1: Empty stats => 0 bytes
  validateEquals(statistics.getMemoryUsageInBytes(), 0);

  // Case 2: Add some cells
  statistics.numberOfLocalUnrefinedCells  = 1;
  statistics.numberOfRemoteUnrefinedCells = 2;
  statistics.numberOfLocalRefinedCells    = 3;
  statistics.numberOfRemoteRefinedCells   = 4;

  const std::size_t numberOfCells = static_cast<std::size_t>(statistics.numberOfLocalUnrefinedCells)
                                    + static_cast<std::size_t>(statistics.numberOfLocalRefinedCells)
                                    + static_cast<std::size_t>(statistics.numberOfRemoteUnrefinedCells)
                                    + static_cast<std::size_t>(statistics.numberOfRemoteRefinedCells);
  // Rough, dimension-agnostic vertex estimate used by the implementation.
  const std::size_t estimatedVertices = 2 * (numberOfCells + 1);
  const std::size_t expectedBytes     = numberOfCells * sizeof(AutomatonState) + estimatedVertices * sizeof(GridVertex);
  validateEquals(statistics.getMemoryUsageInBytes(), expectedBytes);
}

void peano4::grid::tests::GridStatisticsTest::testMPIDatatype() {
#ifdef WITH_MPI
  auto&     rank          = tarch::mpi::Rank::getInstance();
  MPI_Comm  comm          = rank.getCommunicator();
  const int me            = rank.getRank();
  const int numberOfRanks = rank.getNumberOfRanks();
  const int tag           = 5678;

  // Build a rank-distinct statistics object
  GridStatistics sendS;
  sendS.numberOfLocalUnrefinedCells  = 1 + me;
  sendS.numberOfRemoteUnrefinedCells = 2 + me;
  sendS.numberOfLocalRefinedCells    = 3 + me;
  sendS.numberOfRemoteRefinedCells   = 4 + me;
  sendS.numberOfStationarySweeps     = 5 + 2 * me;
  sendS.setCoarseningHasBeenVetoed((me % 2) == 0);
  sendS.setRemovedEmptySubtree((me % 3) == 0);

  for (int d = 0; d < DIMENSIONS; ++d) {
    sendS.minH[d] = 0.01 * (d + 1) + 0.0001 * me;
  }

  GridStatistics recvS;

  if (numberOfRanks == 1) {
    MPI_Sendrecv( // TODO: Capture result for error checking
      &sendS, 1, GridStatistics::Datatype, me, tag,
      &recvS, 1, GridStatistics::Datatype, me, tag,
      comm, MPI_STATUS_IGNORE
    );
    // GridStatistics::send(sendS, me, tag, comm);
    // GridStatistics::receive(recvS, me, tag, comm);
  } else {
    const int dest = (me + 1) % numberOfRanks;
    const int src  = (me - 1 + numberOfRanks) % numberOfRanks;
    if ((me % 2) == 0) {
      GridStatistics::send(sendS, dest, tag, comm);
      GridStatistics::receive(recvS, src, tag, comm);
    } else {
      GridStatistics::receive(recvS, src, tag, comm);
      GridStatistics::send(sendS, dest, tag, comm);
    }
  }

  const int expectedSenderRank = (numberOfRanks == 1) ? me : ((me - 1 + numberOfRanks) % numberOfRanks);
  validateEquals(recvS.numberOfLocalUnrefinedCells, 1 + expectedSenderRank);
  validateEquals(recvS.numberOfRemoteUnrefinedCells, 2 + expectedSenderRank);
  validateEquals(recvS.numberOfLocalRefinedCells, 3 + expectedSenderRank);
  validateEquals(recvS.numberOfRemoteRefinedCells, 4 + expectedSenderRank);
  validateEquals(recvS.numberOfStationarySweeps, 5 + 2 * expectedSenderRank);
  validateEquals(recvS.getCoarseningHasBeenVetoed(), ((expectedSenderRank % 2) == 0));
  validateEquals(recvS.getRemovedEmptySubtree(), ((expectedSenderRank % 3) == 0));

  for (int d = 0; d < DIMENSIONS; ++d) {
    const double exp = 0.01 * (d + 1) + 0.0001 * expectedSenderRank;
    validateTrue(std::fabs(recvS.minH[d] - exp) < 1e-12);
  }
#endif
}
