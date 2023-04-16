#include "MyObserver.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/parallel/Node.h"

#include "tarch/logging/LogFilter.h"



int examples::grid::MyObserver::_iterationCounter(0);


examples::grid::MyObserver::MyObserver(int spacetreeId, int counter)
  #if PeanoDebug>0
  : TraversalVTKPlotter( "grid-construction", spacetreeId )
  #endif
  {
  if (spacetreeId==0) {
	_iterationCounter++;
  }
}


examples::grid::MyObserver::~MyObserver() {
}


void examples::grid::MyObserver::beginTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  #if PeanoDebug>0
  TraversalVTKPlotter::beginTraversal(x,h);
  #endif
}


void examples::grid::MyObserver::endTraversal(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  #if PeanoDebug>0
  TraversalVTKPlotter::endTraversal(x,h);
  #endif
}


void examples::grid::MyObserver::enterCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  #if PeanoDebug>0
  TraversalVTKPlotter::enterCell(event);
  #endif
}


void examples::grid::MyObserver::leaveCell(
  const peano4::grid::GridTraversalEvent&  event
) {
  #if PeanoDebug>0
  TraversalVTKPlotter::leaveCell(event);
  #endif
}


peano4::grid::TraversalObserver* examples::grid::MyObserver::clone(int spacetreeId) {
  #if PeanoDebug>0
  MyObserver* result = new MyObserver( spacetreeId, _iterationCounter );

  if (_spacetreeId!=-1) {
    assertionMsg( false, "clone() should not be called for particular spacetree plotter" );
  }
  else {
    updateMetaFile(spacetreeId);
  }

  return result;
  #else
  return new MyObserver( spacetreeId, _iterationCounter );
  #endif
}


std::vector< peano4::grid::GridControlEvent > examples::grid::MyObserver::getGridControlEvents() {
  std::vector< peano4::grid::GridControlEvent >  controlEvents;

  if (_iterationCounter<GridRefinementIterations) {
    peano4::grid::GridControlEvent newEvent;
    newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
#if Dimensions==2
    newEvent.setOffset( {0.0,0.0} );
    newEvent.setWidth( {0.5,0.5} );
    newEvent.setH( {0.02,0.02} );
#elif Dimensions==3
    newEvent.setOffset( {0.0,0.0,0.0} );
    newEvent.setWidth( {0.5,0.5,0.5} );
    newEvent.setH( {0.02,0.02,0.02} );
#endif
    controlEvents.push_back(newEvent);
  }

  if (_iterationCounter>GridRefinementIterations + StationaryIterations) {
    peano4::grid::GridControlEvent newEvent;
	newEvent.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Erase );
#if Dimensions==2
	newEvent.setOffset( {0.01,0.01} );
	newEvent.setWidth( {0.99,0.99} );
	newEvent.setH( {0.4,0.4} );
#elif Dimensions==3
	newEvent.setOffset( {0.01,0.01,0.01} );
	newEvent.setWidth( {0.99,0.99,0.99} );
	newEvent.setH( {0.4,0.4,0.4} );
#endif
    controlEvents.push_back(newEvent);
  }

  return controlEvents;
}
