// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Rank.h"
#include "toolbox/loadbalancing/InitialSplitPlan.h"

/**
 * Populates MPI ranks before assigning the remaining tree slots within each rank.
 *
 * The sole source rank creates all configured tree slots in one shot when the mesh is large enough. While those
 * pieces are too small, it first gives every rank one balanced piece so uniform refinement proceeds in parallel. A
 * settled partial rank distribution is fatal because independent continuation has no bound on the rank imbalance. A
 * newly forked rank refines once before splitting again because only cells whose coarse parent it owns are eligible
 * split candidates. Startup verifies a common trees-per-rank target, so every rank derives the same split
 * thresholds.
 */
class toolbox::loadbalancing::LoadBalancer::InitialInterRankSplit final
  : public toolbox::loadbalancing::LoadBalancer::Strategy {
public:
  toolbox::loadbalancing::State update(toolbox::loadbalancing::LoadBalancer& loadBalancer) override {
    const auto& rank                   = tarch::mpi::Rank::getInstance();
    const auto  numberOfRanks          = rank.getNumberOfRanks();
    const auto  numberOfPopulatedRanks = loadBalancer._statistics.getGlobalNumberOfRanksWithAtLeastOneTree();
    const auto  interRankDistribution =
      toolbox::loadbalancing::InitialSplitPlan::classifyInterRankDistribution(numberOfRanks, numberOfPopulatedRanks);

    if (interRankDistribution == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Complete) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    if (loadBalancer._statistics.getLocalNumberOfTrees() < 1) {
      return loadBalancer.hasUniformGridCompleted() ? toolbox::loadbalancing::State::InitialIntraRankSplit
                                                    : toolbox::loadbalancing::State::InitialInterRankSplit;
    }

    if (not loadBalancer.mayTriggerSplits()) {
      return toolbox::loadbalancing::State::InitialInterRankSplit;
    }

    if (interRankDistribution == toolbox::loadbalancing::InitialSplitPlan::InterRankDistribution::Partial) {
      tarch::terminateWithError(
        "Initial inter-rank split settled with " + std::to_string(numberOfPopulatedRanks) + " of "
        + std::to_string(numberOfRanks) + " ranks populated."
      );
    }

    if (loadBalancer._statistics.hasAdaptiveMesh()) {
      return loadBalancer.hasUniformGridCompleted() ? toolbox::loadbalancing::State::InitialIntraRankSplit
                                                    : toolbox::loadbalancing::State::InitialInterRankSplit;
    }

    const auto localHeaviestTreeId = loadBalancer.getHeaviestNonBlacklistedLocalSpacetreeId();
    if (localHeaviestTreeId < 0) {
      return loadBalancer.hasUniformGridCompleted() ? toolbox::loadbalancing::State::InitialIntraRankSplit
                                                    : toolbox::loadbalancing::State::InitialInterRankSplit;
    }

    const auto& spacetreeSet          = peano4::parallel::SpacetreeSet::getInstance();
    const auto  localHeaviestTreeLoad = spacetreeSet.getGridStatistics(localHeaviestTreeId).numberOfLocalUnrefinedCells;

    return decomposeSoleOwner(loadBalancer, localHeaviestTreeId, localHeaviestTreeLoad, rank.getRank(), numberOfRanks);
  }

private:
  /**
   * When a piece for every configured tree would be too small, a rank-only split exposes refinement parallelism.
   * Initial forks stay on the uniform mesh because adaptive boundaries contain hanging vertices that cannot be
   * forked faithfully.
   */
  static toolbox::loadbalancing::State decomposeSoleOwner(
    toolbox::loadbalancing::LoadBalancer& loadBalancer,
    const int                             sourceTreeId,
    const int                             sourceLoad,
    const int                             sourceRank,
    const int                             numberOfRanks
  ) {
    const auto configuredNumberOfTrees = loadBalancer.getConfiguredNumberOfTrees();
    const auto minimumPieceSize        = loadBalancer.minPieceSizeForInitialSplits();

    if (sourceLoad / configuredNumberOfTrees >= minimumPieceSize or loadBalancer.hasUniformGridCompleted()) {
      const auto plan = toolbox::loadbalancing::InitialSplitPlan::planFullInterRankSplit(
        sourceLoad,
        sourceRank,
        numberOfRanks,
        configuredNumberOfTrees
      );
      loadBalancer.triggerSplits(sourceTreeId, plan);
      return toolbox::loadbalancing::State::InitialInterRankSplit;
    }

    if (sourceLoad / numberOfRanks >= minimumPieceSize) {
      std::vector<int> targetRanks;
      targetRanks.reserve(numberOfRanks - 1);
      for (int targetRank = 0; targetRank < numberOfRanks; ++targetRank) {
        if (targetRank != sourceRank) {
          targetRanks.push_back(targetRank);
        }
      }
      const auto plan = toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(sourceLoad, targetRanks);
      loadBalancer.triggerSplits(sourceTreeId, plan);
    }

    return toolbox::loadbalancing::State::InitialInterRankSplit;
  }
};

std::unique_ptr<toolbox::loadbalancing::LoadBalancer::Strategy>
toolbox::loadbalancing::LoadBalancer::makeInitialInterRankSplit() {
  return std::make_unique<toolbox::loadbalancing::LoadBalancer::InitialInterRankSplit>();
}
