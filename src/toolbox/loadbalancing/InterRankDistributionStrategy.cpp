// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"

#include <algorithm>
#include <cmath>
class toolbox::loadbalancing::LoadBalancer::InterRankDistributionStrategy final: public Strategy {
public:
  ~InterRankDistributionStrategy() override = default;

  State update(LoadBalancer& lb) override {
    const auto numberOfRanks                       = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    const auto numberOfRanksThatHoldAtLeastOneTree = lb._statistics.getGlobalNumberOfRanksWithAtLeastOneTree();
    const auto numberOfFreeRanks                   = numberOfRanks - numberOfRanksThatHoldAtLeastOneTree;

    if (numberOfRanks <= 1 or numberOfFreeRanks <= 0) {
      return State::IntraRankDistribution;
    }

    if (lb._statistics.getLocalNumberOfTrees() < 1 or lb.hasSplitRecently()
        or not lb._statistics.hasConsistentViewOfWorld()) {
      return State::InterRankDistribution;
    }

    const auto localHeaviestTreeId = lb.getHeaviestNonBlacklistedLocalSpacetreeID();
    if (localHeaviestTreeId < 0) {
      return State::InterRankDistribution;
    }

    const auto localHeaviestTreeLoad = peano4::parallel::SpacetreeSet::getInstance()
                                         .getGridStatistics(localHeaviestTreeId)
                                         .numberOfLocalUnrefinedCells;

    // Check if we are heavy enough to matter
    // We want to reduce our tree until it matches the average load per rank.
    const auto globalTotalCells  = lb._statistics.getGlobalNumberOfInnerUnrefinedCells();
    const auto idealCellsPerRank = std::max(1, static_cast<int>(globalTotalCells / numberOfRanks));

    // If we are already close to (or below) the ideal, don't split.
    if (localHeaviestTreeLoad < idealCellsPerRank * 1.1) {
      return State::InterRankDistribution;
    }

    // We target the first free rank, then jump by the number of currently active ranks.
    // This ensures that if Rank 0 and Rank 1 both split, they target different free ranks.
    const auto firstFreeRank = numberOfRanksThatHoldAtLeastOneTree;
    const auto stride        = numberOfRanksThatHoldAtLeastOneTree;
    const auto myRank        = tarch::mpi::Rank::getInstance().getRank();

    // Ranks holding trees form a prefix [0..K-1]
    assertion(myRank < numberOfRanksThatHoldAtLeastOneTree);

    // Count how many targets I will actually hit in this loop
    int availableTargets = 0;
    for (int t = firstFreeRank + myRank; t < numberOfRanks; t += stride) {
      availableTargets++;
    }
    if (availableTargets == 0) {
      return State::InterRankDistribution;
    }

    // We treat the current load as a pool to be shared evenly among (availableTargets + Me).
    auto participants = availableTargets + 1; // +1 for myself
    auto currentLoad  = localHeaviestTreeLoad;

    for (int targetRank = firstFreeRank + myRank; targetRank < numberOfRanks; targetRank += stride) {
      // Divide the current remaining load by the remaining participants.
      // Example Phase 1 (Load 9, 9 Participants):
      //   1. Rank A: 9/9 = 1. Remaining 8.
      //   2. Rank B: 8/8 = 1. Remaining 7.
      // Example Phase 2 (Load 81, 2 Participants):
      //   1. Rank A: 81/2 = 40. Remaining 41.
      int cellsToSplit = currentLoad / participants;
      if (cellsToSplit < 1) {
        cellsToSplit = 1;
      }
      if (cellsToSplit >= currentLoad) {
        cellsToSplit = currentLoad - 1;
      }

      // If we have nothing meaningful to give, stop.
      if (currentLoad <= 1 or cellsToSplit <= 0) {
        break;
      }

      logDebug(
        "Splitting "
        << cellsToSplit << " cells"
        << " from tree " << localHeaviestTreeId << " on rank " << myRank << " to free rank " << targetRank
        << " (current load=" << currentLoad << ", participants remaining=" << participants - 1 << ")"
      );

      lb.triggerSplit(cellsToSplit, targetRank, localHeaviestTreeId);

      currentLoad -= cellsToSplit;
      participants--;
    }

    return State::InterRankDistribution;
  }
};

std::unique_ptr<toolbox::loadbalancing::LoadBalancer::Strategy> toolbox::loadbalancing::LoadBalancer::
  makeInterRankDistributionStrategy() {
  return std::make_unique<InterRankDistributionStrategy>();
}
