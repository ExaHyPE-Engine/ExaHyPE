// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StatisticsTest.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "toolbox/loadbalancing/Statistics.h"

toolbox::loadbalancing::tests::StatisticsTest::StatisticsTest() :
  tarch::tests::TestCase("toolbox::loadbalancing::tests::StatisticsTest") {}

void toolbox::loadbalancing::tests::StatisticsTest::run() {
  testMethod(testPackedReductionMatchesSimpleMPI);
  testMethod(testStateUpdatesWithoutSplitCounter);
}

void toolbox::loadbalancing::tests::StatisticsTest::testPackedReductionMatchesSimpleMPI() {
  toolbox::loadbalancing::Statistics stats;

  // First call: updates local fields and (with MPI>1 rank) launches the non-blocking global
  // exchange. The reduced global view lags one sweep, so it is only published by the second call
  // (issued below for >1 rank).
  stats.updateGlobalView();

  // Sanity: local view should match the SpacetreeSet's local snapshot
  const auto& gridStats = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  const auto& trees     = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  validateEquals(stats.getLocalNumberOfInnerUnrefinedCells(), gridStats.numberOfLocalUnrefinedCells);
  validateEquals(stats.getLocalNumberOfOuterUnrefinedCells(), gridStats.numberOfRemoteUnrefinedCells);
  validateEquals(stats.getLocalNumberOfVertices(), gridStats.numberOfVertices);
  validateEquals(stats.getLocalNumberOfHangingVertices(), gridStats.numberOfHangingVertices);
  validateEquals(stats.getLocalNumberOfTrees(), static_cast<int>(trees.size()));
  validateEquals(stats.getLocalMinDepth(), gridStats.minDepth);
  validateEquals(stats.getLocalMaxDepth(), gridStats.maxDepth);
  validateEquals(stats.getLocalNumberOfStationarySweeps(), gridStats.numberOfStationarySweeps);

  // Capture the local values from this first snapshot.
  const int           locInner  = stats.getLocalNumberOfInnerUnrefinedCells();
  const int           locOuter  = stats.getLocalNumberOfOuterUnrefinedCells();
  const int           locVerts  = stats.getLocalNumberOfVertices();
  const int           locHang   = stats.getLocalNumberOfHangingVertices();
  const int           locTrees  = stats.getLocalNumberOfTrees();
  const int           locMinD   = stats.getLocalMinDepth();
  const int           locMaxD   = stats.getLocalMaxDepth();
  const int           locSweeps = stats.getLocalNumberOfStationarySweeps();
  const double        locMinH   = stats.getLocalMinH();
  const std::uint64_t locMem    = static_cast<std::uint64_t>(stats.getLocalMemoryUsageInBytes());
  const std::uint64_t locRss    = static_cast<std::uint64_t>(stats.getLocalProcessMemoryUsageInBytes());

  const int locHasTree = (locTrees > 0) ? 1 : 0;

  // Recompute the local min/max tree load with the same semantics as
  // Statistics::updateGlobalView().
  int locMinLoadPacked  = std::numeric_limits<int>::max();
  int locMaxLoad        = 0;
  int locMinDepthPacked = std::numeric_limits<int>::max();
  if (trees.empty()) {
    locMinLoadPacked  = std::numeric_limits<int>::max();
    locMaxLoad        = 0;
    locMinDepthPacked = std::numeric_limits<int>::max();
  } else {
    int localMin = std::numeric_limits<int>::max();
    int localMax = 0;
    for (int t : trees) {
      const int load = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(t).numberOfLocalUnrefinedCells;
      localMin       = std::min(localMin, load);
      localMax       = std::max(localMax, load);
    }
    locMinLoadPacked  = localMin;
    locMaxLoad        = localMax;
    locMinDepthPacked = locMinD;
  }

  const auto validateLocalEqualsGlobal = [&]() {
    validateEquals(stats.getGlobalNumberOfInnerUnrefinedCells(), locInner);
    validateEquals(stats.getGlobalNumberOfOuterUnrefinedCells(), locOuter);
    validateEquals(stats.getGlobalNumberOfVertices(), locVerts);
    validateEquals(stats.getGlobalNumberOfHangingVertices(), locHang);
    validateEquals(stats.getGlobalNumberOfTrees(), locTrees);
    validateEquals(stats.getGlobalNumberOfRanksWithAtLeastOneTree(), locHasTree);

    validateEquals(static_cast<std::uint64_t>(stats.getGlobalMemoryUsageInBytes()), locMem);
    validateEquals(static_cast<std::uint64_t>(stats.getGlobalProcessMemoryUsageInBytes()), locRss);

    if (locHasTree) {
      validateEquals(stats.getGlobalMinTreeLoad(), locMinLoadPacked);
      validateEquals(stats.getGlobalMinDepth(), locMinDepthPacked);
    }
    validateEquals(stats.getGlobalMaxTreeLoad(), locMaxLoad);
    validateEquals(stats.getGlobalMaxDepth(), locMaxD);
    validateNumericalEquals(stats.getGlobalMinH(), locMinH);
    validateEquals(stats.getGlobalNumberOfStationarySweeps(), locSweeps);
  };

#ifdef PEANO_WITH_MPI
  // Second call: completes the non-blocking exchange launched above and publishes the reduced
  // global view (the first call only published the local view because the reduction lags one
  // sweep).
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    stats.updateGlobalView();
  }
#endif

#ifdef PEANO_WITH_MPI
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised and tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    MPI_Comm comm = tarch::mpi::Rank::getInstance().getCommunicator();

    // SUM fields
    int expInner = 0, expOuter = 0, expVerts = 0, expHang = 0, expTrees = 0;
    tarch::mpi::allreduce(&locInner, &expInner, 1, MPI_INT, MPI_SUM, comm);
    tarch::mpi::allreduce(&locOuter, &expOuter, 1, MPI_INT, MPI_SUM, comm);
    tarch::mpi::allreduce(&locVerts, &expVerts, 1, MPI_INT, MPI_SUM, comm);
    tarch::mpi::allreduce(&locHang, &expHang, 1, MPI_INT, MPI_SUM, comm);
    tarch::mpi::allreduce(&locTrees, &expTrees, 1, MPI_INT, MPI_SUM, comm);

    validateEquals(stats.getGlobalNumberOfInnerUnrefinedCells(), expInner);
    validateEquals(stats.getGlobalNumberOfOuterUnrefinedCells(), expOuter);
    validateEquals(stats.getGlobalNumberOfVertices(), expVerts);
    validateEquals(stats.getGlobalNumberOfHangingVertices(), expHang);
    validateEquals(stats.getGlobalNumberOfTrees(), expTrees);

    // Ranks with at least one tree (SUM over 0/1)
    int expRanksWithTrees = 0;
    tarch::mpi::allreduce(&locHasTree, &expRanksWithTrees, 1, MPI_INT, MPI_SUM, comm);
    validateEquals(stats.getGlobalNumberOfRanksWithAtLeastOneTree(), expRanksWithTrees);

    // Memory (SUM)
    std::uint64_t expMem = 0;
    tarch::mpi::allreduce(&locMem, &expMem, 1, MPI_UINT64_T, MPI_SUM, comm);
    validateEquals(static_cast<std::uint64_t>(stats.getGlobalMemoryUsageInBytes()), expMem);

    std::uint64_t expRss = 0;
    tarch::mpi::allreduce(&locRss, &expRss, 1, MPI_UINT64_T, MPI_SUM, comm);
    validateEquals(static_cast<std::uint64_t>(stats.getGlobalProcessMemoryUsageInBytes()), expRss);

    // Depth (MIN/MAX). For MIN, empty ranks contribute +inf.
    int expMinDepth = 0, expMaxDepth = 0;
    tarch::mpi::allreduce(&locMinDepthPacked, &expMinDepth, 1, MPI_INT, MPI_MIN, comm);
    tarch::mpi::allreduce(&locMaxD, &expMaxDepth, 1, MPI_INT, MPI_MAX, comm);

    if (expRanksWithTrees > 0) {
      validateEquals(stats.getGlobalMinDepth(), expMinDepth);
    }
    validateEquals(stats.getGlobalMaxDepth(), expMaxDepth);

    // MinH (MIN)
    double expMinH = 0.0;
    tarch::mpi::allreduce(&locMinH, &expMinH, 1, MPI_DOUBLE, MPI_MIN, comm);
    validateNumericalEquals(stats.getGlobalMinH(), expMinH);

    // Stationary sweeps (MIN)
    int expSweeps = 0;
    tarch::mpi::allreduce(&locSweeps, &expSweeps, 1, MPI_INT, MPI_MIN, comm);
    validateEquals(stats.getGlobalNumberOfStationarySweeps(), expSweeps);

    // Tree load (MIN/MAX). Empty ranks contribute +inf to MIN.
    int expMinLoad = 0, expMaxLoad = 0;
    tarch::mpi::allreduce(&locMinLoadPacked, &expMinLoad, 1, MPI_INT, MPI_MIN, comm);
    tarch::mpi::allreduce(&locMaxLoad, &expMaxLoad, 1, MPI_INT, MPI_MAX, comm);

    if (expRanksWithTrees > 0) {
      validateEquals(stats.getGlobalMinTreeLoad(), expMinLoad);
    }
    validateEquals(stats.getGlobalMaxTreeLoad(), expMaxLoad);

    // The owner rank for the max load is not deterministic under ties; validate only basic
    // properties.
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int owner = stats.getGlobalRankWithMaxTreeLoad();
    validate(owner >= 0);
    validate(owner < ranks);

    std::vector<int> gatheredMaxLoads(ranks, 0);
    tarch::mpi::allgather(&locMaxLoad, 1, MPI_INT, gatheredMaxLoads.data(), 1, MPI_INT, comm);
    validateEquals(gatheredMaxLoads[owner], expMaxLoad);

    // Basic invariant (independent of view-consistency heuristics)
    validate(stats.getGlobalNumberOfInnerUnrefinedCells() >= stats.getLocalNumberOfInnerUnrefinedCells());
  } else {
    validateLocalEqualsGlobal();
  }
#else
  validateLocalEqualsGlobal();
#endif
}

void toolbox::loadbalancing::tests::StatisticsTest::testStateUpdatesWithoutSplitCounter() {
  toolbox::loadbalancing::Statistics stats;

  validateEquals(stats.getNumberOfStateUpdatesWithoutAnySplit(), 0);

  stats.updateGlobalView();
  validateEquals(stats.getNumberOfStateUpdatesWithoutAnySplit(), 1);

  stats.updateGlobalView();
  validateEquals(stats.getNumberOfStateUpdatesWithoutAnySplit(), 2);

  // A (local) split should reset the counter immediately.
  stats.incLocalNumberOfSplits(1);
  validateEquals(stats.getNumberOfStateUpdatesWithoutAnySplit(), 0);

  // The update that carries the split must not increment the counter.
  stats.updateGlobalView();
  validateEquals(stats.getNumberOfStateUpdatesWithoutAnySplit(), 0);

  // After enough updates without splits, the counter must start increasing again.
  stats.updateGlobalView();
#ifdef PEANO_WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and stats.getNumberOfStateUpdatesWithoutAnySplit() == 0) {
    // With the non-blocking exchange the lagged split still shows up in the global snapshot for one
    // extra iteration, resetting the counter once more; one more update lets it climb again.
    stats.updateGlobalView();
  }
#endif
  validate(stats.getNumberOfStateUpdatesWithoutAnySplit() >= 1);
}
