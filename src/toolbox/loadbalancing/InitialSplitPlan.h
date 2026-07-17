// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace toolbox::loadbalancing {
  struct InitialSplitPlan;
} // namespace toolbox::loadbalancing

struct toolbox::loadbalancing::InitialSplitPlan final {
  enum class Decomposition { Feasible, InsufficientCells };

  enum class InterRankDistribution { SoleOwner, Partial, Complete };

  struct Request final {
    int numberOfCells;
    int targetRank;
  };

  std::vector<Request> requests;

  static Decomposition
  classifyDecomposition(int configuredNumberOfTrees, std::uint64_t availableNumberOfCells, bool uniformGridComplete);

  static InterRankDistribution classifyInterRankDistribution(int numberOfRanks, int numberOfPopulatedRanks);

  static InitialSplitPlan planBalancedSplit(int sourceLoad, const std::vector<int>& targetRanks);
  static InitialSplitPlan
  planFullInterRankSplit(int sourceLoad, int sourceRank, int numberOfRanks, int configuredNumberOfTrees);

private:
  static std::vector<int> balancedPieceLoads(int totalLoad, int numberOfPieces);
};
