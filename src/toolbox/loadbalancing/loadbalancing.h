// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_LOAD_BALANCING_LOAD_BALANCING_
#define _TOOLBOX_LOAD_BALANCING_LOAD_BALANCING_



namespace toolbox {
  /**
   * @namespace toolbox::loadbalancing
   *
   * Suite of generic load balancing algorithms for Peano 4.
   *
   *
   * <h2> Load balancing strategies </h2>
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
   *
   */
  namespace loadbalancing {
    /**
     * Dump the stats of the lb to the terminal (info device). It is invoked
     * around once per time step/grid sweep and runs through all the spacetrees
     * held on this particular rank.
     */
    void dumpStatistics();
  }
}


#endif
