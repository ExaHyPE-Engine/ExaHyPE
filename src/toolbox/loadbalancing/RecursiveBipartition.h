// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "LoadBalancing.h"


namespace toolbox {
  namespace loadbalancing {
    class RecursiveBipartition;
  }
}


/**
 * Recursive load balancing micking the behaviour of the guided partitioning in OpenMP
 *
 * This is one of the simplest load balancing strategies one can think about. We
 * try to mimick the behaviour of OpenMP's guided scheduling: We determine the
 * maximum number of unrefined inner cells around and compute an optimal load per
 * rank. We first spread out over all ranks, and then ensure that there is at least
 * one tree per core. From thereon, we examine whether we violate the per-rank
 * load balancing. If a rank does this, it splits its heaviest tree and deploys the
 * new subtree to the most underbooked rank.
 *
 *
 * <h2> Implementation remarks </h2>
 *
 * The strategy runs through different phases.
 *
 * <h3> Phase 1: The tree is too small so there would not be enough cells if we had even only one cell per tree </h3>
 *
 * We wait and do nothing. After all, this might be the first step of the grid
 * construction, so we are well-advised to wait for a few iterations to get
 * an idea what the final grid will look like and in particular to get enough
 * cells so we can keep all threads/ranks busy.
 *
 * <h3> Phase 2: We have not yet used all ranks </h3>
 *
 * First of all, we make all ranks busy. This can be done in one rush: Rank 0
 * splits up equally sized tree to each and every rank. As phase 1 has
 * terminated already, we know that there are enough cells around.
 *
 * <h3> Phase 3: Get reasonably many cores into the game </h3>
 *
 * We'd like to occupy a lot of cores. We should trigger
 * this step asap. At the same time, we might already be in a situation where
 * the per-rank mesh is pretty detailed, i.e. too many forks in one rush might
 * be too ambitious.
 *
 * This phase still is rank-local, i.e. I do not spread between ranks.
 * Consequently, I should not overdecompose. It might be reasonable to wait
 * with further splits and to balance later on a case-by-case base such that
 * we can deploy cells to other ranks, too. This throttling is implemented via
 * getNumberOfSplitsOnLocalRank().
 *
 * <h3> Phase 4: Load balancing between the ranks (dynamic) </h3>
 *
 * This is the dynamic load balancing phase. Take the biggest tree, check
 * whether it is too heavy and split it. When you split it, search for the
 * rank with the lowest total load and get the new subtree there. To avoid
 * that too many ranks deploy to one rank at the same time, we use kind of
 * a round-robin token here.
 *
 * <h2> Broad vs. deep trees </h2>
 *
 * @todo Update this part with new insight
 *
 * The splitting has to be reasonably aggressive. If we worked iteratively
 * depth first, i.e. identified the biggest tree, split it, and then continue,
 * then we run into situations where we have a complex master-worker
 * topology. This restricts our flexibility as we have to preserve the
 * topology. The broader a tree the better.
 *
 * Therefore, it is absolutely mandatory (and it took me a while to recognise
 * this) to use the min functions and always try to split from a very heavy
 * tree just that part that you need.
 *
 *
 * <h2> Cool-down phases </h2>
 *
 * It is important that we have some kind of cool-down phases, i.e. certain blocks of
 * sweeps where no rebalancing is done no matter how imbalanced we are. Splits are
 * notoriously expensive as they serialise parts of the traversals. Furthermore, we
 * will never reach a stationary grid throughout the grid construction phase if we
 * split all the time (and perhaps then are unsuccessful as the grids have a layout
 * that we can't split actually).
 */
class toolbox::loadbalancing::RecursiveBipartition: public toolbox::loadbalancing::LoadBalancing {
  public:
    /**
     * @see getAction()
     */
    enum class Action {
      /**
       * Required for the strategy.
       */
      Unspecified,
      None,
      SpreadEquallyOverAllRanks,
      SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_UseLocalRank_UseRecursivePartitioning
    };

    static std::string toString( Action action );


    /**
     * Set up recursive subdivision
     *
     * The ownership of the pointer goes over to the recursive subdivision
     * instance, i.e. you don't have to delete the passed object.
     */
    RecursiveBipartition(Configuration* configuration = new DefaultConfiguration());
    virtual ~RecursiveBipartition();

    /**
     * Triggers actual load balancing data exchange, triggers rebalancing, and
     * dumps statistics.
     *
     * <h2> Spread equally </h2>
     * This step is usually called early throughout the grid construction. At
     * this point, we want to bring in ranks as soon as possible. However, it
     * is very unlikely that the grid at that point will allow us to balance
     * properly. We will likely have something like 27 cells for 8 ranks. With
     * integer arithmetics, we would now get 3 cells per rank with the
     * remainder of cells remaining on rank 0. This is not a big issue for
     * small meshes, but for large meshes even a tiny overloading of rank 0 can
     * result in an out-of-memory situation. I therefore have this module
     * increment within the for loop such that module imbalances are at least
     * spread out.
     */
    void finishStep();

    /**
     * I need the stats here mainly for debugging purposes. The load balancing
     * alread dumps information per time step. This routine however is more
     * elaborate/detailed and not used by default.
     */
    std::string toString() const;

  private:
    static tarch::logging::Log  _log;

    int _stepsToWaitForNextLoadBalancingDecision;

    /**
     * @see updateState()
     */
    int _roundRobinToken;

    /**
     * This operation checks if the inter-rank load decomposition does
     * violate the overall load balancing constraints.
     */
    bool doesRankViolateBalancingCondition() const;

    /**
     * Translate the state and further global knowledge into actions
     *
     *
     * ## InterRankDistribution
     *
     * This is the initial state, where our ambition has to be to get as many
     * ranks into the computation as quickly as possible. The only constraint
     * to take into account is that the user might have defined a certain
     * minimal tree size that they want to use. So as long as the local cell
     * count times the number of ranks is bigger than this minimal tree size,
     * we can spread out, i.e. return Action::SpreadEquallyOverAllRanks.
     *
     * There are a few further exceptions to the rule: If we have less then two
     * ranks, we ignore the step and rely on updateState() to toggle the state
     * so we start another strategy next. If we are not the global master, we
     * don't do anything either. It is the global master that has to decide.
     * Once it has spread out, the global number of trees will eventually
     * become bigger than one and we will leave the InterRankDistribution
     * state. But this is not our job. It is realised within updateState(). If
     * we are on the global master and have just spread out, we should not do
     * anything either. It needs at least two cycles to realise the initial
     * domain decomposition, so we should not trigger the spreading twice in
     * a row - otherwise the global rank zero will be left empty. So if we have
     * just split on the global master, lets just return None and wait for one
     * more cycle.
     *
     * Load balancing recommendation: Most load balancing configurations set
     * the minimal tree size for this stage to zero, so they simply say "spread
     * out as aggressively as possible, we care about minimal mesh sizes per
     * subdomain later".
     *
     */
    Action getAction() const;

    /**
     * Wrapper around the spacetree set which also updates the blacklist.
     *
     * ## Rationale
     *
     * I originally wanted to add an assertion
     *
     *       assertionEquals(_stepsToWaitForNextLoadBalancingDecision,0);
     *
     * which is correct for the very first split. However, if a tree splits
     * multiple times, then the first split will set this counter to something
     * greater to 0 and therefore the assertion will fail for the next split.
     * So I removed this statement.
     *
     *
     */
    void triggerSplit( int sourceTree, int numberOfCells, int targetRank );

    /**
     * <h2> Init has-spread-over-all-ranks flag </h2>
     *
     * By the time we construct the load balancing, we often haven't
     * initialised the MPI environment properly yet. At least, I don't want
     * to rely on this. Therefore, I set _hasSpreadOutOverAllRanks to false
     * by default, and then make updateGlobalView() set it to true for the
     * non-0 rank. The 0-rank case is covered anyway.
     *
     * <h2> Round-robin balancing </h2>
     *
     * I test my load balancing typically first with a rather regular grid.
     * What happens here is that the grid is decomposed and one or two ranks
     * are slightly ``too light''. All the others identify a rank to which
     * they could outsource cells and then all outsource to the same ranks
     * at the same time. To avoid this, I introduce a round-robin token such
     * that only one rank at a time does balance in-between ranks (the other
     * types of lb are not affected). This way, it cannot happen that a high
     * number of ranks all outsource cells to the same ''victim'' in one
     * rush.
     *
     * <h2> Stagnation </h2>
     *
     * Weird things can happen with our greedy approach: We might have three
     * heavy trees on our rank which all cover regions that cannot be forked
     * further (as they have already children or cover boundaries or some 
     * AMR regions). In this case, it might happen that we rotate through: We
     * ask tree 1 to split, then tree 2, then 3. Once the split of 3 has 
     * finished (unsuccessfully), 1 has already left the blacklist. We end up
     * with a cycle.
     *
     * Therefore, we increase the time how long a tree stays on the blacklist
     * everytime the maximum tree weight has not changed. This way, we avoid
     * the rotations.
     *
     * If everybody is on the blacklist, then we have obviously tried to 
     * split every tree and it did not really work, so we would like to split 
     * further, but we have run into a stagnation.
     */
    void updateState();

    /**
     * Core actions where we take the action and translate it into action -
     * what a pun.
     *
     * Please call this routine before you invoke updateState().
     */
    void updateLoadBalancing();

    bool isLocalBalancingBad() const;

    /**
     * Determine splits for initial load balancing
     *
     *
     * This is part of the action SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning,
     * i.e. we use it for the initial local splitting.
     *
     * The algorithm is relatively simple: We determine the maximum number of
     * trees given the minimum of the weight (size) of the heaviest tree, the
     * weight that we are allowed to pick given the number of threads, and the
     * number of threads that are available.
     *
     * @param Number of splits that the code should run
     */
    int getNumberOfSplitsOnLocalRank() const;
};



