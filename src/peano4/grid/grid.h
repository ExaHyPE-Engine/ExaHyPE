// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <vector>
#include <map>
#include <set>
#include <list>


#include "GridVertex.h"


namespace peano4 {
  /**
   * Instruction to split
   *
   * An instruction to split always holds the number of splits that we should
   * fork off, and it also contains an instruction how we want the tree to
   * split the data off. We can either do the bottom up scheme, where we try
   * to find exactly the right number of fine grid cells on the finest level
   * and then assign refined cells to remote trees where it is a fit. The
   * realisation of these splitting variants is discussed in peano4::grid::Spacetree::isCellBottomUpSplitCandidate(),
   * ::grid::Spacetree::isCellTopDownSplitCandidate()
   * and peano4::grid::Spacetree::splitOrJoinCell(). The underlying algorithmic
   * considerations are documented in @ref peano_domain_decomposition "the domain decomposition overview".
   *
   * I have no constructor for this struct, but you can easily create one via
   *
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   * SplitInstruction{24,SplitInstruction::Mode::AggressiveTopDown}
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *
   * using the brace initialisation of C++.
   */
  struct SplitInstruction {
    enum class Mode {
      AggressiveTopDown,
      BottomUp
    };

    int  numberOfFineGridCells;
    Mode mode;

    static std::string toString(Mode mode);
    std::string toString() const;
  };


  typedef std::map< int, SplitInstruction >  SplitSpecification;

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
     *
     * ## Algorithm
     *
     * The algorithm works in multiple steps:
     * 
     * - We separate erase from refine events.
     * - We remove those erase events which are overwritten by refinement
     *   events. As we work conservatively, refinement events are always
     *   more important than coarsening ones.
     * - Sort the events
     * - Create the erase power set.
     * - The code is asked to fuse multiple adjacent refinement events into
     *   fewer large ones, and to fuse multiple coarsening events as well.
     *   This reduces the event count, and gives the coarsening more
     *   flexibility (see discussion above).
     * - We join the remaining erase and refinement lists.
     *
     *
     * ## Sorting
     *
     * We sort the respective events along x,y,z offset. In 2d, this doesn't
     * make that much of a difference, for d>2 it is important: The merger of
     * multiple events works in a greedy fashion. Otherwise, it would be too
     * slow. Due to this greediness, we now might run into situations where we
     * are stuck. Consider a 2x2x2 cube of refinement operations which all have
     * the same target resolution. It should be possible to merge this cube
     * into one single large cube. However, we might end up with
     *
     *        (r_111,r_211), (r_121,r_122), (r_212,r_222), r_112, r221)
     *
     * These six events now can't be merged greedily anymore.
     *
     * Once we sort the events by x,y,z, the original input sequence implicitly
     * ensures that we first merge along the z direction, then along y, and
     * finally along x:
     *
     *        (r_111,r1_112,r_121,r_122,r_211,r1_212,r_221,r_222)
     *
     * Due to the SFC that we employ for the mesh traversal, it is reasonable
     * to assume that the refinement events already align in a localised order.
     * Therefore, I apply a simple bubble sort.
     *
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
     * The power set construction very quickly explodes, i.e. leads to a vast
     * number of events. I thus manually truncate the creation of new events.
     *
     * Remark: I had to remove this power set step, as it turned out to be too
     * time consuming.
     *
     *
     * ## Properties
     *
     * The algorithm has to be idempotent, i.e. if we invoke it multiple times
     * over the input set, the outcome should always remain the same.
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
     * isContained() is defined over the closed interval, i.e. we look if x is
     * contained within the cube spanned by x including its faces.
     *
     * @return x is completely embedded into event. x is subject to a re-calibration.
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

    std::string toString( const std::vector<GridControlEvent>& events );
    std::string toString( const std::list<GridControlEvent>& events );

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
       * In-situ sort (bubble sort)
       *
       * The sorting is required to ensure that the merging is deterministic on
       * a parallel machine. peano4::grid::merge() discusses the details behind
       * this. Unfortunately, the sorting can be extremely expensive (notably
       * in 3d). I assume that an "inefficient" merger of the refinement events
       * is kind of acceptable: In the worst case, most codes will not erase as
       * aggressively as they could and then trigger more erase operations in
       * the subsequent mesh traversal.
       *
       * The rationale behind this statement is as follows: A cell or region
       * around a vertex refine if they overlap with a refinement instruction.
       * It hence does not really make any difference if we fuse two adjacent
       * refinement instructions or not. Things are different with erase: A
       * region erases if and only if it is contained completely within one
       * erase command. So if we have to adjacent cells which say "erase", but
       * these guys are not merged into one, then an octant which is half
       * overlapping and half with the other erase command will not erase, as
       * it is not completely embedded. I therefore typically recommend to
       * fuse erase commands as aggressively as possible, but to be a little
       * bit more relaxed with the refines.
       *
       * @see merge() for a discussion
       * @param events Input and output set of grid control events
       * @param maxIterations Maximum number of bubble sort iterations
       */
      void sort( std::list< peano4::grid::GridControlEvent >& events, int maxIterations );

      peano4::grid::GridControlEvent createBoundingBoxEvent( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs );
    }
  }
}


peano4::grid::GridStatistics operator+( peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs );

std::ostream& operator<<(std::ostream& out, const peano4::SplitInstruction& instruction);
std::ostream& operator<<(std::ostream& out, const peano4::SplitInstruction::Mode& mode);
