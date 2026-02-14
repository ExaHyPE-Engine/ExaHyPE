// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StatisticsTest.h"

#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/mpi/Rank.h"
#include "toolbox/loadbalancing/Statistics.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

toolbox::loadbalancing::tests::StatisticsTest::StatisticsTest():
  tarch::tests::TestCase("toolbox::loadbalancing::tests::StatisticsTest") {}

void toolbox::loadbalancing::tests::StatisticsTest::run() {
  testMethod(testPackedReductionMatchesSimpleMPI);
  testMethod(testStateUpdatesWithoutSplitCounter);
}

void toolbox::loadbalancing::tests::StatisticsTest::testPackedReductionMatchesSimpleMPI() {
  toolbox::loadbalancing::Statistics stats;

  // First call: updates local fields and (with MPI>1 rank) starts the non-blocking global exchange.
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
  validate(stats.getLocalMemoryUsageInBytes() >= 0);

  // Capture the local values from this first snapshot. If MPI is enabled with >1 rank,
  // the global fields will correspond to this snapshot after the next updateGlobalView() call.
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

  const int locHasTree = (locTrees > 0) ? 1 : 0;

  // Recompute the local min/max tree load with the same semantics as Statistics::updateGlobalView().
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

#ifdef WITH_MPI
  // Second call: waits for completion of the first non-blocking global exchange and unpacks it.
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1) {
    stats.updateGlobalView();
  }
#endif

#ifdef WITH_MPI
  int mpiInitialised = 0;
  MPI_Initialized(&mpiInitialised);
  if (mpiInitialised) {
    MPI_Comm comm = tarch::mpi::Rank::getInstance().getCommunicator();

    // SUM fields
    int expInner = 0, expOuter = 0, expVerts = 0, expHang = 0, expTrees = 0;
    MPI_Allreduce(&locInner, &expInner, 1, MPI_INT, MPI_SUM, comm);
    MPI_Allreduce(&locOuter, &expOuter, 1, MPI_INT, MPI_SUM, comm);
    MPI_Allreduce(&locVerts, &expVerts, 1, MPI_INT, MPI_SUM, comm);
    MPI_Allreduce(&locHang, &expHang, 1, MPI_INT, MPI_SUM, comm);
    MPI_Allreduce(&locTrees, &expTrees, 1, MPI_INT, MPI_SUM, comm);

    validateEquals(stats.getGlobalNumberOfInnerUnrefinedCells(), expInner);
    validateEquals(stats.getGlobalNumberOfOuterUnrefinedCells(), expOuter);
    validateEquals(stats.getGlobalNumberOfVertices(), expVerts);
    validateEquals(stats.getGlobalNumberOfHangingVertices(), expHang);
    validateEquals(stats.getGlobalNumberOfTrees(), expTrees);

    // Ranks with at least one tree (SUM over 0/1)
    int expRanksWithTrees = 0;
    MPI_Allreduce(&locHasTree, &expRanksWithTrees, 1, MPI_INT, MPI_SUM, comm);
    validateEquals(stats.getGlobalNumberOfRanksWithAtLeastOneTree(), expRanksWithTrees);

    // Memory (SUM)
    std::uint64_t expMem = 0;
    MPI_Allreduce(&locMem, &expMem, 1, MPI_UINT64_T, MPI_SUM, comm);
    validateEquals(static_cast<std::uint64_t>(stats.getGlobalMemoryUsageInBytes()), expMem);

    // Depth (MIN/MAX). For MIN, empty ranks contribute +inf.
    int expMinDepth = 0, expMaxDepth = 0;
    MPI_Allreduce(&locMinDepthPacked, &expMinDepth, 1, MPI_INT, MPI_MIN, comm);
    MPI_Allreduce(&locMaxD, &expMaxDepth, 1, MPI_INT, MPI_MAX, comm);

    if (expRanksWithTrees > 0) {
      validateEquals(stats.getGlobalMinDepth(), expMinDepth);
    }
    validateEquals(stats.getGlobalMaxDepth(), expMaxDepth);

    // MinH (MIN)
    double expMinH = 0.0;
    MPI_Allreduce(&locMinH, &expMinH, 1, MPI_DOUBLE, MPI_MIN, comm);
    validateNumericalEquals(stats.getGlobalMinH(), expMinH);

    // Stationary sweeps (MIN)
    int expSweeps = 0;
    MPI_Allreduce(&locSweeps, &expSweeps, 1, MPI_INT, MPI_MIN, comm);
    validateEquals(stats.getGlobalNumberOfStationarySweeps(), expSweeps);

    // Tree load (MIN/MAX). Empty ranks contribute +inf to MIN.
    int expMinLoad = 0, expMaxLoad = 0;
    MPI_Allreduce(&locMinLoadPacked, &expMinLoad, 1, MPI_INT, MPI_MIN, comm);
    MPI_Allreduce(&locMaxLoad, &expMaxLoad, 1, MPI_INT, MPI_MAX, comm);

    if (expRanksWithTrees > 0) {
      validateEquals(stats.getGlobalMinTreeLoad(), expMinLoad);
    }
    validateEquals(stats.getGlobalMaxTreeLoad(), expMaxLoad);

    // The owner rank for the max load is not deterministic under ties; validate only basic properties.
    const int ranks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const int owner = stats.getGlobalRankWithMaxTreeLoad();
    validate(owner >= 0);
    validate(owner < ranks);

    std::vector<int> gatheredMaxLoads(ranks, 0);
    MPI_Allgather(&locMaxLoad, 1, MPI_INT, gatheredMaxLoads.data(), 1, MPI_INT, comm);
    validateEquals(gatheredMaxLoads[owner], expMaxLoad);

    // Basic invariant (independent of view-consistency heuristics)
    validate(stats.getGlobalNumberOfInnerUnrefinedCells() >= stats.getLocalNumberOfInnerUnrefinedCells());
  }
#else
  // Serial build: global == local
  validateEquals(stats.getGlobalNumberOfInnerUnrefinedCells(), locInner);
  validateEquals(stats.getGlobalNumberOfOuterUnrefinedCells(), locOuter);
  validateEquals(stats.getGlobalNumberOfVertices(), locVerts);
  validateEquals(stats.getGlobalNumberOfHangingVertices(), locHang);
  validateEquals(stats.getGlobalNumberOfTrees(), locTrees);
  validateEquals(stats.getGlobalNumberOfRanksWithAtLeastOneTree(), locHasTree);

  validateEquals(static_cast<std::uint64_t>(stats.getGlobalMemoryUsageInBytes()), locMem);

  if (locHasTree) {
    validateEquals(stats.getGlobalMinTreeLoad(), locMinLoadPacked);
    validateEquals(stats.getGlobalMinDepth(), locMinDepthPacked);
  }
  validateEquals(stats.getGlobalMaxTreeLoad(), locMaxLoad);
  validateEquals(stats.getGlobalMaxDepth(), locMaxD);
  validateNumericalEquals(stats.getGlobalMinH(), locMinH);
  validateEquals(stats.getGlobalNumberOfStationarySweeps(), locSweeps);
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
#ifdef WITH_MPI
  if (tarch::mpi::Rank::getInstance().getNumberOfRanks() > 1 and stats.getNumberOfStateUpdatesWithoutAnySplit() == 0) {
    // With non-blocking exchange, the split can still appear in the global snapshot for one extra iteration.
    stats.updateGlobalView();
  }
#endif
  validate(stats.getNumberOfStateUpdatesWithoutAnySplit() >= 1);
}
