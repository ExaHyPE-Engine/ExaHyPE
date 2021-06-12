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
};


#endif
