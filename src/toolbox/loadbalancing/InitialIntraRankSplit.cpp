// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoadBalancer.h"

#include "peano4/grid/GridStatistics.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "tarch/mpi/Rank.h"
#include "toolbox/loadbalancing/InitialSplitPlan.h"

/**
 * Fills the remaining configured tree slots on each MPI rank.
 *
 * Before uniform construction ends, the phase waits until the global average load can give every configured tree one
 * coarse cell's children. Each participating rank then divides its heaviest local tree among all missing local slots.
 * Splits start only from a coherent snapshot without forks in flight. Adaptive-boundary hanging vertices cannot be
 * forked, so a settled adaptive mesh can leave configured slots unreachable.
 */
class toolbox::loadbalancing::LoadBalancer::InitialIntraRankSplit final
  : public toolbox::loadbalancing::LoadBalancer::Strategy {
public:
  toolbox::loadbalancing::State update(toolbox::loadbalancing::LoadBalancer& loadBalancer) override {
    const auto maxPermittedTreesPerRank = loadBalancer.getMaxPermittedTreesPerRank();
    const auto localNumberOfTrees       = loadBalancer._statistics.getLocalNumberOfTrees();
    const auto configuredNumberOfTrees  = static_cast<std::uint64_t>(loadBalancer.getConfiguredNumberOfTrees());
    const auto globalNumberOfTrees      = static_cast<std::uint64_t>(loadBalancer._statistics.getGlobalNumberOfTrees());
    const auto globalLoad               = loadBalancer._statistics.getGlobalNumberOfInnerUnrefinedCells();
    const auto uniformGridComplete      = loadBalancer.hasUniformGridCompleted();
    const auto globalTreeSetSettled     = loadBalancer._statistics.hasConsistentViewOfWorld()
      and loadBalancer._statistics.getGlobalNumberOfSplits() == 0
      and loadBalancer._statistics.getGlobalNumberOfStationarySweeps() >= 0;
    const auto decompositionImpossible = uniformGridComplete and globalTreeSetSettled
      and (globalLoad <= globalNumberOfTrees or loadBalancer._statistics.hasAdaptiveMesh());

    if ((globalNumberOfTrees >= configuredNumberOfTrees and globalTreeSetSettled) or decompositionImpossible) {
      return toolbox::loadbalancing::State::Stagnation;
    }

    if (maxPermittedTreesPerRank <= 1 or localNumberOfTrees >= maxPermittedTreesPerRank or localNumberOfTrees < 1) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    if (not loadBalancer.mayTriggerSplits()) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    if (loadBalancer._statistics.hasAdaptiveMesh()) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    if (globalLoad / configuredNumberOfTrees < static_cast<std::uint64_t>(loadBalancer.minPieceSizeForInitialSplits())
        and not uniformGridComplete) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    const auto localHeaviestTreeId = loadBalancer.getHeaviestNonBlacklistedLocalSpacetreeId();
    if (localHeaviestTreeId < 0) {
      return toolbox::loadbalancing::State::InitialIntraRankSplit;
    }

    const auto& spacetreeSet          = peano4::parallel::SpacetreeSet::getInstance();
    const auto  localHeaviestTreeLoad = spacetreeSet.getGridStatistics(localHeaviestTreeId).numberOfLocalUnrefinedCells;

    const auto& rank        = tarch::mpi::Rank::getInstance();
    const auto  targetRanks = std::vector<int>(maxPermittedTreesPerRank - localNumberOfTrees, rank.getRank());
    const auto  plan = toolbox::loadbalancing::InitialSplitPlan::planBalancedSplit(localHeaviestTreeLoad, targetRanks);
    loadBalancer.triggerSplits(localHeaviestTreeId, plan);

    return toolbox::loadbalancing::State::InitialIntraRankSplit;
  }
};

std::unique_ptr<toolbox::loadbalancing::LoadBalancer::Strategy>
toolbox::loadbalancing::LoadBalancer::makeInitialIntraRankSplit() {
  return std::make_unique<toolbox::loadbalancing::LoadBalancer::InitialIntraRankSplit>();
}
