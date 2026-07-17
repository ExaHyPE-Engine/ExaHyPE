// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "InitialSplitScenarioTest.h"

#include "toolbox/loadbalancing/InitialSplitPlan.h"

toolbox::loadbalancing::tests::InitialSplitScenarioTest::InitialSplitScenarioTest() :
  tarch::tests::TestCase("toolbox::loadbalancing::tests::InitialSplitScenarioTest") {}

toolbox::loadbalancing::tests::InitialSplitScenarioTest::PlannedDecomposition
toolbox::loadbalancing::tests::InitialSplitScenarioTest::applyPlan(
  const int                                       sourceLoad,
  const int                                       sourceRank,
  const int                                       numberOfRanks,
  const toolbox::loadbalancing::InitialSplitPlan& plan
) {
  toolbox::loadbalancing::tests::InitialSplitScenarioTest::PlannedDecomposition result{
    std::vector<int>(numberOfRanks, 0),
    std::vector<int>(numberOfRanks, 0)
  };
  result.treesPerRank[sourceRank] = 1;
  result.cellsPerRank[sourceRank] = sourceLoad;
  for (const auto& request : plan.requests) {
    ++result.treesPerRank[request.targetRank];
    result.cellsPerRank[sourceRank] -= request.numberOfCells;
    result.cellsPerRank[request.targetRank] += request.numberOfCells;
  }
  return result;
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::run() {
  testMethod(testDecompositionClassification);
  testMethod(testInterRankDistributionClassification);
  testMethod(testBalancedSplitPlan);
  testMethod(testFullInterRankRequestOrder);
  testMethod(testFullInterRankSplitBoundaries);
  testMethod(testTenRankDecomposition);
  testMethod(testLargeDecomposition);
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testDecompositionClassification() {
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyDecomposition(256, 81, false)
    == toolbox::loadbalancing::InitialSplitPlan::Decomposition::Feasible
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyDecomposition(256, 81, true)
    == toolbox::loadbalancing::InitialSplitPlan::Decomposition::InsufficientCells
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyDecomposition(256, 256, true)
    == toolbox::loadbalancing::InitialSplitPlan::Decomposition::Feasible
  );
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testInterRankDistributionClassification() {
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(10, 1)
    == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::SoleOwner
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(10, 2)
    == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Partial
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(10, 9)
    == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Partial
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(10, 10)
    == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Complete
  );
  validate(
    toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(1, 1)
    == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Complete
  );
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testBalancedSplitPlan() {
  const auto targetRanks = std::vector<int>(7, 4);
  const auto plan        = toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(90, targetRanks);
  validateEquals(static_cast<int>(plan.requests.size()), 7);

  int requestedLoad = 0;
  for (const auto& request : plan.requests) {
    validate(request.numberOfCells == 11 or request.numberOfCells == 12);
    validateEquals(request.targetRank, 4);
    requestedLoad += request.numberOfCells;
  }
  validateEquals(90 - requestedLoad, 11);

  const auto cappedPlan = toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(5, std::vector<int>(8, 3));
  validateEquals(static_cast<int>(cappedPlan.requests.size()), 4);
  for (const auto& request : cappedPlan.requests) {
    validateEquals(request.numberOfCells, 1);
    validateEquals(request.targetRank, 3);
  }

  validate(toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(0, std::vector<int>{0}).requests.empty());
  validate(toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(1, std::vector<int>{0}).requests.empty());
  validate(toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(5, std::vector<int>{}).requests.empty());
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testFullInterRankRequestOrder() {
  constexpr int                SourceLoad = 81;
  constexpr std::array<int, 5> ExpectedLoads{14, 14, 13, 13, 14};
  constexpr std::array<int, 5> ExpectedRanks{0, 0, 1, 1, 1};
  const auto plan         = toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(SourceLoad, 0, 2, 6);
  const auto repeatedPlan = toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(SourceLoad, 0, 2, 6);

  validateEquals(plan.requests.size(), ExpectedLoads.size());
  validateEquals(repeatedPlan.requests.size(), plan.requests.size());
  for (std::size_t requestIndex = 0; requestIndex < plan.requests.size(); ++requestIndex) {
    validateEquals(plan.requests[requestIndex].numberOfCells, ExpectedLoads[requestIndex]);
    validateEquals(plan.requests[requestIndex].targetRank, ExpectedRanks[requestIndex]);
    validateEquals(repeatedPlan.requests[requestIndex].numberOfCells, plan.requests[requestIndex].numberOfCells);
    validateEquals(repeatedPlan.requests[requestIndex].targetRank, plan.requests[requestIndex].targetRank);
  }

  const auto decomposition = applyPlan(SourceLoad, 0, 2, plan);
  validateEquals(decomposition.treesPerRank[0], 3);
  validateEquals(decomposition.treesPerRank[1], 3);
  validateEquals(decomposition.cellsPerRank[0], 41);
  validateEquals(decomposition.cellsPerRank[1], 40);
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testFullInterRankSplitBoundaries() {
  validate(toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(1, 0, 10, 256).requests.empty());

  constexpr int SourceLoad    = 3;
  constexpr int SourceRank    = 9;
  constexpr int NumberOfRanks = 10;
  const auto    plan =
    toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(SourceLoad, SourceRank, NumberOfRanks, 256);
  const auto decomposition = applyPlan(SourceLoad, SourceRank, NumberOfRanks, plan);

  validateEquals(static_cast<int>(plan.requests.size()), SourceLoad - 1);
  for (int rank = 0; rank < NumberOfRanks; ++rank) {
    const auto expectedLoad = rank == 0 or rank == 1 or rank == SourceRank ? 1 : 0;
    validateEquals(decomposition.treesPerRank[rank], expectedLoad);
    validateEquals(decomposition.cellsPerRank[rank], expectedLoad);
  }

  const auto oneRankPlan          = toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(81, 0, 1, 8);
  const auto oneRankDecomposition = applyPlan(81, 0, 1, oneRankPlan);
  validateEquals(static_cast<int>(oneRankPlan.requests.size()), 7);
  validateEquals(oneRankDecomposition.treesPerRank[0], 8);
  validateEquals(oneRankDecomposition.cellsPerRank[0], 81);
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testTenRankDecomposition() {
  constexpr int NumberOfRanks       = 10;
  constexpr int TreesPerRank        = 2;
  constexpr int SourceLoad          = 6561;
  constexpr int ConfiguredTreeCount = NumberOfRanks * TreesPerRank;
  const auto    plan =
    toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(SourceLoad, 0, NumberOfRanks, ConfiguredTreeCount);
  const auto decomposition = applyPlan(SourceLoad, 0, NumberOfRanks, plan);

  validateEquals(static_cast<int>(plan.requests.size()), ConfiguredTreeCount - 1);
  for (const auto numberOfTrees : decomposition.treesPerRank) {
    validateEquals(numberOfTrees, TreesPerRank);
  }
  validateEquals(std::accumulate(decomposition.cellsPerRank.begin(), decomposition.cellsPerRank.end(), 0), SourceLoad);
  const auto [minimumRankLoad, maximumRankLoad] =
    std::minmax_element(decomposition.cellsPerRank.begin(), decomposition.cellsPerRank.end());
  validate(*maximumRankLoad - *minimumRankLoad <= 1);
}

void toolbox::loadbalancing::tests::InitialSplitScenarioTest::testLargeDecomposition() {
  constexpr int NumberOfRanks       = 16;
  constexpr int TreesPerRank        = 16;
  constexpr int ConfiguredTreeCount = NumberOfRanks * TreesPerRank;
  constexpr int SourceLoad          = 59049;

  const auto localPlan = toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(
    SourceLoad,
    std::vector<int>(ConfiguredTreeCount - 1, 0)
  );
  validateEquals(static_cast<int>(localPlan.requests.size()), ConfiguredTreeCount - 1);
  int retainedLoad    = SourceLoad;
  int minimumTreeLoad = SourceLoad;
  int maximumTreeLoad = 0;
  for (const auto& request : localPlan.requests) {
    retainedLoad -= request.numberOfCells;
    minimumTreeLoad = std::min(minimumTreeLoad, request.numberOfCells);
    maximumTreeLoad = std::max(maximumTreeLoad, request.numberOfCells);
  }
  minimumTreeLoad = std::min(minimumTreeLoad, retainedLoad);
  maximumTreeLoad = std::max(maximumTreeLoad, retainedLoad);
  validate(retainedLoad > 0);
  validate(maximumTreeLoad - minimumTreeLoad <= 1);

  const auto interRankPlan =
    toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(SourceLoad, 0, NumberOfRanks, ConfiguredTreeCount);
  const auto decomposition = applyPlan(SourceLoad, 0, NumberOfRanks, interRankPlan);
  validateEquals(static_cast<int>(interRankPlan.requests.size()), ConfiguredTreeCount - 1);
  for (const auto numberOfTrees : decomposition.treesPerRank) {
    validateEquals(numberOfTrees, TreesPerRank);
  }
  const auto [minimumRankLoad, maximumRankLoad] =
    std::minmax_element(decomposition.cellsPerRank.begin(), decomposition.cellsPerRank.end());
  validate(*maximumRankLoad - *minimumRankLoad <= 1);
}
