// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/loadbalancing/metrics/CellCount.h"
#include "toolbox/loadbalancing/metrics/CustomCellWeight.h"


/**
 * @page toolbox_loadbalancing Loadbalancing toolbox
 *
 * Suite of generic load balancing algorithms for Peano 4. The toolbox's
 * strategies typically have a State, a Configuration and a Blacklist. The
 * toolbox::loadbalancing::strategies namespace hosts primitive strategies, but you
 * can combine them into more sophisticated cascade of load balancing
 * schemes. You find examples for such cascades in the namespace
 * toolbox::loadbalancing::strategies::cascade. Most of them are only plain typedefs.
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
 * see more on how to configure load balancing of ExaHyPE:
 * \li \subpage page_exahype_multicore
 * 
 * and suggenstions on performance optimisation in Peano
 * \li \subpage page_peano_performance_optimisation
 *
 * ## Overview over some pre-manufactured load balancing strategies
 *
 * - toolbox::loadbalancing::strategies::Hardcoded Don't do any analysis but hand out the
 *   partitioning instructions following a rulebook. This version allows you
 *   to create a reproducible load balancing scheme or to manually determine
 *   the splitting.
 * - toolbox::loadbalancing::strategies::SpreadOut Ensure that, if the problem is large
 *   enough, each thread on each rank gets one partition.
 * - toolbox::loadbalancing::strategies::SpreadOutHierarchically This strategy is very
 *   similar to toolbox::loadbalancing::strategies::SpreadOut, but is spreads out in two
 *   phases. First, the strategy tries to give each rank a partition. From
 *   thereon, each rank tries to give each of its threads one partition.
 * - toolbox::loadbalancing::strategies::RecursiveBipartition As the name suggests, the
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
 *
 * ## Alter cost metrics
 *
 * All the load balancing strategies work against some cost metrics. By
 * default, we use CellCount. But you can create your own cost metric and
 * use this one instead.
 *
 * ## Visualizing load balancing
 * 
 * In the toolbox.loadbalancing toolbox, you find a script plot-load-distribution.py which creates 
 * plots over the data distribution.
 * The invocation is close to trivial 
 * 
 * ~~~~~~~~~~~~~~~~~~~~~
 * python3 ~/git/Peano/python/peano4/toolbox/loadbalancing/plot_load_distribution.py mpi/ccz4-backfill-fv-0.05-4-ranks.results 
 * ~~~~~~~~~~~~~~~~~~~~~
 * 
 * though you might want to study the help dumped if you pass --help.
 * 
 * 
 * The script works for shared and distributed memory and gives you a scatter plot
 * over the load distribution and tells you also how many trees aka subpartitions
 * you employ per rank.
 * 
 * @image html runtime_analysis_loadbalancing_example00.png 
 * @image html runtime_analysis_loadbalancing_example01.png 
 * 
 * 
 * ### Statistics
 * 
 * Peano provides a statistics API, and a lot of Peano components, the technical 
 * architecture, and Peano's extensions use this API to dump stats. As tracking
 * statistics can be expensive, it is disabled by default. The statistics are not
 * meant to replace any performance statistics as you get them from profiling.
 * In the context of runtime, we offer them to supplement performance data and 
 * behaviour with an application point of view. 

 * All the statistics writing is realised through tarch::logging::Statistics. 
 * This is a singleton (similar to a normal logger) which builds up statistics
 * databases over runtime. It flushes the data once you call tarch::logging::Statistics::writeToCSV().
 
 * It is up to the source code to dump stats into the singleton. The data dump
 * calls also can control how the statistics backend stores the data: It can 
 * memorise (trace) each individual data submission, 
 * or dump data in regular intervals (sampling).
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
     * This is a helper routine which is used by ExaHyPE's default main for 
     * for example. It is never used by the actual load balancing at the
     * moment.
     * 
     * @return -1 if there is no local tree yet
     */
    int getWeightOfHeaviestLocalSpacetree();
  }
}


