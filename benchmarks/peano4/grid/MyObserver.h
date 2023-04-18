// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_GRID_MYOBSERVER_H_
#define _EXAMPLES_GRID_MYOBSERVER_H_


#include "peano4/grid/TraversalObserver.h"
#include "peano4/grid/TraversalVTKPlotter.h"


namespace examples {
  namespace grid {
    class MyObserver;
  }
}


class examples::grid::MyObserver: public
  #if PeanoDebug>0
  peano4::grid::TraversalVTKPlotter
  #else
  peano4::grid::TraversalObserver
  #endif
{
  private:
    static int _iterationCounter;
  public:
    static constexpr int GridRefinementIterations = 16;
    static constexpr int StationaryIterations     = 4;

    MyObserver(int spacetreeId = -1, int counter=0);
    virtual ~MyObserver();

    void beginTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void endTraversal(
      const tarch::la::Vector<Dimensions,double>&  x,
      const tarch::la::Vector<Dimensions,double>&  h
    ) override;

    void enterCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;


    void leaveCell(
      const peano4::grid::GridTraversalEvent&  event
    ) override;

    /**
     * I use the clone to create one observer object per traversal thread. So
 	   * between different spacetrees of one spacetree set, there can be no race
	   * condition. Yet, the clone() itself could be called in parallel.
	   */
    TraversalObserver* clone(int spacetreeId) override;

    std::vector< peano4::grid::GridControlEvent > getGridControlEvents() override;
};

#endif
