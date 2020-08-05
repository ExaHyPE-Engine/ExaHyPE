// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_LOAD_BALANCING_RECURSIVE_SUBDIVISION_
#define _TOOLBOX_LOAD_BALANCING_RECURSIVE_SUBDIVISION_



#include "tarch/logging/Log.h"
#include "tarch/mpi/mpi.h"
#include <map>



namespace toolbox {
  namespace loadbalancing {
    class RecursiveSubdivision;
  }
}


/**
 * Recursive load balancing micking the behaviour of the guided partitioning in OpenMP
 *
 * This is one of the simplest load balancing strategies one can think about. We
 * try to mimick the behaviour of OpenMP's guided scheduling: We determine the
 * maximum number of unrefined inner cells around and compute an optimal load per
 * core. If we used this tree size, we'd not have any flexibility to move stuff around.
 * So we say that no tree anywhere should be bigger than half of this size. So, in
 * theory, we overbook all of the cores by a factor of two. Whenever a rank decides
 * that it has to split up its tree once more, it deploys the new tree to the rank
 * with the smallest total load (with a slight bias to keep stuff local). The new
 * tree results from classic bipartitioning.
 *
 * If we have N cells in total and run with R ranks, then the algorithm first spreads
 * these cells over the ranks with N/R cells per rank. After that, each rank hosts
 * one spacetree.
 *
 * In an idea world, each core would in this scenario host N/R/C cells. We however set
 * the maximum size of an admissible tree to N/R/C/2.
 *
 * Whenever we run into a situation where not all cores of the local rank are occupied,
 * we cut the local tree into two equally sized pieces and deploy the new tree locally.
 * This way, we flood all local cores.
 *
 * Whenever any local tree violates the magic threshold of N/R/C/2, we split it up into
 * two parts. This time, we deploy the new subtree to the MPI rank with the smallest
 * total load. This way, we balance across MPI ranks.
 *
 * <h2> Implementation remarks </h2>
 *
 * The strategy runs through different phases.
 *
 * <h3> Phase 1: The tree is too small so there would not be enough cells if we had even only one cell per tree </h3>
 *
 * We wait and do nothing. After all, this might be the first step of the grid
 * construction, so we are well-advised to wait for a few iterations to get
 * an idea what the final grid will look like.
 *
 * <h3> Phase 2: We have not yet used all ranks </h3>
 *
 * First of all, we make all ranks busy. This can be done in one rush: Rank 0
 * splits up equally sized tree to each and every rank. As phase 1 has
 * terminated already, we know that there are enough cells around.
 *
 * <h3> Phase 3: Get reasonably many cores into the game </h3>
 *
 *
 * <h3> Phase 4: Load balancing between the ranks (dynamic) </h3>
 *
 * This is the dynamic load balancing phase. Take the biggest tree, check
 * whether it is too heavy and split it. When you split it, search for the
 * rank with the lowest total load.
 *
 *
 * <h2> Broad vs. deep trees </h2>
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
 * <h2> Blacklisting </h2>
 *
 * The recursive subdivision works with one tree at a time: It always identifies the
 * one tree with the biggest load and subdivides this one further. A tree however
 * requires two traversals to split. If a tree is identified as split candidate, the
 * code hence will tell it to split, but will try to split exactly the same tree again
 * in the subsequent iteration.
 *
 * The tree's stats (how many cells does it own) at this point are skewed, as the
 * tree has not yet successfully given away cells. Indeed, a tree needs at least three
 * iterations to 'recover'. The first one initialises the split. The second traversal
 * conducts the split and eventually moves around data, but the ownership in this
 * traversal still is with the master. Therefore, the master still has all the local
 * cells. After the third split, the master has actually given away its cells.
 *
 * Therefore, we work with a blacklist map: Whenever we have identified a tree that is to
 * be split, we memorise it and give it an internal weight of 3. Memorise means it is
 * stored in a map. As long as a rank is on the blacklist map, it may not be selected
 * as potential split tree. After each iteration, we reduce the weight of the blacklisted
 * trees by one. If a weight becomes zero, we eventually remove it from the blacklist.
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
class toolbox::loadbalancing::RecursiveSubdivision {
  public:
    /**
     * @param ratioOfCoresThatShouldInTheoryGetAtLeastOneCell Pass in something between 0 
     *           and 1 (though bigger than 1 might work, too)
     */
    RecursiveSubdivision(double ratioOfCoresThatShouldInTheoryGetAtLeastOneCell=0.8);
    ~RecursiveSubdivision();

    /**
     * Triggers actual load balancing data exchange, triggers reblaancing, and
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
    void finishSimulation();

    /**
     * I need the stats here mainly for debugging purposes. The load balancing
     * alread dumps information per time step. This routine however is more
     * elaborate/detailed and not used by default.
     */
    std::string toString() const;
  private:
    /**
     * @see getStrategyStep()
     */
    enum class StrategyStep {
      Wait,
      SpreadEquallyOverAllRanks,
      SplitHeaviestLocalTreeMultipleTimes_UseLocalRank_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_UseAllRanks_UseRecursivePartitioning,
      SplitHeaviestLocalTreeOnce_DontUseLocalRank_UseRecursivePartitioning
    };

    static std::string toString( StrategyStep step );

    /**
     * Analyse current global situation and commit to a strategy. This routine
     * analyses the rank state and returns an action.
     */
    StrategyStep getStrategyStep() const;

    /**
     * Is used by tree identification and either indicates that there are no trees
     * at all or means that the heaviest tree is on the blacklist. See implementation
     * remarks in class description.
     */
    static constexpr int NoHeaviestTreeAvailable = -1;

    static tarch::logging::Log  _log;

    const double _RatioOfCoresThatShouldInTheoryGetAtLeastOneCell;

    std::map< int, int>    _blacklist;

    bool _hasSpreadOutOverAllRanks;

    /**
     * Status variable required to compute good load balancing. 
     */
    int _localNumberOfInnerUnrefinedCell;

    /**
     * Status variable required to compute good load balancing
     */
    int _globalNumberOfInnerUnrefinedCells;
    bool _globalNumberOfInnerUnrefinedCellsHasGrownSinceLastSnapshot;

    int _lightestRank;

    enum class StrategyState {
      Standard,
      WaitForRoundRobinToken,
      PostponedDecisionDueToLackOfCells,
      Stagnation
    };

    static std::string toString( StrategyState state );

    StrategyState  _state;

    void updateGlobalView();

    /**
     * Determine the (optimal) maximum spacetree size
     *
     * The maximum size in principle is the total number of cells divided by
     * all possible spacetrees. The latter is in principle the number of ranks
     * times the number of cores (p), though we allow users to diminish this number
     * slightly due to _RatioOfCoresThatShouldInTheoryGetAtLeastOneCell. I
     * call this one @f$ \alpha @f$ below. So we have the situation
     * that the maximum spacetree size
     *
     * @f$
       n_{\text{max}} = \frac{N_{\text{global}}}{\alpha p} \cdot \frac{1}{k}
       @f$
     *
     * with a natural number k>1. If we follow this approach, we can end up with
     * situations where we have 400 cells in a local partition, the optimal
     * number of cells might be 399. Such situations arise with AMR but also
     * throughout the top-down grid construction. In this case, we'd end up with
     * a remaining partition of 10 which is not what we want. Therefore I make the
     * equation above subject to the constraint that the remainder of the
     * partition should not be smaller than the new subpartition. If we would run
     * into this situation, then I take the average in-between.
     *
     * <h2> Delay of build-up/analysis </h2>
     *
     * Our analysis of an optimal spacetree size depends on the local attribute
     * _globalNumberOfInnerUnrefinedCells. This one is determined after each
     * traversal, i.e. it runs risk to lag behind the by one iteration.
     * Therefore, I do use some historic data: If
     * _globalNumberOfInnerUnrefinedCells hasn't grown over the last two
     * iterations, then I'm kind of confident that it gives us a good idea of
     * what the overall spacetree looks like.
     *
     *
     * @see _globalNumberOfInnerUnrefinedCells
     */
    int getMaximumSpacetreeSize(int localSize = std::numeric_limits<int>::max()) const;

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
     * A more sophisticated version of the code thus
     *
     * @return NoHeaviestTreeAvailable If there are no local trees or
     *   if the heaviest tree is on the blacklist, i.e. we have to
     *   assume that it still is splitting.
     */
    int getIdOfHeaviestLocalSpacetree() const;

    /**
     * @return -1 if there is no local tree yet
     */
    int getWeightOfHeaviestLocalSpacetree() const;

    bool doesBiggestLocalSpactreeViolateOptimalityCondition() const;

    void updateBlacklist();

    /**
     * Wrapper around the spacetree set which also updates the blacklist.
     */
    void triggerSplit( int sourceTree, int numberOfCells, int targetRank );

    /**
     *
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
     */
    void updateState();

    #ifdef Parallel
    MPI_Request*    _globalSumRequest;
    MPI_Request*    _globalLightestRankRequest;

    /**
     * It is totally annoying, but it seems that MPI's maxloc and reduction are broken
     * in some MPI implementations if we use them for integers.
     */
    struct ReductionBuffer {
      double   _localUnrefinedCells;
      int      _rank;
    };

    int             _globalNumberOfInnerUnrefinedCellsBufferIn;
    ReductionBuffer _lightestRankBufferIn;
    ReductionBuffer _lightestRankBufferOut;
    #endif

    /**
     * @see updateState()
     */
    int _roundRobinToken;

    /**
     * If _roundRobinToken is smaller than or equal to this threshold, then the
     * rank is allowed to load balance.
     */
    int _roundRobinThreshold;

    int _maxTreeWeightAtLastSplit;
};


#endif

