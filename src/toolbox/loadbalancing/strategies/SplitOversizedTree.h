// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/logging/Log.h"

#include "peano4/parallel/Node.h"

#include <map>

#include "toolbox/loadbalancing/AbstractLoadBalancing.h"

#include "toolbox/loadbalancing/metrics/CellCount.h"


namespace toolbox {
  namespace loadbalancing {
    namespace strategies {
      class SplitOversizedTree;
    }
  }
}


/**
 * Find trees that are bigger than largest tree size and split them
 *
 * This routine works if and only if you prescribe a maximum tree size. If you
 * do not specify a maximum tree size, it takes the optimal subtree size per
 * core as guideline. Whenever a tree is bigger than this threshold, it forks
 * off a new tree. However, the forked off tree is at most as big as the
 * threshold, as we don't want to create recursive splitting patterns. See
 * RecursiveBipartition if you are interested in the latter.
 *
 * This strategy is often used "late" throughout the grid construction. We can
 * phrase it the other way round: As it is an incremental strategy, we have to
 * assume that even a dynamically constructed grid is well ahead of this
 * balancing. So we split aggressively, top-down here. Consult a discussion of
 * splitting variants and constraints in peano4::grid::Spacetree::isCellTopDownSplitCandidate()
 * and  peano4::grid::Spacetree::isCellBottomUpSplitCandidate(),
 * peano4::grid::Spacetree::splitOrJoinCell() and the general
 * @ref peano_domain_decomposition "domain decomposition overview".
 *
 * While forking off trees once they exceed a certain threshold is not as
 * elegant as iterative, recursive splits, this strategy avoids very deep
 * tree topologies. These become an issue in line with
 * @ref page_peano_domain_decomposition "some domain split-up constraints",
 * as the logical tree topology's depth obviously is constraint by the
 * depth of the global spacetree. That is, this load balancing scheme is
 * advantageous if recursive splitting fails.
 *
 * I recommend to use this strategy if the following things hold:
 *
 * 1. Another strategy has already spread out over all ranks, so all ranks are
 *    busy with at least one tree. In this case, we use this strategy as a
 *    post-balancing step within a cascade of decompositions.
 * 2. We have a good grasp of the load distribution at the time when this load
 *    balancing step kicks in already.
 *
 * The second item implies that I've seen this strategy struggle for strongly
 * adaptive, fine meshes and meshes with a very inhomogeneous load pattern.
 */
class toolbox::loadbalancing::strategies::SplitOversizedTree: public toolbox::loadbalancing::AbstractLoadBalancing {
  public:
    /**
     * Set up recursive subdivision
     *
     * The ownership of the pointer goes over to the recursive subdivision
     * instance, i.e. you don't have to delete the passed object.
     */
    SplitOversizedTree(Configuration* configuration = new DefaultConfiguration(), CostMetrics*   costMetrics   = new toolbox::loadbalancing::metrics::CellCount());
    virtual ~SplitOversizedTree();

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
    virtual void finishStep() override;

    /**
     * I need the stats here mainly for debugging purposes. The load balancing
     * alread dumps information per time step. This routine however is more
     * elaborate/detailed and not used by default.
     */
    virtual std::string toString() const override;

  private:
    static tarch::logging::Log  _log;

    /**
     * If it equals the rank, then we are allowed to do something
     *
     * @see updateState()
     */
    int _roundRobinToken;

    /**
     * Is the balancing on the rank ok
     *
     */
    bool doesLocalTreeViolateThreshold() const;

    double getTargetTreeCost() const;

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
    void triggerSplit( int sourceTree, int targetRank, int numberOfSplits );

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

    int computeNumberOfSplits(int sourceTree) const;
};



