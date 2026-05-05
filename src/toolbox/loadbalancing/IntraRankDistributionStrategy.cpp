// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include <algorithm>
#include <cmath>

class toolbox::loadbalancing::LoadBalancer::IntraRankDistributionStrategy final: public Strategy {
public:
  ~IntraRankDistributionStrategy() override = default;

  State update(LoadBalancer& lb) override {
    const auto numberOfThreads          = tarch::multicore::Core::getInstance().getNumberOfThreads();
    const auto maxPermittedTreesPerRank = lb.getMaxPermittedTreesPerRank();

    if (numberOfThreads <= 1 or lb._statistics.getLocalNumberOfTrees() >= maxPermittedTreesPerRank) {
      return State::Stagnation;
    }

    if (lb._statistics.getNumberOfStateUpdatesWithoutAnySplit() > 5) {
      logWarning("Current local mesh is too small or poorly balanced to split further into trees");
      return State::Stagnation;
    }

    if (lb.hasSplitRecently() or not lb._statistics.hasConsistentViewOfWorld()) {
      return State::IntraRankDistribution;
    }

    const auto localHeaviestTreeId = lb.getHeaviestNonBlacklistedLocalSpacetreeID();
    if (localHeaviestTreeId < 0) {
      return State::IntraRankDistribution;
    }

    const auto localHeaviestTreeLoad = peano4::parallel::SpacetreeSet::getInstance()
                                         .getGridStatistics(localHeaviestTreeId)
                                         .numberOfLocalUnrefinedCells;

    const auto minTreeSize                           = lb._configuration->getMinTreeSize(lb._state);
    const auto numberOfTreesToAccommodateMinTreeSize = std::max(localHeaviestTreeLoad / minTreeSize, 1);

    if (numberOfTreesToAccommodateMinTreeSize < maxPermittedTreesPerRank) {
      return State::IntraRankDistribution;
    }

    const auto numberOfAllowedTrees = std::min({numberOfTreesToAccommodateMinTreeSize, maxPermittedTreesPerRank});
    auto       numberOfSplits       = numberOfAllowedTrees - 1;

    if (numberOfSplits > 0) {
      const auto numberOfCellsPerTree = std::max(
        static_cast<int>(std::round(localHeaviestTreeLoad / numberOfAllowedTrees)),
        1
      );

      auto remainder = localHeaviestTreeLoad - (numberOfCellsPerTree * numberOfAllowedTrees);
      while (numberOfSplits) {
        if (remainder > 0) {
          lb.triggerSplit(numberOfCellsPerTree + 1, tarch::mpi::Rank::getInstance().getRank(), localHeaviestTreeId);
          remainder--;
        } else {
          lb.triggerSplit(numberOfCellsPerTree, tarch::mpi::Rank::getInstance().getRank(), localHeaviestTreeId);
        }
        numberOfSplits--;
      }
    }

    return State::IntraRankDistribution;
  }
};

std::unique_ptr<toolbox::loadbalancing::LoadBalancer::Strategy> toolbox::loadbalancing::LoadBalancer::
  makeIntraRankDistributionStrategy() {
  return std::make_unique<IntraRankDistributionStrategy>();
}
