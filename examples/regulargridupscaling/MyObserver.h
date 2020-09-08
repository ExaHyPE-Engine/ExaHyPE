// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAMPLES_REGULAR_GRID_UPSCALING_MYOBSERVER_H_
#define _EXAMPLES_REGULAR_GRID_UPSCALING_MYOBSERVER_H_


#include "peano4/grid/TraversalObserver.h"
#include "tarch/logging/Log.h"


namespace examples {
  namespace regulargridupscaling {
    class MyObserver;
  }
}


class examples::regulargridupscaling::MyObserver: public peano4::grid::TraversalObserver {
  private:
    static tarch::logging::Log _log;

    const int    _spacetreeId;
    const double _h;
    const int    _flopsPerCell;

    /**
     * I use an accumulator just to ensure that the flop calculations are not removed.
     */
    double _accumulator;

    double _taskAccumulator;

  public:
    static constexpr int RanksObserverTemplate = 1;

    static double FractionOfCellsYieldingIntegrationTask;
    static int    IntegrationAccuracy;

    MyObserver(int spacetreeId, double h, int flopsPerCell);
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
