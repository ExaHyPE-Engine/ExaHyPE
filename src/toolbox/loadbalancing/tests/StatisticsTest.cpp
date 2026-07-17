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
  testMethod(testLocalSnapshotMatchesSpacetreeSet);
  testMethod(testHasAdaptiveMesh);
  testMethod(testControlledPackedReduction);
  testMethod(testWidePackedReduction);
  testMethod(testDelayedCollectiveParticipant);
}

void toolbox::loadbalancing::tests::StatisticsTest::testLocalSnapshotMatchesSpacetreeSet() {
  toolbox::loadbalancing::Statistics statistics;
  statistics.updateGlobalView();

  const auto& gridStatistics = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics();
  const auto& localTreeIds   = peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees();

  std::uint64_t expectedInnerUnrefinedCells = 0;
  std::uint64_t expectedInnerRefinedCells   = 0;
  std::uint64_t expectedOuterUnrefinedCells = 0;
  std::uint64_t expectedVertices            = 0;
  std::uint64_t expectedHangingVertices     = 0;
  std::uint64_t expectedMemoryUsageInBytes  = 0;
  for (const auto treeId : localTreeIds) {
    const auto& treeStatistics = peano4::parallel::SpacetreeSet::getInstance().getGridStatistics(treeId);
    expectedInnerUnrefinedCells += static_cast<std::uint64_t>(treeStatistics.numberOfLocalUnrefinedCells);
    expectedInnerRefinedCells += static_cast<std::uint64_t>(treeStatistics.numberOfLocalRefinedCells);
    expectedOuterUnrefinedCells += static_cast<std::uint64_t>(treeStatistics.numberOfRemoteUnrefinedCells);
    expectedVertices += static_cast<std::uint64_t>(treeStatistics.numberOfVertices);
    expectedHangingVertices += static_cast<std::uint64_t>(treeStatistics.numberOfHangingVertices);
    expectedMemoryUsageInBytes += static_cast<std::uint64_t>(treeStatistics.getMemoryUsageInBytes());
  }

  validateEquals(statistics.getLocalNumberOfInnerUnrefinedCells(), expectedInnerUnrefinedCells);
  validateEquals(statistics.getLocalNumberOfInnerRefinedCells(), expectedInnerRefinedCells);
  validateEquals(statistics.getGlobalNumberOfInnerRefinedCells(), expectedInnerRefinedCells);
  validateEquals(statistics.getLocalNumberOfOuterUnrefinedCells(), expectedOuterUnrefinedCells);
  validateEquals(statistics.getLocalNumberOfVertices(), expectedVertices);
  validateEquals(statistics.getLocalNumberOfHangingVertices(), expectedHangingVertices);
  validateEquals(statistics.getLocalMemoryUsageInBytes(), expectedMemoryUsageInBytes);
  validateEquals(statistics.getLocalNumberOfTrees(), static_cast<int>(localTreeIds.size()));
  validateEquals(statistics.getLocalMinDepth(), gridStatistics.minDepth);
  validateEquals(statistics.getLocalMaxDepth(), gridStatistics.maxDepth);
  validateEquals(statistics.getLocalNumberOfStationarySweeps(), gridStatistics.numberOfStationarySweeps);
}

void toolbox::loadbalancing::tests::StatisticsTest::testHasAdaptiveMesh() {
  toolbox::loadbalancing::Statistics statistics;

  validate(not statistics.hasAdaptiveMesh());

  statistics._globalNumberOfInnerUnrefinedCells = std::uint64_t{81};
  statistics._globalMinDepth                    = 2;
  statistics._globalMaxDepth                    = 2;
  validate(not statistics.hasAdaptiveMesh());

  statistics._globalMaxDepth = 3;
  validate(statistics.hasAdaptiveMesh());

  statistics._globalNumberOfInnerUnrefinedCells = 0;
  validate(not statistics.hasAdaptiveMesh());
}

void toolbox::loadbalancing::tests::StatisticsTest::testControlledPackedReduction() {
  toolbox::loadbalancing::Statistics statistics;
  statistics.updateGlobalView();

#ifdef PEANO_WITH_MPI
  int mpiIsInitialised = 0;
  MPI_Initialized(&mpiIsInitialised);
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (mpiIsInitialised and numberOfRanks > 1) {
    const auto rankNumber = tarch::mpi::Rank::getInstance().getRank();
    const auto rankValue  = rankNumber + 1;

    statistics._localNumberOfInnerUnrefinedCells = rankValue;
    statistics._localNumberOfInnerRefinedCells   = 5 * rankValue;
    statistics._localNumberOfOuterUnrefinedCells = 2 * rankValue;
    statistics._localNumberOfVertices            = 3 * rankValue;
    statistics._localNumberOfHangingVertices     = 4 * rankValue;
    statistics._localNumberOfTrees               = 1;
    statistics._localNumberOfSplits              = rankValue;
    statistics._localMinTreeLoad                 = 10 + rankNumber;
    statistics._localMaxTreeLoad                 = 42;
    statistics._localMinTreeOuterLoad            = 20 + rankNumber;
    statistics._localMaxTreeOuterLoad            = 30 + rankNumber;
    statistics._localMinDepth                    = 2 + rankNumber;
    statistics._localMaxDepth                    = 3 + rankNumber;
    statistics._localMinH                        = 1.0 / rankValue;
    statistics._localNumberOfStationarySweeps    = rankNumber == numberOfRanks - 1 ? -3 : rankNumber + 2;
    statistics._localUniformGridComplete         = rankNumber == numberOfRanks - 1;
    statistics._localMemoryUsageInBytes          = 100 + rankNumber;
    statistics._localProcessMemoryUsageInBytes   = 200 + rankNumber;
    statistics._localTotalMemoryInBytes          = 1000 + rankNumber;
    statistics._localFreeMemoryInBytes           = 500 + rankNumber;

    statistics.startGlobalViewExchange();
    statistics.synchroniseGlobalView();

    const auto rankValueSum       = numberOfRanks * (numberOfRanks + 1) / 2;
    const auto globalRankValueSum = static_cast<std::uint64_t>(rankValueSum);
    const auto rankOffsetSum =
      static_cast<std::uint64_t>(numberOfRanks) * static_cast<std::uint64_t>(numberOfRanks - 1) / 2;
    validateEquals(statistics.getGlobalNumberOfInnerUnrefinedCells(), globalRankValueSum);
    validateEquals(statistics.getGlobalNumberOfInnerRefinedCells(), 5 * globalRankValueSum);
    validateEquals(statistics.getGlobalNumberOfOuterUnrefinedCells(), 2 * globalRankValueSum);
    validateEquals(statistics.getGlobalNumberOfVertices(), 3 * globalRankValueSum);
    validateEquals(statistics.getGlobalNumberOfHangingVertices(), 4 * globalRankValueSum);
    validateEquals(statistics.getGlobalNumberOfSplits(), rankValueSum);
    validateEquals(statistics.getGlobalNumberOfTrees(), numberOfRanks);
    validateEquals(statistics.getGlobalNumberOfRanksWithAtLeastOneTree(), numberOfRanks);
    validateEquals(statistics.getGlobalMinTreeLoad(), 10);
    validateEquals(statistics.getGlobalMaxTreeLoad(), 42);
    validateEquals(statistics.getGlobalRankWithMaxTreeLoad(), 0);
    validateEquals(statistics.getGlobalMinTreeOuterLoad(), 20);
    validateEquals(statistics.getGlobalMaxTreeOuterLoad(), 30 + numberOfRanks - 1);
    validateEquals(statistics.getGlobalMinRankLoad(), 1);
    validateEquals(statistics.getGlobalMaxRankLoad(), static_cast<std::uint64_t>(numberOfRanks));
    validateEquals(
      statistics.getGlobalMemoryUsageInBytes(),
      static_cast<std::uint64_t>(100 * numberOfRanks) + rankOffsetSum
    );
    validateEquals(
      statistics.getGlobalProcessMemoryUsageInBytes(),
      static_cast<std::uint64_t>(200 * numberOfRanks) + rankOffsetSum
    );
    validateEquals(statistics.getGlobalMinDepth(), 2);
    validateEquals(statistics.getGlobalMaxDepth(), numberOfRanks + 2);
    validateNumericalEquals(statistics.getGlobalMinH(), 1.0 / numberOfRanks);
    validateEquals(statistics.getGlobalNumberOfStationarySweeps(), -3);
    validate(statistics.hasGlobalUniformGridCompleted());

    const auto    isNodeMaster        = tarch::mpi::Rank::getInstance().isNodeMaster();
    const auto    localTotalMemory    = isNodeMaster ? static_cast<std::uint64_t>(1000 + rankNumber) : std::uint64_t{0};
    const auto    localFreeMemory     = isNodeMaster ? static_cast<std::uint64_t>(500 + rankNumber) : std::uint64_t{0};
    std::uint64_t expectedTotalMemory = 0;
    std::uint64_t expectedFreeMemory  = 0;
    const auto    communicator        = tarch::mpi::Rank::getInstance().getCommunicator();
    tarch::mpi::allreduce(&localTotalMemory, &expectedTotalMemory, 1, MPI_UINT64_T, MPI_SUM, communicator);
    tarch::mpi::allreduce(&localFreeMemory, &expectedFreeMemory, 1, MPI_UINT64_T, MPI_SUM, communicator);
    validateEquals(statistics.getGlobalTotalMemoryInBytes(), expectedTotalMemory);
    validateEquals(statistics.getGlobalFreeMemoryInBytes(), expectedFreeMemory);

    return;
  }
#endif

  statistics._localNumberOfSplits = 3;
  statistics.startGlobalViewExchange();
  statistics.synchroniseGlobalView();
  validateEquals(statistics.getGlobalNumberOfSplits(), 3);
}

void toolbox::loadbalancing::tests::StatisticsTest::testWidePackedReduction() {
  toolbox::loadbalancing::Statistics statistics;
  statistics.updateGlobalView();

  constexpr auto LargeLocalCount               = static_cast<std::uint64_t>(std::numeric_limits<int>::max()) + 128;
  statistics._localNumberOfInnerUnrefinedCells = LargeLocalCount;
  statistics._localNumberOfInnerRefinedCells   = LargeLocalCount;
  statistics._localNumberOfOuterUnrefinedCells = LargeLocalCount;
  statistics._localNumberOfVertices            = LargeLocalCount;
  statistics._localNumberOfHangingVertices     = LargeLocalCount;

  validateEquals(statistics.getLocalNumberOfInnerUnrefinedCells(), LargeLocalCount);
  validateEquals(statistics.getLocalNumberOfInnerRefinedCells(), LargeLocalCount);
  validateEquals(statistics.getLocalNumberOfOuterUnrefinedCells(), LargeLocalCount);
  validateEquals(statistics.getLocalNumberOfVertices(), LargeLocalCount);
  validateEquals(statistics.getLocalNumberOfHangingVertices(), LargeLocalCount);

  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (numberOfRanks <= 1) {
    return;
  }

  statistics.startGlobalViewExchange();
  statistics.synchroniseGlobalView();

  const auto expectedGlobalCount = LargeLocalCount * static_cast<std::uint64_t>(numberOfRanks);
  validate(expectedGlobalCount > static_cast<std::uint64_t>(std::numeric_limits<int>::max()));
  validateEquals(statistics.getGlobalNumberOfInnerUnrefinedCells(), expectedGlobalCount);
  validateEquals(statistics.getGlobalNumberOfInnerRefinedCells(), expectedGlobalCount);
  validateEquals(statistics.getGlobalNumberOfOuterUnrefinedCells(), expectedGlobalCount);
  validateEquals(statistics.getGlobalNumberOfVertices(), expectedGlobalCount);
  validateEquals(statistics.getGlobalNumberOfHangingVertices(), expectedGlobalCount);
  validateEquals(statistics.getGlobalMinRankLoad(), LargeLocalCount);
  validateEquals(statistics.getGlobalMaxRankLoad(), LargeLocalCount);
}

void toolbox::loadbalancing::tests::StatisticsTest::testDelayedCollectiveParticipant() {
  toolbox::loadbalancing::Statistics statistics;
  statistics.updateGlobalView();

#ifdef PEANO_WITH_MPI
  int mpiIsInitialised = 0;
  MPI_Initialized(&mpiIsInitialised);
  const auto numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  if (mpiIsInitialised and numberOfRanks > 1) {
    constexpr auto ParticipantDelay = std::chrono::milliseconds(50);
    const auto     rankNumber       = tarch::mpi::Rank::getInstance().getRank();
    const auto     communicator     = tarch::mpi::Rank::getInstance().getCommunicator();

    tarch::mpi::barrier(communicator);
    if (rankNumber == numberOfRanks - 1) {
      std::this_thread::sleep_for(ParticipantDelay);
    }

    statistics._localNumberOfSplits = rankNumber + 1;
    statistics.startGlobalViewExchange();
    statistics.synchroniseGlobalView();

    validateEquals(statistics.getGlobalNumberOfSplits(), numberOfRanks * (numberOfRanks + 1) / 2);
    return;
  }
#endif

  statistics._localNumberOfSplits = 1;
  statistics.startGlobalViewExchange();
  statistics.synchroniseGlobalView();
  validateEquals(statistics.getGlobalNumberOfSplits(), 1);
}
