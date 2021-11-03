// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_H_
#define _PEANO4_GRID_H_


#include <vector>
#include <map>
#include <set>


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
     * Merge set such that many events next to each other are fused into one
     *
     * Take a set of grid control events and try to consolidate them, i.e.
     * remove overlaps and fuse individual requests. The consolidation
     * algorithm is pretty primitive:
     *
     * - We create a new (empty) result list.
     * - We iterate over the input list. 
     * - Per entry, we check it against all elements that have already been
     *   added ot the result set. 
     *   -- If the current one is adjacent to an element from the output set,
     *      and if both trigger the same kind of refinement, we enlarge the 
     *      element that is already in the result set and memorise that we 
     *      have "inserted" something into the output. We can immediately 
     *      terminate our run-through through the output.
     *   -- If a refinement entry in the input sequence overlaps with an 
     *      erase entry in the output (or the other way round), we keep the 
     *      refinement only. We can immediately terminate our run-through 
     *      through the output.
     *   -- If an entry from the input set has not triggered any modification
     *      of an entry in the output set, we append it to the output set.
     * - If changes have been made, i.e. if the output set is smaller than 
     *   the input set, then we rerun the algorithm recursively again. Due to
     *   mergers, four adjacent blocks might now form two bigger blocks which 
     *   in turn can be merged once again.
     *
     * <h2> Two boxes can be merged into one </h2>
     * 
     * The "next to each other" implementation is again simplistic: We
     * construct the bounding box around two events and compare its volume
     * to the sum of the volumes of the two elements. If the bounding box
     * volume is only around a few percent bigger than the sum, then we use
     * the boundary box to replace the original two elements.
     *
     * What I don't do in this algorithm is to take overlaps into account.
     * Indeed, the slack (percentage) accommodates slight overlaps. But the
     * code still might not recognise two elements if they massively overlap.
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
  }
}


peano4::grid::GridStatistics operator+( peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs );

#endif
