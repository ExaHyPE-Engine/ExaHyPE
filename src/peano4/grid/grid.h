// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_H_
#define _PEANO4_GRID_H_


#include <vector>
#include <map>
#include <set>
#include <list>


#include "GridVertex.h"


namespace peano4 {
  typedef std::map< int, int >  SplitSpecification;

  /**
   * @namespace peano4::grid
   *
   * The grid namespace is Peano's core.
   *
   * There are a few key classes in this namespace which realise Peano's core:
   *
   * - GridVertex: Represents one vertex of the tree. These vertices are geometric
   *     objects and do not carry any user data. They solely span the grid.
   * - Spacetree: Represents one tree. A tree is totally defined via its vertices.
   *     When we run through the mesh, we have a unique AutomatonState and the
   *     vertices which give us all information we need for our DFS.
   * - PeanoCurve: A collection of utility routines which are used to work with
   *     the Peano SFC. The routines therein help us to identify which stacks are
   *     to be accessed. This is main purpose of this routine collection/class.
   * - GridTraversalEvent and TraversalObserver: The spacetree holds the grid data
   *     and manages the grid traversal through its automaton. It does not invoke
   *     any user routines, manage partitions or move around user data. It however
   *     issues GridTraversalEvents and passes them over to a TraversalObserver.
   *     That's where the user code plugs in.
   * - GridTraversalEventGenerator Translates the transitions of the tree traversal
   *     automaton into GridTraversalEvents. It is a collection of utility
   *     routines.
   *
   */
  namespace grid {
    /**
     * Forward declaration
     */
    class GridStatistics;
    class GridControlEvent;

    class AutomatonState;
    class GridControlEvent;

    /**
     * The term clear() is not 100% correct, as the number of stationary traversals
     * is not reset to a dummy but instead incremented.
     */
    void clear( GridStatistics& statistics, bool isGlobalMasterTree );

    /**
     * Merge set of refinement/coarsening commands
     *
     * This routine has two roles:
     * - It shall reduce the number of refine events, so the actual AMR
     *   checks are faster when we next trigger the traversal
     * - It brings together the erase events: Erases are only triggered
     *   if a cell is completely embedded into an event. See
     *   Spacetree::evaluateGridControlEvents() for details. However, if
     *   we have two adjacent erase events, then it might happen that none
     *   of them hosts a cell, while the merger of the two of them does
     *   host one.
     *
     * ## Algorithm
     *
     * The algorithm works in multiple steps:
     * 
     * - We separate erase from refine events.
     * - We remove those erase events which are overwritten by refinement
     *   events. As we work conservatively, refinement events are always
     *   more important than coarsening ones.
     * - Create the erase power set.
     * - The code is asked to fuse multiple adjacent refinement events into
     *   fewer large ones, and to fuse multiple coarsening events as well.
     *   This reduces the event count, and gives the coarsening more
     *   flexibility (see discussion above).
     * - We join the remaining erase and refinement lists.
     *
     *
     * ##  Erasing the mesh (again)
     *
     * We have to create the power sets over all
     * erase events. This gives the mesh the opportunity to see faster
     * where erases are possible, as it also handle non-rectangular
     * regions: Assume there's a large L-shape of erase instructions consisting
     * of three erase commands. Our algorithm will create a rectangle
     * plus a square when it fuses the erases.
     *
     * Consequently, any cell overlapping with the boundary between the square
     * and the rectangle will not be erased.
     *
     * If we had not merged the three cubic erases forming the L into 2+1 but
     * into 2+2, we would also cover the boundary of the L shape. Therefore,
     * it is important that we construct the power set before we merge events.
     *
     */
    std::vector< GridControlEvent > merge( std::vector< GridControlEvent>  events, const double Tolerance = 0.1 );

    /**
     * Factory mechanism
     *
     * We expect the calling code to tell us about the adjacent ranks of a
     * vertex. There is a routine Spacetree::getAdjacentRanksForNewVertex()
     * which allows you to distill adjacency information while you step down
     * within the tree and create new vertices. This is information we write
     * directly into the new data plus the backup of the old data. This means,
     * by the time we create a new vertex, anybody analysing the adjacency
     * information things that this data has always been there.
     *
     * <h2> Dummy values </h2>
     *
     * There are a few attributes which should have dummy values. There are
     * also a few attributes which are set later on throughout the traversal,
     * but I should initialise them here properly to ensure that valgrind's
     * memchecker doesn't complain.
     *
     * @image html grid_createVertex.png
     *
     * @param adjacentRanks  Typically the result from Spacetree::getAdjacentRanksForNewVertex(coarseGridVertices,vertexPositionWithin3x3Patch).
     */
    GridVertex createVertex(
      GridVertex::State                            state,
      const tarch::la::Vector<Dimensions,double>&  x,
      int                                          level,
      const tarch::la::Vector<TwoPowerD,int>&      adjacentRanks,
      bool                                         isNewFineGridVertex
    );


    /**
     * A spacetree node is refined if any of its adjacent vertices holds one of
     * the following flags:
     *
     * - refining If all vertices are refining or hanging or triggered, but
     *     none of them has one of the flags discussed below, then we run into
     *     a brand new cell of the tree.
     * - refinement-triggered
     * - erase-triggered We want to erase this spacetree node, but the erase
     *     process is not triggered yet.
     * - erasing If none of the other vertices holds another flag of this list,
     *     then this cell is to be removed.
     */
    bool isSpacetreeNodeRefined(GridVertex  vertices[TwoPowerD]);

    /**
     * A vertex will be refined if it is already refined or currently refining.
     * It also will be refined if the erase is only triggered.
     */
    bool willVertexBeRefined(const GridVertex&  vertex);

    /**
     * A vertex has been refined if it is (already) refined or is erasing or
     * the erase has been triggered.
     */
    bool hasVertexBeenRefined(const GridVertex&  vertex);

    /**
     * A vertex is unrefined if it is hanging.
     *
     * @return bitset of vertices for which isVertexRefined() holds. If you wanna
     *   find out whether a cell is refined, you can compare the result to zero.
     *   You can also use isSpacetreeNodeRefined() instead.
     */
    std::bitset<TwoPowerD> willVerticesBeRefined(GridVertex  vertices[TwoPowerD]);
    std::bitset<TwoPowerD> haveVerticesBeenRefined(GridVertex  vertices[TwoPowerD]);


    /**
     * A spacetree node as 2^d adjacent vertices. So there are 2^d integers
     * stored within these vertices that overlap with the current node. They
     * all have to be the same. If they identify the local _id, then the
     * node is local. They are also local if the markers are set to
     * RankOfCellWitchWillBeJoined. This magic constant identifies cells on a
     * worker which might join into their master.
     *
     * Throughout the splitting process, an id might be already set to a
     * remote rank, though it still is technically and logically local. So
     * this routine interprets locality pretty technical and even marks those
     * cells as non-local (anymore) which still are for another grid sweep or
     * two.
     */
    bool isSpacetreeNodeLocal(
      GridVertex    vertices[TwoPowerD],
      bool          splittingIsConsideredLocal,
      bool          joiningIsConsideredLocal,
      int           id
    );


    enum class VertexType {
      New,
      Hanging,
      Persistent,
      Delete
    };

    enum class FaceType {
      New,
      Hanging,
      Persistent,
      Delete
    };

    enum class CellType {
      New,
      Persistent,
      Delete
    };


    std::string toString( VertexType type );
    std::string toString( FaceType type );
    std::string toString( CellType type );

    constexpr int InvalidRank(-1);


    enum class SpacetreeState {
      /**
       * Not yet a new root. Just got created, so we have to run through the
       * cloned data once, just to get it into the right order, and then we
       * can really mirror the master's traversal and send out stuff (in state
       * NewRoot).
       */
      EmptyRun,
      NewRoot,
      /**
       * Set if this tree results from a split and if this is the first
       * grid sweep when the former owner actually is in the mode
       * splitting.
       */
      NewFromSplit,
      Running,
      /**
       * Join has been triggered for this tree. Nothing is happening yet. It is
       * only the worker that updates all adjacency lists. These updates
       * however are not yet given to the master.
       */
      JoinTriggered,
      Joining,
      Joined
    };

    std::string toString( SpacetreeState state );

    bool overlaps( const tarch::la::Vector<Dimensions,double>& x, const GridControlEvent& event );
    bool overlaps( const AutomatonState& x, const GridControlEvent& event );

    /**
     * @return x is completely embeddd into event. x is subject to a re-calibration.
     */
    bool isContained( const AutomatonState& x, const GridControlEvent& event, double upscaleAutomatonState=1.0 );

    /**
     * Peano 4 does not reduce any grid control events globally. If you
     * want to reduce these events, you have to do so manually. Operation
     * becomes identify if you don't compile with MPI.
     *
     * This is a blocking routine, and it thus requires all ranks to call
     * it at exactly the same time. Use it with care. In ExaHyPE 2, e.g.,
     * I prefer a more anarchic exchange of grid control events.
     */
    void reduceGridControlEvents( std::vector<GridControlEvent>& events );

    namespace internal {
      /**
       * This is the first thing I do. Before I even start to think about the
       * erase events, lets get rid of those that are definitely cancelled.
       *
       * Uses the routine internal::refinementEventOverrulesCoarsening() to
       * flag those events within eraseEvents which have to go.
       */
      void removeEraseEventsThatAreCancelledByRefineEvents(
        const std::list< peano4::grid::GridControlEvent >&  refineEvents,
        std::list< peano4::grid::GridControlEvent >&        eraseEvents
      );

      /**
       * This routine merges adjacent events, i.e. events of the same time which
       * share a common face and can be combined into one larger, rectangular
       * event. The routine does not distinguish refine from coarsen events, i.e.
       * you should only pass in events of the same type.
       *
       * ## Algorithm
       *
       * It is basically a huge while loop. As long as we can merge any two events,
       * we have to continue to try to merge further. Any merge might produce a new,
       * larger area which then in turn might be a fit to another refinement area.
       *
       *
       * @see createBoundingBoxEvent()
       * @see twoEventsAreAdjacent()
       */
      void mergeAdjacentRefinementEvents( std::list< peano4::grid::GridControlEvent >&  inputEvents );

      /**
       * Helper function which helps us throughout the merge.
       */
      bool twoEventsOverlap( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs );

      bool equals( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs );

      /**
       * A refinement event overrules the coarsening if
       *
       * - The two events overlap
       * - The refine event defines a way coarser mesh then the coarse event.
       */
      bool refinementEventOverrulesCoarsening( const peano4::grid::GridControlEvent& refineEvent, const peano4::grid::GridControlEvent& eraseEvent );

      /**
       * Two events are adjacent if they have the same h, and if the sum of
       * their bounding box volumes is roughly the same as the bounding box
       * volume of the merger. We notice that the name is slightly wrong:
       * We are not only flagging adjacent events, we are even searching for
       * adjacent events of the same size.
       */
      bool twoEventsAreAdjacent( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs );

      /**
       * I need the power set over erase events to ensure that I realise
       * enough erases. See class documentation.
       *
       * ## Algorithm
       *
       * Run over all combinations of events. Construct the merged event
       * if possible. This includes a merged event of p==pp, i.e. if I
       * compare an event to itself. Check if the event is already in the
       * power set. If not, store it in a temporary container. After you've
       * run over all combinations, add the content of the temporary
       * container to the input set and run again. If this temporary container
       * still is empty after we have compared all potential event combinations,
       * we know that we have already all possible combinations in the power
       * set. So terminate.
       */
      void generateEraseEventPowerset( std::list< peano4::grid::GridControlEvent >&  inputEvents );


      peano4::grid::GridControlEvent createBoundingBoxEvent( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs );
    }
  }
}


peano4::grid::GridStatistics operator+( peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs );

#endif
