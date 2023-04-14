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
 * rank. If a rank hosts more than the optimal load subject to an inaccuracy factor,
 * we split that one up into two parts and it deploys the new tree to the most
 * underbooked other rank.
 *
 * In a second phase, we apply the same scheme on the rank: If a rank's local trees
 * are too different, the heaviest tree on the rank has to split further.
 *
 * After this second phase, we enter a second phase where we pass around a
 * token. Whichever rank holds the token, checks the balancing. If is is bad,
 * we return to step number one.
 *
 * The scheme never enters stagnation.
 */
class toolbox::loadbalancing::RecursiveBipartition: public toolbox::loadbalancing::LoadBalancing {
  public:
    /**
     * Set up recursive subdivision
     *
     * The ownership of the pointer goes over to the recursive subdivision
     * instance, i.e. you don't have to delete the passed object.
     */
    RecursiveBipartition(Configuration* configuration = new DefaultConfiguration(), bool deleteInDestructor = true);
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
    void finishStep() override;

    /**
     * I need the stats here mainly for debugging purposes. The load balancing
     * alread dumps information per time step. This routine however is more
     * elaborate/detailed and not used by default.
     */
    std::string toString() const;

  private:
    static tarch::logging::Log  _log;

    /**
     * If it equals the rank, then we are allowed to do something
     *
     * @see updateState()
     */
    int _roundRobinToken;

    /**
     * Is the balancing between the ranks ok
     *
     * This operation checks if the inter-rank load decomposition does
     * violate the overall load balancing constraints. The global balancing
     * is determined by a cascade of checks:
     *
     * - If we have an inconsistent data view, i.e. if the stats are not valid,
     *   we have to assume that everything is fine.
     * - If we have fewer trees globally than we have ranks, the balancing is
     *   by definition poor.
     * - If we host more trees than allowed, the balancing by definition is
     *   good, since we could not refine further anyway.
     * - Otherwise, we look if the max load per rank and the min load per rank
     *   differ more than the tolerance.
     */
    bool isInterRankBalancingBad() const;

    /**
     * Is the balancing on the rank ok
     *
     * Similar to isInterRankBalancingBad(), this operation runs through a
     * series of checks:
     *
     * - If there are no trees on the rank, then the result is false. By
     *   definition, everything is well-balanced.
     * - If there is only one tree, the balancing is by definition bad, as
     *   we cannot make a balancing statement.
     * - If we host more trees than allowed, the balancing by definition is
     *   good, since we could not refine further anyway.
     * - Otherwise, we compare the minimum and maximum tree.
     */
    bool isIntraRankBalancingBad() const;

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
};



