// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


/**
 * @page toolbox_loadbalancing Loadbalancing toolbox
 *
 * Suite of generic load balancing algorithms for Peano 4. The toolbox's
 * strategies typically have a State, a Configuration and a Blacklist. The
 * toolbox::loadbalancing namespace hosts primitive strategies, but you
 * can combine them into more sophisticated cascade of load balancing
 * schemes. You find examples for such cascades in the namespace
 * toolbox::loadbalancing::cascade. Most of them are only plain typedefs.
 *
 *
 * ## Usage through Python API
 *
 * Peano itself does not provide any support for the load balancing. You
 * have to add it in your main routine.
 *
 * Many extensions, such as ExaHyPE, not only use the load balancing
 * toolbox, they also offer a tailored ExaHyPE load balancing configuration.
 * Indeed,the toolbox follows the philosophy that the chosen load balancing
 * determines the load balancing strategy, while the configuration object
 * tailors this strategy to your experimental setup and machine.
 *
 *
 * ## Overview over some pre-manufactured load balancing strategies
 *
 * - toolbox::loadbalancing::Hardcoded Don't do any analysis but hand out the
 *   partitioning instructions following a rulebook. This version allows you
 *   to create a reproducible load balancing scheme or to manually determine
 *   the splitting.
 * - toolbox::loadbalancing::SpreadOut Ensure that, if the problem is large
 *   enough, each thread on each rank gets one partition.
 * - toolbox::loadbalancing::SpreadOutHierarchically This strategy is very
 *   similar to toolbox::loadbalancing::SpreadOut, but is spreads out in two
 *   phases. First, the strategy tries to give each rank a partition. From
 *   thereon, each rank tries to give each of its threads one partition.
 * - toolbox::loadbalancing::RecursiveBipartition As the name suggests, the
 *   load balancing tries to identify oversized partitions and then splits
 *   these guys into two.
 *
 *
 * ## Rationale behind sophisticated load balancing strategies
 *
 * Every load balancing strategy in Peano 4 will have to balance two
 * contradicting objectives:
 *
 * - Load balancing has to happen as soon as possible throughout the grid
 *   construction process, as we want the grid construction to run in parallel
 *   and splits per se are extremely memory-demanding---at least temporarily
 *   as parts of a partition have to be replicated and moved around.
 * - Load balancing performs best if triggered for reasonably big meshes,
 *   as finer meshes allow for better tree cuts. If we know how many cells
 *   there are, we can balance these cells better, and we can subdivide cells
 *   more accurately: If we deploy a 3x3 grid to two cores, one core will get
 *   5 and one 4 cells. If all cells refine once more, this will induce a
 *   45 vs 36 mesh. If we cute a 9x9 mesh in a balanced way, we get a 41 vs
 *   40 mesh.
 *
 * The first argument is a strict one. If we run out of memory, the
 * simulation will crash. The second argument is a desireable one.
 *
 * We derive generic requirements for any load balancing:
 *
 * - Split up aggressively initially such that a reasonable load
 *   decomposition is achieved and there are no single large partitions left
 *   over which will eventually be split up further. Such single large
 *   partitions might make us run out of memory later on.
 * - Split aggressively for small partitions, split only once or twice per
 *   traversal later on when there are large partitions.
 * - Fine tune a load decomposition as late as possible, when accurate SFC
 *   cuts in the mesh can be made. Keep in mind that we cannot diffuse load
 *   decompositions. We can only merge with a master and then resplit.
 * - Throttle the grid construction or a grid refinement whenever we
 *   rebalance. Rebalancing is already problematic from a memory constraint
 *   point of view. Any further refinement makes this situation worse.
 *
 */


namespace toolbox {
  /**
   * @namespace toolbox::loadbalancing
   *
   * The namespace hosts some generic utilities, it hosts a statistics class
   * (Statistics) and the Blacklist, and it finally hosts a few core load
   * balancing classes. They are rather primitive. You can construct more
   * sophisticated load balancing schemes from there by combining simple
   * schemes via a Cascade, e.g.
   */
  namespace loadbalancing {
    /**
     * Dump the stats of the lb to the terminal (info device). It is invoked
     * around once per time step/grid sweep and runs through all the spacetrees
     * held on this particular rank.
     */
    void dumpStatistics();
    

    /**
     * Is used by tree identification and either indicates that there are no trees
     * at all or means that the heaviest tree is on the blacklist. See implementation
     * remarks in class description.
     */
    constexpr int NoHeaviestTreeAvailable = -1;

    /**
     * Determines the maximum spacetree size a tree should have in the
     * optimal case.
     *
     * As this routine does not really adopt the blacklist, it can introduce
     * cyclles: If we repeatedly try to split the same
     * rank this means that we have tried to split it, have not been
     * successful, and tried again. This can happen, as not all trees
     * can be split. See peano4::grid::Spacetree::isCellSplitCandidate()
     * for a discussion which cells can be split and which can't. As
     * not all cells can't be given away, not all trees can be split up.
     *
     * ## Randomisation
     *
     * If two or more trees have the same weight, the routine should
     * randomly return one of them.
     *
     * @param tolerance By default, you get the last heaviest tree. But you
     *   can ask for a random tree
     *
     * @return NoHeaviestTreeAvailable If there are no local trees or
     *   if the heaviest tree is on the blacklist, i.e. we have to
     *   assume that it still is splitting.
     */
    int getIdOfHeaviestLocalSpacetree();

    /**
     * Similar to getIdOfHeaviestLocalSpacetree() but you might get one of
     * the trees back that is close to the heaviest one up to tolerance.
     * Tolerance here is the relative difference: So you are guaranteed
     * that
     *
     * @f$ m_i \geq tolerance \cdot m_{max} @f$
     *
     * or
     *
     * @f$ \frac{m_i}{m_{max}} \geq tolerance @f$
     *
     *
     *
     */
    int getIdOfHeaviestLocalSpacetree(
      double tolerance
    );

    /**
     * @return -1 if there is no local tree yet
     */
    int getWeightOfHeaviestLocalSpacetree();
  }
}


