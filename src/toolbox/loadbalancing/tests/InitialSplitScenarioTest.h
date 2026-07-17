// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace toolbox::loadbalancing {
  struct InitialSplitPlan;
} // namespace toolbox::loadbalancing

namespace toolbox::loadbalancing::tests {
  class InitialSplitScenarioTest;
} // namespace toolbox::loadbalancing::tests

class toolbox::loadbalancing::tests::InitialSplitScenarioTest final : public tarch::tests::TestCase {
public:
  InitialSplitScenarioTest();
  void run() override;

private:
  void testDecompositionClassification();
  void testInterRankDistributionClassification();
  void testBalancedSplitPlan();
  void testFullInterRankRequestOrder();
  void testFullInterRankSplitBoundaries();
  void testTenRankDecomposition();
  void testLargeDecomposition();

  struct PlannedDecomposition final {
    std::vector<int> treesPerRank;
    std::vector<int> cellsPerRank;
  };

  static PlannedDecomposition
  applyPlan(int sourceLoad, int sourceRank, int numberOfRanks, const toolbox::loadbalancing::InitialSplitPlan& plan);
};
