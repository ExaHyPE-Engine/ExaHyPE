// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_GRID_EMPTY_TRAVERSAL_OBSERVER_H_
#define _PEANO4_GRID_EMPTY_TRAVERSAL_OBSERVER_H_


#include "TraversalObserver.h"


#include "tarch/logging/Log.h"


namespace peano4 {
  namespace grid {
    class EmptyTraversalObserver;
  }
}


/**
 * Empty observer
 *
 * I usually use this only for debugging reasons. The name is not 100 percent
 * correct. The observer at least logs its state transitions.
 */
class peano4::grid::EmptyTraversalObserver: public peano4::grid::TraversalObserver {
  private:
    static tarch::logging::Log  _log;

  public:
    void beginTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void endTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void enterCell(
	    const GridTraversalEvent&  event
    ) override;

    void leaveCell(
      const GridTraversalEvent&  event
    ) override;

    TraversalObserver* clone(int spacetreeId) override;
    std::vector< GridControlEvent > getGridControlEvents() const override;
};

#endif
