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

    //      const AutomatonState& fineGridState,
 //     const SplitSpecification&                 splitTriggered,
 //     const std::set<int>&                      splitting,
 //     const std::set< int >&                    joinTriggered,
  //    const std::set< int >&                    joining,
};


#endif
