// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_SPACETREE_H_
#define _PEANO4_GRID_SPACETREE_H_



#include "AutomatonState.h"
#include "GridVertex.h"
#include "GridStatistics.h"
#include "GridControlEvent.h"


#include "tarch/logging/Log.h"


#include "peano4/stacks/STDVectorStack.h"


#include "peano4/maps/STDStackMap.h"
#include "peano4/maps/HierarchicalStackMap.h"


#include <set>
#include <bitset>


namespace peano4 {
  namespace grid {
    class Spacetree;
    class TraversalObserver;
    class GridTraversalEvent;

    namespace tests {
      class SpacetreeTest;
    }
  }

  namespace parallel {
    class SpacetreeSet;
  }
}



/**
 * Represents one tree
 */
class peano4::grid::Spacetree {
  public:
    static const int InvalidRank;
    static const int RankOfPeriodicBoundaryCondition;
    static const int NumberOfStationarySweepsToWaitAtLeastTillJoin;

    static bool isVertexRefined(GridVertex  vertex);

    /**
     * A vertex is unrefined if it is hanging.
     *
     * @return bitset of vertices for which isVertexRefined() holds. If you wanna
     *   find out whether a cell is refined, you can compare the result to zero.
     *   You can also use isSpacetreeNodeRefined() instead.
     */
    std::bitset<TwoPowerD> areVerticesRefined(GridVertex  vertices[TwoPowerD]) const;

    /**
     * Vertices are local. I consider splitting and joining vertices to be
     * local, too. It is therefore consistent with areFacesLocal().
     */
    std::bitset<TwoPowerD> areVerticesLocal(GridVertex  vertices[TwoPowerD]) const;

    std::bitset<TwoPowerD> areVerticesInsideDomain(GridVertex  vertices[TwoPowerD]) const;

    /**
     * Identifies for the @f$ 2 \cdot d @f$ faces whether they are local or not.
     *
     * <h2> Implementation </h2>
     *
     * - I loop over the 2d faces.
     * - Per face, I loop over all @f$ 2^d @f$ vertices but alter the entry
     *   along the normal manually. So I'm inefficient, but I don't care.
     * - Per relevant vertex, I have to check two entries in the adjacency
     *   list.
     * - Splitting and split-triggered ranks are still considered to be
     *   local.
     */
    std::bitset<TwoTimesD> areFacesLocal(GridVertex  vertices[TwoPowerD]) const;

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
    static bool isSpacetreeNodeRefined(GridVertex  vertices[TwoPowerD]);


  private:
    static tarch::logging::Log  _log;

    friend class peano4::parallel::SpacetreeSet;
    friend class peano4::grid::tests::SpacetreeTest;

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

    static std::string toString( SpacetreeState state );

    static std::string toString( VertexType type );
    static std::string toString( FaceType type );
    static std::string toString( CellType type );

    /**
     * Simple recursive type analysis
     *
     * \section  Merge semantics
     *
     * We run over the parent vertices and merge along the faces (per
     * dimension). The priority of flags should be taken from the code
     * directly.
     */
    static VertexType getVertexType(
      GridVertex                         coarseGridVertices[TwoPowerD],
      tarch::la::Vector<Dimensions,int>  position,
      int                                dimension = Dimensions-1
    );

    /**
     * Identify type of vertex.
     *
     * Find out what type a face has, i.e. is it a new one, one that is to
     * be deleted, or is it a persistent one. The outcome will decide where
     * to take a vertex from and where to store it. It also will determine
     * which cascade of events we will invoke.
     *
     * <h2> Implementation </h2>
     *
     * Different to getVertexType(), we can work with the the fine grid's
     * vertices here already. The implementation might not be the most
     * elegant one: We loop over all four or eight vertices, respectively,
     * but we use the normal of the faceNumber to alter the vertex index
     * such that we eventually study only half of the vertices; those adjacent
     * to the face.
     *
     * @image html Spacetree_getFaceType.png
     *
     *
     * There are four potential outcomes: new, hanging, persistent, delete.
     * They derive as follows:
     *
     * - Hanging: A face is hanging if all adjacent vertices on the same
     *   level are hanging (red in sketch above).
     * - Delete: A face is delete if all adjacent vertices are either
     *   hanging or delete and the face is not hanging.
     * - New: A face is new if all adjacent vertices are either
     *   hanging or new and the face is not hanging.
     * - Persistent: If nothing else applies.
     *
     * So while I use the vertex states, I cannot directly use the vertices.
     * They have already gone through the vertex lifecycle, i.e. new vertices
     * might already have the new flag. Instead, I have to go through
     * getVertexType(). This might be inefficient and leave room for improvements,
     * but is does the job for the time being.
     */
    static FaceType getFaceType(
      GridVertex                         coarseGridVertices[TwoPowerD],
      tarch::la::Vector<Dimensions,int>  positionOfCell,
      int                                faceNumber
    );

    /**
     * Can a cell be split (deployed to another rank)
     *
     * Not all cells are a fit and can be deployed to another rank even
     * though the spacetree set wants to split a tree. Cells that disqualify
     * for splitting are
     *
     * - Cells which a adjacent to a periodic boundary conditions. Such cells
     *   (including their adjacent vertices) all are kept on tree 0 so all
     *   periodic boundary data exchange happens on tree 0 only.
     * - Non-local cells (obviously)
     * - Local cells whose master is not local. If we'd move such cells to
     *   another rank, we'd destroy the logical topology between the trees.
     */
    bool isCellSplitCandidate(
      GridVertex                         coarseGridVertices[TwoPowerD],
      GridVertex                         fineGridVertices[TwoPowerD]
    ) const;

    /**
     * You pass in the vertices and it gives you back the cell type.
     * This routine translates the 2^d vertices of a cell into a cell type.
     *
     * @see getFaceType()
     */
    static CellType getCellType(
      GridVertex                         coarseGridVertices[TwoPowerD],
      tarch::la::Vector<Dimensions,int>  positionOfCell
    );

    int              _id;

    SpacetreeState   _spacetreeState;

    /**
     * The root of a spacetree corresponds to the initial state of the tree
     * traversal automaton. So we reuse the object here. It is basically the
     * bounding box.
     */
    AutomatonState   _root;

    GridStatistics   _statistics;

    /**
     * This is not a static master. There's only a real master-worker relation
     * built into this part of the code when we actually split or join.
     *
     * This field should be const (it never changes), but in the MPI case, I
     * wanna be able to construct a tree first and then to set the master.
     * Actually, I should introduce a special constructor for this.
     */
    int            _masterId;
    std::set<int>  _childrenIds;

    /**
     * Indicate per axis whether we have periodic boundary conditions.
     */
    const std::bitset<Dimensions>      _periodicBC;

    /**
     * A split is identified by a tuple of id and cell count which tells the
     * code how many cells should go to a particular id. The actual split then
     * is done in a second iteration, i.e. we first bookmark all split requests
     * and then we roll over the requests in the subsequent iteration to be
     * actually performed.
     */
    typedef std::map< int, int >  SplitSpecification;
    SplitSpecification   _splitTriggered;

    std::set<int>        _splitting;

    /**
     * I need this post-mortem list to identify which tree structures have to be
     * copied/replicated where.
     */
    std::set<int>        _hasSplit;


    constexpr static int NoJoin = -1;
    /**
     * This should be -1 if no join is triggered. Otherwise it holds the id of
     * the joining rank.
     */
    std::set< int >      _joinTriggered;

    /**
     * If master: Set of workers that should join
     */
    std::set< int >      _joining;

    //typedef peano4::maps::STDStackMap< peano4::stacks::GridVertexStack >   GridVertexStackMap;
    typedef peano4::maps::HierarchicalStackMap< peano4::stacks::GridVertexStack >   GridVertexStackMap;

    static GridVertexStackMap  _vertexStack;

    /**
     * We get these control events when we kick off the traversal and then
     * realise/interpret them.
     */
    std::vector< peano4::grid::GridControlEvent >       _gridControlEvents;

    /**
     * Should only be called for inner cells
     */
    void evaluateGridControlEvents(
      const AutomatonState& fineGridState,
      GridVertex            coarseGridVertices[TwoPowerD],
      GridVertex            fineGridVertices[TwoPowerD]
    );

    /**
     * If a cell is given away to another rank, we have to mark the vertices
     * of its mother cell such that we do not coarsen above it and thus
     * effectively remove our whole splitted subpartition.
     */
    void markVerticesAroundParentOfForkedCell(
      GridVertex            coarseGridVertices[TwoPowerD]
    ) const;


    /**
     * You may exchange data horizontally with rank if and only if
     *
     * - a grid entity is local
     * - this predicate holds for rank
     *
     * @see getNeighbourTrees()
     */
    bool doesRankIndexIdentifyHorizontalDataExchange(int rank, bool calledByReceivingProcess) const;

    /**
     *
     *
     * <h2> Veto coarsening </h2>
     *
     * The routine markVerticesAroundParentOfForkedCell() is used in this routine
     * to ensure that we don't erase the grid over a worker domain. We have to do
     * this both on the master and the worker. The worker invokes the veto
     * coarsening when it comes from a coarse remote tree and runs into local fine
     * grid cell. The master does it the other way round: It sets the vetos if
     * we hit a remote cell out from the local tree. In both cases, the bottom-up
     * analysis of the flag ensures that the information propagates all the way up
     * the grid hierarchies.
     *
     */
    void descend(
      const AutomatonState& state,
      GridVertex            vertices[TwoPowerD],
      TraversalObserver&    observer
    );

    /**
     * Every local refined cell should call this routine. We increment the
     * respective counter. Upon storage, we then refine if the counter
     * equals 2^d. This way we avoid hanging vertices within the domain.
     */
    static void incrementNumberOfAdjacentRefinedLocalCells(GridVertex  vertices[TwoPowerD]);

    /**
     * Takes a state (describing a node in the tree) and returns the
     * @f$ 3^d @f$ states on the next finer level along the Peano SFC. This
     * routine is basically the grammar generation of Bader et al. It relies
     * internally on a recursive call of the other refineState() routine
     * along the spatial dimensions.
     *
     * The array fineGrid is filled along the Peano SFC. So you don't have to
     * traverse the array with a zfor - a simple for is sufficient.
     */
    static void refineState(
      const AutomatonState&              coarseGrid,
      AutomatonState                     fineGrid[ThreePowerD],
      tarch::la::Vector<Dimensions,int>  fineGridPosition = tarch::la::Vector<Dimensions,int>(0),
      int                                axis = Dimensions-1
    );

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
      bool          joiningIsConsideredLocal
    ) const;

    /**
     * We run over the @f$ 2^d @f$ adjacent vertices of the cell and look at
     * each vertex's adjacency list. Usually they should all agree on the who's
     * gonna own a cell. It is only hanging vertices which we should exclude
     * from our check. These vertices might carry invalid adjacency lists.
     *
     * Other vertices which might hold invalid data are remote vertices. If a
     * cell adjacent to the local domain changes its owner, the adjacency lists
     * of the shared vertices all are updated. But those vertices further away,
     * inside the remote area, are not updated. So we have to ignore these guys.
     *
     * The latter data inconsistency leads to the case that we might run
     * through all @f$ 2^d @f$ vertices and not find a single vertex where we
     * can be sure that it holds the right data. So we also run a second result
     * datum (weakID). This one is always updated unless we encounter a hanging
     * vertex or an invalid id. So we basically cover this second case which
     * occurs at the vertical interface of two domains: The coarser level is
     * local, the finer level is remote and thus would not yield any owner.
     * However, as we do not rebalance (fork) along vertical cuts, we can
     * trust in the weakId in this case.
     *
     */
    int getTreeOwningSpacetreeNode(
      GridVertex            vertices[TwoPowerD]
    ) const;

    bool areAllVerticesRefined(
      GridVertex            vertices[TwoPowerD]
    ) const;

    bool areAllVerticesUnrefined(
      GridVertex            vertices[TwoPowerD]
    ) const;

    /**
     * Could also be called areAllVerticesPersistent() in the Peano
     * terminology.
     */
    bool areAllVerticesNonHanging(
      GridVertex            vertices[TwoPowerD]
    ) const;


    /**
     * Study the adjacency flags and do ignore hanging nodes.
     *
     * A vertex is remote, if all its adjacent cells are handled by another
     * rank. However, this rank may not have the attribute fork-triggered
     * (because then it does not yet exist) or joining (because then it is
     * already forwarding its work to its master), or forking. The latter
     * case means that the rank is just about to forward all vertices to the
     * new worker, i.e. it does not compute anything anymore on the local
     * vertex, but it still has to do the send/receive stuff, i.e. it still
     * has to handle the vertices.
     *
     * Hanging vertices are never local to the current spacetree.
     */
    bool isVertexAdjacentToLocalSpacetree(
      GridVertex  vertex,
      bool        splittingIsConsideredLocal,
      bool        joiningIsConsideredLocal
    ) const;

    /**
     * Load the vertices of one cell
     *
     * The load process has to be done along the local order of the Peano
     * SFC. For this, I rely on PeanoCurve::getFirstVertexIndex().
     *
     * <h2> Hanging vertices </h2>
     *
     * I originally thought that I might be able to run hanging vertices
     * through the in/out stacks, too. In Peano 3, I had such a feature.
     * Yet, there I had a way more complicated logic. In Peano 4, I simplify
     * the logic. As a consequence, I don't know how many adjacent cells a
     * hanging vertex has and thus create a hanging vertex per adjacent
     * cell.
     */
    void loadVertices(
      const AutomatonState&                        fineGridState,
      GridVertex                                   coarseGridVertices[TwoPowerD],
      GridVertex                                   fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     cellPositionWithin3x3Patch,
      TraversalObserver&                           observer
    );

    void storeVertices(
      const AutomatonState&                        fineGridState,
      GridVertex                                   coarseGridVertices[TwoPowerD],
      GridVertex                                   fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     cellPositionWithin3x3Patch,
      TraversalObserver&                           observer
    );

    /**
     * Little helper. Should likely go into la or utils.
     */
    static tarch::la::Vector<Dimensions,int> convertToIntegerVector( const std::bitset<Dimensions>& in );

    /**
     * This operation has multiple purposes
     *
     * - Merge with neighbour vertices. See
     *   receiveAndMergeGridVertexAtHorizontalBoundary().
     * - Roll over the flags. These guys now are merged already. See
     *   receiveAndMergeGridVertexAtHorizontalBoundary().
     * - Do the refinement flag (state) update.
     * - Erase non-local vertices if they do not carry a veto flag.
     *
     *
     * <h2> Flag update </h2>
     *
     * Implements the standard refinement status transition, i.e. a triggered
     * becomes actually ing. And if a vertex has been refining or erasing in
     * the previous sweep, we finally update it to refined or unrefined.
     *
     * This operation has to be called whenever we load a vertex from the input
     * stream, i.e. we touch it for the very first time. We are not allowed to
     * invoke it after we've created a new vertex.
     *
     * We don't do any statistics here. I've moved all the statistics into
     * updateVertexBeforeStore().
     */
    void updateVertexAfterLoad(
      GridVertex&                               vertex,
      GridVertex                                fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&  fineVertexPositionWithinPatch,
      TraversalObserver&                        observer
    );

    /**
     * Some of the entries in the event are modelled as array (for example the
     * set of neighbours of a vertex), though they are logically sets. So I
     * befill the lists and then eventually remove duplicates from the lists
     * before I hand out the event. As a result, the lists hold set data, i.e.
     * each entry only once.
     *
     * Routine could even be static. Nothing changes in the spacetree state.
     * We actually don't even need it.
     */
    void removeDuplicateEntriesFromAdjancyListInEvent(
      GridTraversalEvent&  event
    ) const;

    /**
     * <h2> Implementation details </h2>
     *
     * Don't use the result's toString() operation in the traceOut statement.
     * The event is not yet totally populated (we don't know the data-specific
     * properties and only befill the generic stuff). As a consequence any
     * toString() will cause valgrind's memchecker to raise (falsely) an alarm.
     */
    GridTraversalEvent createGenericCellTraversalEvent(
      GridVertex              fineGridVertices[TwoPowerD],
      const AutomatonState&   state,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather
    ) const;

    /**
     * Create description of an enter cell traversal.
     *
     * <h2> Identifying NoData entries </h2>
     *
     * The routine sets entries in the load instruction manually from "take from the
     * in stream" to NoData if the entry refers to a remote grid entity. That is, if
     * a face for example is not local, then we should not move it over the streams.
     * After the fork, we should throw it away.
     *
     * Throughout a fork, grid elements remain local on the master. We still call all
     * events there while we are splitting. After that, we have to get rid of the
     * data. This gives us two opportunities: We can either throw away data after
     * the splitting traversal or we can throw it away one iteration later. There is
     * however not really a decision to be made: We send out data (stream) after we
     * have piped data to hte output stream. That is, we still have to write all user
     * data to the output stream throughout the splitting traversal even if this data
     * will become remote afterwards. It is the output stream from where the spacetree
     * will pick data and stream it over. So we have to use the iteration after
     * splitting to get rid of data. Getting rid means simply: load it but do not
     * store it anymore. It helps us in this context that data streams are
     * monotoneous. We throw away stuff but we never re-create stuff (cmp joining
     * below which poses an exception).
     *
     * So, when we decide whether to load data, we first of all check whether a grid
     * entity is local. If so, we have to load it from the input stream. If it is
     * not local, we have to check whether a rank that is contained within _hasSplit
     * is contained. If this is the case, we still load the data despite the fact
     * that we might not run any routine on them.
     *
     * @todo Joining is not discussed or implemented yet.
     *
     * @param createEnterCellTraversalEvent Piped through to createGenericCellTraversalEvent().
     */
    GridTraversalEvent createEnterCellTraversalEvent(
      GridVertex              coarseGridVertices[TwoPowerD],
      GridVertex              fineGridVertices[TwoPowerD],
      const AutomatonState&   state,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather
    ) const;

    /**
     * Create description of a leave cell traversal.
     */
    GridTraversalEvent createLeaveCellTraversalEvent(
      GridVertex              coarseGridVertices[TwoPowerD],
      GridVertex              fineGridVertices[TwoPowerD],
      const AutomatonState&   state,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather
    ) const;


    /**
     * When we fork or join, the worker's locality analysis identifies local
     * vertices and faces. It also identifies local cells. That's all correct.
     * However, despite the fact that they are local, we should not invoke
     * any user event on them. We should move data over the stacks, and we
     * should (maybe) exchange data, but we should never call user code within
     * the observers. So eventually, we need a pruned version of the event
     * with all local flags unset. We still need the version of the event
     * with set local flags, as they feed into the boundary data exchange.
     * But for enterCell and leaveCell, we need copies without these flags.
     * Pruned copies.
     *
     * <h2> Rationale </h2>
     *
     * I originally thought about having the pruning mechanism as a part of
     * createEnterCellTraversalEvent() or createLeaveCellTraversalEvent(). This
     * does not work however, as the data exchange et al need the real inside/
     * outside flag whereas a pruned version of the event might disable all of
     * these flags to effectively switch off the invocation of user events.
     */
    GridTraversalEvent createPrunedCellTraversalEvent( const GridTraversalEvent& event ) const;


    /**
     *
     *
     * <h2> Implementation </h2>
     *
     * Run through the vertices in the Peano vertex order (modified z depending on
     * curve orientation). This information is already implicitly encoded in the order
     * of the indices within the event. So no fancy arithmetics is required here
     * anymore.
     */
    void receiveAndMergeUserData(const AutomatonState& state, TraversalObserver&  observer, const GridTraversalEvent&  enterCellTraversalEvent, GridVertex  fineGridVertices[TwoPowerD]);

    /**
     *
     * <h2> Data order on output stacks </h2>
     *
     * When we send out data, we send that data from the output stack, i.e. we assume that
     * the user code has already piped its stuff there. As the output data structure is a
     * stack, we have to be careful: If a cell writes N pieces of data to the output stream
     * and we want to copy the first piece of data into an output stream for MPI, then we
     * have to pick the Nth entry from the output stream. With our modified top(), we can
     * access these elements directly. We just have to be careful that top() equals actually
     * a top(0), so picking the Nth element requires us to call top(N-1).
     */
    void sendUserData(const AutomatonState& state, TraversalObserver&  observer, const GridTraversalEvent&  enterCellTraversalEvent, GridVertex  fineGridVertices[TwoPowerD]);


    /**
     * @see splitOrJoinCell() For both the usage and a description how we
     *   hijack the routine.
     */
    std::vector<int>  _splittedCells;

    /**
     * We do realise a @f$LR(3^d)@f$ grammar to identify admissible splits.
     * First, this routine checks whether splits are active. If so, we do
     * study the split condition or we run into the move branch (which is
     * essentially a split of the whole tree to a different tree).
     *
     * <h2> Split </h2>
     *
     * A node may be split if the parent is still no the same node (so we
     * always have an intermediate mesh layer and preserve the tree topology)
     * and if the node does not act as parent to another rank. If we did
     * omit the latter check, a split might alter the tree topology. Which
     * we don't like.
     *
     * The first check is trivial. The second check is realised through a
     * pushback container and an LR(k) lookup.
     *
     * - Every leaf pushes a marker into this container: _id if it is and
     *   remains local, the new id if the node has been split, and a -1
     *   otherwise.
     * - A refined cell induces a reduce on the container. @f$ 3^d@f$ markers
     *   are removed and analysed: If they all have the same ip as the next
     *   split marker to be used, we add it.
     *
     * - A refined cell finally adds its marker again.
     *
     * It is important that I evaluate the reduce analysis even if no splits
     * are do be done anymore. Because it might happen that I have just done
     * exactly 9 splits for example (2d) and thus, the parent of these 9
     * guys should go to the remote node, too.
     *
     * <h2> Count down the split counter </h2>
     *
     *
     *
     */
    void splitOrJoinCell(
      GridVertex                                vertex[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD]
    );

    /**
     *
     * <h2> Merge process </h2>
     *
     * If we receive a remote cell, then this cell is not refined. In this case
     * the (former) workers holds all the valid adjacency data: We might on a
     * master receive a cell right from the middle of the worker's domain where
     * the master has no clue about any adjacency. So we might think that we can
     * copy over the (former) worker's adjacency data.
     *
     * As we also receive hanging vertices from the worker, we can safely (and
     * pretty naively) copy over the adjacency. THe first non-hanging vertex
     * will bring in the right adjacency information.
     *
     * There is however one tricky special case. Assume that a rank A serves as
     * master to B and C and both of these workers merge into A. We furthermore
     * study one vertex in-between B and C. The merge runs as follows:
     *
     * - Rank B tells rank C that it will merge its cell into A.
     * - Rank C tells rank B that it will merge its cell into A.
     * - Both ranks update their respective local adjacency lists.
     * - Rank B streams its vertex up to rank A.
     *
     * If we now simply copied the adjacency list from B, we'd loose the
     * information that a cell is adjacent to C. B has already updated its list,
     * so it is slightly "ahead" of time.
     *
     * So for a successful merge, it is important that we actually only reset
     * the flags of this very cell. We do so through
     * updateVertexRanksWithinCell(). This logic relies on the fact that we
     * keep adjacency flags of all vertices which are remote yet just one level
     * below the current rank. This check, realised in updateVertexBeforeStore(),
     * ensures we kind of know what we do.
     *
     *
     * <h2> Joins </h2>
     *
     * If we join (parts of) a worker into the local partition, it can happen
     * that the rank receives a vertex that used to be remote before. As a
     * consequence, the local copy of a vertex holds invalid data whereas the
     * incoming data holds up-to-date adjacency information. So if a vertex is
     * remote locally, we take the worker's vertex to deliver accurate adjacency
     * data. The only alteration we make is that we replace the
     * RankOfCellWitchWillBeJoined markers with the rank of the worker.
     * RankOfCellWitchWillBeJoined is used to mark those cells which go to the
     * worker in the join triggered phase, i.e. it is used only on the worker
     * and only to prepare a join.
     *
     *
     * @see updateVertexBeforeStore()
     */
    void mergeCellFromWorkerWithMaster(
      GridVertex                                vertex[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD]
    );

    /**
     *
     * <h2> Implementation </h2>
     *
     * We only remove adjacency information for unrefined outside vertices
     * and we are very restrictive what we consider to be outside. Indeed,
     * we consider any vertex still local when we are in a join or fork.
     * If a vertex is really outside but refined, we wait for the erase to
     * pass through before we consider erasing any adjacency info.
     *
     * @see updateVertexAfterLoad Erases refined vertices outside of local
     *                            domain.
     */
    bool shouldEraseAdjacencyInformation(
      const GridVertex&                  vertex,
      GridVertex                         coarseGridVertices[TwoPowerD],
      tarch::la::Vector<Dimensions,int>  fineVertexPositionWithinPatch
    ) const;

    /**
     * <h2> Restriction of veto flags </h2>
     *
     * We make each vertex hold a flag isAntecessorOfRefinedVertex. If it is
     * set, we veto any erasing here. This way, we can ensure that the trees
     * remove at most one level at a time. We illustrate all further
     * explanations with a simple example:
     *
     * @image html Spacetree_updateVertexBeforeStore_restrictIsAntecessorOfRefinedVertex.png
     *
     * Let the red vertices be refined. In a serial code, the tree is not
     * split. You have to glue together the left and right tree. As the middle
     * level's vertex holds the refinement flag, the top level vertex carries
     * the flag isAntecessorOfRefinedVertex. It consequently never is erased.
     * Users first have to remove the finer level.
     *
     * If we split up tree into two threads, this approach fails if the flag
     * is restricted per core in the bottom-up steps. By the end of the sweep,
     * the flag is set on the right tree, but it is not set on the left tree.
     * We consequently might decide to erase on the left tree but veto this on
     * the right tree.
     *
     * To eliminate this behaviour, we split up the flag into a current flag
     * and a flag from the previous solution. This flag is rolled over. If the
     * flag is set, it undoes any erase trigger.
     *
     * <h2> Clean-up of vertices </h2>
     *
     * I originally had the following idea:
     *
     * If a vertex is remote, we manually clear all of its adjacency flags.
     * This is a security thing. It might happen that such a vertex refers to
     * a tree x and that this index x is later re-used (for a new local subtree
     * for example). In this case, a left-over index might suddenly suggest that
     * a totally irrelevant vertex is adjacent to the newish x tree.
     *
     * However, this is wrong in some cases: We have to keep our adjacency.
     * Because if we merge later on, we rely on this adjacency to find out
     * which vertices we have to join.
     *
     * So we have to run for the middle way: We erase all adjacency data but
     * if ad only if a vertex is not adjacent to any kid.
     *
     *
     * @param fineVertexPositionWithinPatch Position of vertex within 3x3 or 3x3x3 patch respectively
     *
     * @see updateVertexAfterLoad()
     * @see mergeCellFromWorkerWithMaster() for an explanation why we have to
     *         keep all adjacency information that we actually need later for
     *         merges.
     */
    void updateVertexBeforeStore(
      GridVertex&                               vertex,
      GridVertex                                fineGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&  fineVertexPositionWithinPatch
    );

    static bool restrictToCoarseGrid(
      const tarch::la::Vector<Dimensions,int>&  coarseVertexPosition,
      const tarch::la::Vector<Dimensions,int>&  fineVertexPosition
    );

    /**
     * If a cell gets a new id, we have to update its vertices. This routine is
     * only used for splits. I originally thought I might use it for joins as
     * well. Indeed I can. But I can only do this on the master. The worker may
     * not update any cell immediately. If I do this, then the adjacency
     * information of the adjacent vertices is overwritten and I loose this
     * clue that these vertices are adjacent to the local, joining rank.
     *
     * If we split, we have to be careful that we preserve the master-worker
     * topology over the ranks. So this routine may only be called on the master,
     * on a cell that is local, and on a cell whose parent is local, too.
     */
    static void updateVertexRanksWithinCell(
      GridVertex  fineGridVertices[TwoPowerD],
	  int         newId
    );

    tarch::la::Vector<TwoPowerD,int> getAdjacentRanksForNewVertex(
      GridVertex                                   coarseGridVertices[TwoPowerD],
      const tarch::la::Vector<Dimensions,int>&     vertexPositionWithin3x3Patch
    ) const;

    /**
     *
     *
     * <h2> Admissible splits </h2>
     *
     * Splits must preserve a master-worker, i.e. a tree topology. In the
     * example below, a grey rank is the worker of a blue rank (left). If
     * the grey rank splits up, there always has to be a layer in-between
     * the blue and green rank to preserve a unique master-worker topology.
     * Therefore, the middle layout is not admissible whereas the right one
     * is.
     *
     * @image html Spacetree_split.png
     *
     * There are many reasons to enforce this constraints. The two most
     * important ones are:
     *
     * - I want to have a clear tree topology on the MPI ranks.
     * - These splits make joins easier. Joins are allowed between
     *   masters and workers (in the very tree topology) and only for
     *   ranks which hold no refined local cells. However, to be able to
     *   realise the join, the master has to cover the whole area (on a
     *   coarser level) joining into its area. If we did not enforce the
     *   tree topology, i.e. the level in-between two ranks, then the
     *   greyish rank in the example above would fork away a subdomain and
     *   consequently coarsen its grid where the green rank now steps in.
     *   So this grid is "lost" and it can't join these data in anymore.
     */
    void split(int treeId, int fineGridCells);

    /**
     * Get the ids of the surrounding cells of a vertex.
     *
     * This operation studies the vertex only. Please check manually whether
     * your code is in the states SpacetreeState::NewFromSplit or
     * SpacetreeState::EmptyRun. Joining is not taken into account
     * either. So as a summary: I do analyse the vertex data and I do
     * take into account whether subranks are currently joining or
     * triggered to split. But I do ignore the current spacetree's
     * state.
     *
     * The operation returns the empty set if a vertex is not local.
     * It also returns the empty set if a vertex is hanging.
     */
    std::set<int>  getNeighbourTrees( const GridVertex& vertex, bool calledByReceivingProcess ) const;

    /**
     * Get the ids of the surround ids of a face.
     *
     * <h2> Implementation remarks </h2>
     *
     * The domain ids (adjacency lists) along the boundary tell us what the neighbour
     * number is. If a neighbouring rank has triggered a split, we get an updated
     * adjacency list for affected vertices. This list is immediately merged into the
     * the local vertex. The new entries however are only to be taken into account
     * when we send data. For receives, we should stick to the old entries. Therefore,
     * we use the backup of the adjacency list when we receive data, but we use the
     * updated/new list when we send out stuff.
     *
     * If we bump into a new vertex, we should explicitly ignore it when we receive.
     * After all, new vertices cannot yet have triggered incoming data. The counterpart
     * is deleted vertices which should not contribute towards a send command.
     *
     * @return -1  (TraversalObserver::NoData) if there's no neighbour or face is not local.
     */
    int  getNeighbourTrees( GridVertex vertex[TwoPowerD], int faceNumber, bool calledByReceivingProcess ) const;

    /**
     * @see getNeighbourTrees()
     */
    tarch::la::Vector< TwoPowerD, int >  getAdjacentRanksOfFace( GridVertex vertex[TwoPowerD], int faceNumber, bool calledByReceivingProcess ) const;



    /**
     * This one is to be invoked if and only if a vertex goes to the in/out
     * stacks. The routine should be called just before the vertex goes to
     * the output stack. We call it in updateVertexBeforeStore() here, so
     * this constraints automatically is followed.
     *
     * The routine builds up a set over integers into which it adds all ranks
     * that have to receive a copy. Once this is done, it iterates over the
     * set and sends out data. The two-step scheme becomes necessary, as we
     * have to avoid that vertices are sent around multiple times.
     *
     * As this routine is called from within updateVertexBeforeStore(), we
     * may assume that this is not an empty tree run.
     */
    void sendGridVertex( const GridVertex& vertex );

    /**
     * Manage the data exchange after a vertex is loaded for the first time
     *
     * The operation has three jobs to do:
     * - We backup the adjacency ranks.
     * - Exchange vertices along domain boundary.
     * - Exchange vertices belonging to periodic boundaries.
     *
     * The order of these three steps is important. The first one is a simple
     * copy. The other ones loop over neighbours and call a series of operations.
     * Logically, domain boundaries and periodic boundaries for me are both
     * realised by domain cuts. Therefore, we use the same routines within.
     *
     * <h2> Backup of adjacency data </h2>
     *
     * It is convenient to merge the adjacency flags straightaway after a vertex
     * has been loaded and its boundary counterparts have dropped in. However, once
     * we merge we loose the information about the previous traversal's adjacency.
     * This means, when we construct the neighbour information (who merges with
     * what) for the user data (createEnterCellTraversalEvent()) we lack the
     * information we actually need. Therefore, this routine backups the
     * adjacency information from the vertex.
     *
     * <h2> Boundary data exchange (grid) </h2>
     *
     * For a local vertex, i.e. one adjacent to the current active tree, which
     * is neighbouring another tree, we do receive this tree's data copy and
     * then merge it into the local tree.
     *
     * This approach doesn't work straightforwardly when we split: Lets assume
     * tree 0 is triggered to split into 0 and 1. In a first step, tree 0 enters
     * the state split triggered. It now updates all adjacency lists, so
     * boundary data is already sent out to 1 - even though 1 is not instantiated
     * yet. Once we are done, we copy over (parts of) tree 0 into tree 1, so
     * tree 1 now does exist and is well-prepared to receive the data already
     * dropped by 0. This second step is the actual splitting step. Now, other
     * ranks have still sent in data to 0 which actually should go to 1. We
     * receive those guys, but we have to eliminate explicitly any incoming data
     * we'd expect from 1 as 1 didn't have the chance yet to send it out.
     *
     * The function is called directly
     * after a vertex has been read from the input stream. Please note that
     * an update of the refinement states (e.g. switch from
     * refinement-triggered to refining) happens after the merge. Any update of the
     * refinement state in this operation hence immediately affects the
     * vertex updates in this very iteration.
     *
     * The update of the adjacency information is simple: If a neighbour tells us
     * that it has changed an adjacency entry in one of its own fields, i.e. if
     * it has changed its own entry, we copy this information over. Otherwise, we
     * ignore any updates (there should not be any).
     *
     * <h2> Periodic boundary conditions </h2>
     *
     * Periodic boundary conditions fall back to standard boundary data
     * exchanges through specialised stacks. They are supported on tree 0 only.
     * Different to standard boundaries, we don't have to update any adjacency data
     * here, as all periodic values are always handled on spacetree 0.
     *
     * <h2> Horizontal vs. vertical </h2>
     *
     * Has to happen before receiveAndMergeGridVertexAtVerticalBoundary(). See the
     * receiveAndMergeGridVertexAtVerticalBoundary()'s documentation for an explanation.
     * Some more reasons are given in the guidebook.
     *
     * <h2> Context </h2>
     *
     * The routine is called by updateVertexAfterLoad(). The counterpart of the
     * routine is sendGridVertex(). However, as my sends are literally
     * just memcopies, sends are way simpler than this routine. The operation
     * affects solely the grid's vertices. It does not interfere with any user
     * data. In principle, this follows my pattern that the grid has to be there
     * first and then events are triggered afterwards.
     */
    void receiveAndMergeGridVertexAtHorizontalBoundary( GridVertex& vertex );

    /**
     * This is a merge routine for vertical data exchange. It is important
     * that you merge
     *
     * <h2> Join behaviour </h2>
     *
     * If we are joining in another rank, we might get update adjacency information
     * for this rank. Such adjacency updates are important for the outgoing data,
     * i.e. for the send. Logically, the master and the dying worker run in parallel,
     * that is the updates on the worker belong logically into steps after the actual
     * load.
     *
     * It therefore is important that you invoke this operation afer
     * receiveAndMergeGridVertexAtVerticalBoundary() and that this routine overwrites the
     * adjacentRanks yet not the backup of this field. This way, we are consistent
     * with any horizontal merges on the worker.
     *
     * @see receiveAndMergeGridVertexAtHorizontalBoundary()
     */
    void receiveAndMergeGridVertexAtVerticalBoundary( GridVertex& vertex );

    /**
     * Called by receiveAndMergeGridVertexAtHorizontalBoundary(). Besides tons of assertions,
     * the routine realises the mergers of the refinement flags and the
     * setIsAntecessorOfRefinedVertexInCurrentTreeSweep. Please consult
     * receiveAndMergeGridVertexAtHorizontalBoundary() for a higher level overview.
     */
    void mergeGridVertexRefinementStateAtHorizontalDomainBoundary( GridVertex& vertex, const GridVertex& inVertex, int neighbour );
    void mergeGridVertexAdjacencyListsAtHorizontalDomainBoundary( GridVertex& vertex, const GridVertex& inVertex, int neighbour );

    /**
     * Only used by SpacetreeSet to create children of the original tree.
     *
     * We have to set the stats's stationary counter manually, as clear() does not
     * reset it. We furthermore set it to -2, as we'll need two iterations to set
     * a new remote spacetree up.
     */
    Spacetree(
      int newId,
      int masterId,
      const tarch::la::Vector<Dimensions,double>&  offset,
      const tarch::la::Vector<Dimensions,double>&  width,
      bool  traversalInverted
    );

    /**
     * Join with master. Call this routine only for degenerated trees,
     * i.e. for trees without any refined local cells.
     *
     * We have to be careful with the merges. Often when we fork a tree,
     * this tree first is not refined further. We need a couple of sweeps
     * to see whether a tree is refined further or not.
     */
    void joinWithMaster();
    void joinWithWorker(int id);

    /**
     * Only ranks that have no kids are allowed to join. This is
     * implicitly ensured as we have only degenerated trees here,
     * i.e. we only allow a tree to join if it does not host local
     * spacetree nodes. Furthermore, we require a tree to be
     * stationary, i.e. to linger around like that for a while.
     *
     * @see join()
     */
    bool mayJoinWithMaster() const;

    /**
     * We allow at most one join at a time and not while we split
     */
    bool mayJoinWithWorker() const;

    bool maySplit() const;

    /**
     * @return Id of splitting tree or -1 if there's none.
     */
    int getSplittingTree() const;
    void updateSplittingCounter( int treeId );
  public:
    Spacetree(
      const tarch::la::Vector<Dimensions,double>&  offset,
      const tarch::la::Vector<Dimensions,double>&  width,
      const std::bitset<Dimensions>&               periodicBC = 0
    );

    ~Spacetree();

    /**
     * @param calledFromSpacetreeSet If you use traverse directly, please do
     *     not alter this value
     */
    void traverse(TraversalObserver& observer, bool calledFromSpacetreeSet = false);

    GridStatistics getGridStatistics() const;

    std::string toString() const;

    bool isInvolvedInJoinOrFork() const;
};


#endif
