// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_GRID_TRAVERSAL_EVENT_GENERATOR_H_
#define _PEANO4_GRID_GRID_TRAVERSAL_EVENT_GENERATOR_H_

#include "tarch/tests/TestCase.h"
#include "tarch/logging/Log.h"


#include "grid.h"
#include "GridTraversalEvent.h"
#include "AutomatonState.h"
#include "GridVertex.h"





namespace peano4 {
  namespace grid {
    class GridTraversalEventGenerator;
  }
}



class peano4::grid::GridTraversalEventGenerator {
  private:
    static tarch::logging::Log  _log;

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
      GridVertex              fineGridVertices[TwoPowerD],
      const AutomatonState&                        state,
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
  public:
    GridTraversalEventGenerator(int id);

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
    std::bitset<TwoTimesD> areFacesLocal(
      GridVertex  vertices[TwoPowerD],
      const SplitSpecification&                 splitTriggered,
      const std::set<int>&                      splitting,
      const std::set< int >&                    joinTriggered,
      const std::set< int >&                    joining
    ) const;


    std::bitset<TwoPowerD> areVerticesInsideDomain(GridVertex  vertices[TwoPowerD]) const;


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
     */
    tarch::la::Vector< TwoPowerD, int >  getAdjacentRanksOfFace(
      GridVertex vertex[TwoPowerD],
      int faceNumber,
      bool calledByReceivingProcess
    ) const;

};


#endif
