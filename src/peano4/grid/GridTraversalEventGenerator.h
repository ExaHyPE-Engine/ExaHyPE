// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"

#include "grid.h"
#include "GridTraversalEvent.h"
#include "AutomatonState.h"
#include "GridVertex.h"

namespace peano4 {
  namespace grid {
    class GridTraversalEventGenerator;

    namespace tests {
      class GridTraversalEventGeneratorTest;
    }
  }
}

/**
 * Translate grid traversal automaton's transitions into user events
 *
 * The majority of these routines is all about identifying ownership. So we might also
 * call the class something around ownership analysis.
 *
 * ## Event order
 *
 * The generated events trigger the following things per grid sweep on the user side:
 *
 * - beginTraversal: Called once per rank prior to the actual grid traversal.
 * - endTraversal: Called once per rank at the end of the traversal.
 * - createPersistentVertex: First thing that is called for a new vertex.
 * - destroyPersistentVertex: Very last thing that is called if a vertex is deleted
 *   becomes a hanging vertex.
 * - createHangingVertex: Can be called arbitrarily often per grid traversal for a
 *   hanging vertex. You don't know how often hanging vertices are created/destroyed,
 *   but you can be sure that this action is triggered before anything else gets
 *   access to a hanging vertex. Please note that we call touchVertexFirstTime()
 *   for any newly created vertex. However, we do not call touchVertexFirstTime()
 *   for hanging vertices.
 * - destroyHangingVertex: Every createHangingVertex() pairs up with one of these
 *   guys. As hanging vertices are non-persistent, it is at least called once per
 *   traversal per hanging vertex. However, Peano might decide to create hanging
 *   vertices multiple times. It hence might also destroy them multiple times.
 * - createPersistentFace: The very first thing called for a new persistent face.
 *   Precedes any touchFirst operation.
 * - destroyPersistentFace: Last thing done for a face that is destroyed or becomes
 *   hanging.
 * - createHangingFace: Counterpart of vertex operation.
 * - destroyHangingFace: Pairs up with creational event.
 * - createCell: Called if we create a new cell before anything else is done for this
 *   cell. By the time this operation is called, you can be sure however that all of
 *   its @f$ 2^d @f$ vertices and $2d$ faces are there - either hanging or real ones.
 * - destroyCell: This routine precedes any destruction of adjacent faces or vertices.
 *   So we have the creational sequence vertices-faces-cells and we destroy the other
 *   way round: cells-faces-vertices.
 * - touchVertexFirstTime: Called exactly once per rank per grid traversal per vertex,
 *   before any action for any adjacent face or cell is triggered.
 * - touchVertexLastTime: Last thing before the vertex is safely stored away for the
 *   next grid sweep or destroyed.
 * - touchFaceFirstTime: Called exactly once per rank per grid traversal per face. By
 *   the time a face is touched first, its @f$ 2^{d-1} @f$ adjacent vertices have been
 *   touched already.
 * - touchFaceLastTime: Counterpart of touchFaceFirstTime.
 * - touchCellFirstTime: Called once per rank per cell. When you bump into a cell, you
 *   can be sure that all adjacent faces and vertices have been loaded (aka touched)
 *   already. If this is a fine grid cell, the code will immediately afterwards issue
 *   the touchCellLastTime action. If the cell is refined, the code will first descend
 *   within the tree.
 * - touchCellLastTime: Called just before the tree depth-first traversal leaves a
 *   cell. This is the backtracking within the tree.
 *
 */
class peano4::grid::GridTraversalEventGenerator {
  private:
    friend class peano4::grid::tests::GridTraversalEventGeneratorTest;

    static tarch::logging::Log _log;

    /**
     * Number of underlying tree
     */
    const int _id;

    /**
     * <h2> Implementation details </h2>
     *
     * Don't use the result's toString() operation in the traceOut statement.
     * The event is not yet totally populated (we don't know the data-specific
     * properties and only befill the generic stuff). As a consequence any
     * toString() will cause valgrind's memchecker to raise (falsely) an alarm.
     *
     * @param spacetreeStateIsRunning  spacetreeState == SpacetreeState::Running
     */
    GridTraversalEvent createGenericCellTraversalEvent(
      GridVertex                                coarseGridVertices[TwoPowerD],
      GridVertex                                fineGridVertices[TwoPowerD],
      const AutomatonState&                     state,
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather,
      bool                                      spacetreeStateIsRunning
    ) const;

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
     * A vertex is inside the domain, if all of its ids equal _id. We use the
     * backup of the ids here, as we study the current (committed) state. The
     * routine runs over all @f$ 2^d @f$ vertices and analyses their status
     * w.r.t. inside.
     */
    std::bitset<TwoPowerD> areVerticesAdjacentToParallelDomainBoundary(
      GridVertex                                vertices[TwoPowerD],
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      bool calledByLeaveCell
    ) const;

    std::bitset<TwoTimesD> areFacesAdjacentToParallelDomainBoundary(
        GridVertex                                vertices[TwoPowerD],
        const SplitSpecification&                 splitTriggered,
        const std::set<int>&                      splitting,
        const std::set< int >&                    joinTriggered,
        const std::set< int >&                    joining,
        bool calledByLeaveCell
      ) const;

    /**
     * Vertices are local. I consider splitting and joining vertices to be
     * local, too. It is therefore consistent with areFacesLocal().
     */
    std::bitset<TwoPowerD> areVerticesLocal(
      GridVertex                  vertices[TwoPowerD],
      const SplitSpecification&   splitTriggered,
      const std::set<int>&        splitting,
      const std::set< int >&      joinTriggered,
      const std::set< int >&      joining
    ) const;

    /**
     * Identifies for the @f$ 2 \cdot d @f$ faces whether they are local or not.
     *
     * ## Implementation
     *
     * - I loop over the 2d faces.
     * - Per face, I loop over all @f$ 2^d @f$ vertices but alter the entry
     *   along the normal manually. So I collapse the vertex index along the
     *   normal. This is inefficient, as I'm effectively checking each vertex
     *   twice, but I don't care.
     * - Per relevant vertex, I have to check two entries in the adjacency
     *   list.
     * - Splitting and split-triggered ranks are still considered to be
     *   local.
     *
     * ## Hanging vertices
     *
     * I assume that all adjacency information/locality information on vertices
     * is correctly set, i.e. is interpolated from the next coarser level. So
     * I have valid neighbours on the vertex per se. Actually, however, the
     * neighbour might be wrong, as an adjacent cell might have been given away
     * to another rank.
     *
     * We however have to be aware that a hanging vertex entry might be wrong if
     * the corresponding adjacency value is not the one pointing towards our cell
     * and the vertex is also hanging. So these constellations have to be ignored.
     *
     * @see peano4::grid::Spacetree::getAdjacentRanksForNewVertex() for a
     *   discussion how adjacency lists are computed.
     * @see peano4::grid::Spacetree::loadVertices() for the location where these
     *   lists are set for hanging and new vertices.
     */
    std::bitset<TwoTimesD> areFacesLocal(
      GridVertex  vertices[TwoPowerD],
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining
    ) const;

  public:
    GridTraversalEventGenerator(int id);

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
     * We assume that the adjacency information of hanging vertices is
     * properly set. So I have valid neighbours. Actually, the neighbour might
     * be wrong, but if it is the local neighbour, then it is correct.
     * Therefore, we can distinguish hanging remote and hanging local vertices.
     *
     * @see peano4::grid::Spacetree::getAdjacentRanksForNewVertex() for a
     *   discussion how adjacency lists are computed.
     * @see peano4::grid::Spacetree::loadVertices() for the location where these
     *   lists are set for hanging and new vertices.
     *
     * The treatment of hanging nodes in the action sets digesting the
     * events thus differs from how we handle hanging nodes within the core
     * spacetree. For the lattern, hanging vertices are never local to the
     * current spacetree.
     *
     * @see Spacetree::isVertexAdjacentToLocalSpacetree() for further remarks
     *   on the difference between hanging vertices within the meshes and
     *   for events.
     */
    bool isVertexAdjacentToLocalSpacetree(
      GridVertex  vertex,
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      bool        splittingIsConsideredLocal,
      bool        joiningIsConsideredLocal
    ) const;

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
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      bool          splittingIsConsideredLocal,
      bool          joiningIsConsideredLocal
    ) const;

    /**
     * Create description of an enter cell traversal.
     *
     * # Identifying NoData entries
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
     * have piped data to the output stream. That is, we still have to write all user
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
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      const std::set< int >&                    hasSplit,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather,
      bool                                      spacetreeStateIsRunning
    ) const;

    /**
     * Create description of a leave cell traversal.
     */
    GridTraversalEvent createLeaveCellTraversalEvent(
      GridVertex              coarseGridVertices[TwoPowerD],
      GridVertex              fineGridVertices[TwoPowerD],
      const AutomatonState&   state,
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining,
      const std::set< int >&                    hasSplit,
      const tarch::la::Vector<Dimensions,int>&  relativePositionToFather,
      bool                                      spacetreeStateIsRunning
    ) const;

    /**
     * When we fork or join, the worker's locality analysis identifies local
     * vertices and faces. It also identifies local cells. That's all correct.
     * However, despite the fact that they are local, we should not invoke
     * any user event on them. We should move data over the stacks, and we
     * should (maybe) exchange data, but we should never call user code within
     * the observers, as the reponsibility of the user operations remains with
     * the original owner prior to the fork or join. See links below.
     *
     * Therefore, we need a pruned version of the event
     * with all local flags unset. We still need the version of the event
     * with set local flags, as they feed into the boundary data exchange.
     * But for enterCell and leaveCell, we need copies without these flags.
     * Pruned copies.
     *
     * The actual data transfer, i.e. which data goes from one stack to which
     * other one, has to remain in place however: The additional sweeps that
     * we run on a new rank/core are there to get the spacetree data into the
     * right order and to send out all boundary data. It is thus essential
     * that the data transfers stay in. It is "just" the user routines
     * (computations) that shall not be invoked and therefore we have to unset
     * the local flags.
     *
     * # Rationale
     *
     * I originally thought about having the pruning mechanism as a part of
     * createEnterCellTraversalEvent() or createLeaveCellTraversalEvent(). This
     * does not work however, as the data exchange et al need the real inside/
     * outside flag whereas a pruned version of the event might disable all of
     * these flags to effectively switch off the invocation of user events.
     *
     * @see peano4::parallel::SpactreeSet::streamDataFromSplittingTreesToNewTrees()
     */
    GridTraversalEvent createPrunedEnterCellTraversalEvent( SpacetreeState spacetreeState, const GridTraversalEvent& event ) const;
    GridTraversalEvent createPrunedLeaveCellTraversalEvent( SpacetreeState spacetreeState, const GridTraversalEvent& event ) const;

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
      GridVertex            vertices[TwoPowerD],
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining
    ) const;

    /**
     * @see getNeighbourTrees()
     * @param useBackedUpAdjacencyInformation If this one is true, I use the backup of the
     *  adjacency list and not the new data. In most cases, I could thus call
     *  it calledByReceivingProcess.
     */
    tarch::la::Vector< TwoPowerD, int >  getAdjacentRanksOfFace(
      GridVertex vertex[TwoPowerD],
      int faceNumber,
      bool useBackedUpAdjacencyInformation
    ) const;
};
