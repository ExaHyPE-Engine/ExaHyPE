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

//      const AutomatonState& fineGridState,
 //     const SplitSpecification&                 splitTriggered,
 //     const std::set<int>&                      splitting,
 //     const std::set< int >&                    joinTriggered,
  //    const std::set< int >&                    joining,
};


#endif
