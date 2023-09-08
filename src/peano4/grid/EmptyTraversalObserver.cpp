#include "EmptyTraversalObserver.h"
#include "GridControlEvent.h"
#include "GridTraversalEvent.h"

tarch::logging::Log  peano4::grid::EmptyTraversalObserver::_log( "peano4::grid::EmptyTraversalObserver" );

void peano4::grid::EmptyTraversalObserver::beginTraversal(
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  h
) {
  logInfo( "beginTraversal(...)", "begin traversal" );
}

void peano4::grid::EmptyTraversalObserver::endTraversal(
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  x,
  [[maybe_unused]] const tarch::la::Vector<Dimensions,double>&  h
) {
  logInfo( "endTraversal(...)", "end traversal" );
}

void peano4::grid::EmptyTraversalObserver::loadCell(
  [[maybe_unused]] const GridTraversalEvent&  event
) {
  logTraceInWith1Argument( "loadCell(...)", event.toString() );
  logTraceOut( "loadCell(...)" );
}

void peano4::grid::EmptyTraversalObserver::enterCell(
  [[maybe_unused]] const GridTraversalEvent&  event
) {
  logTraceInWith1Argument( "enterCell(...)", event.toString() );
  logTraceOut( "enterCell(...)" );
}

void peano4::grid::EmptyTraversalObserver::leaveCell(
  [[maybe_unused]] const GridTraversalEvent&  event
) {
  logTraceInWith1Argument( "leaveCell(...)", event.toString() );
  logTraceOut( "leaveCell(...)" );
}

void peano4::grid::EmptyTraversalObserver::storeCell(
  [[maybe_unused]] const GridTraversalEvent&  event
) {
  logTraceInWith1Argument( "storeCell(...)", event.toString() );
  logTraceOut( "storeCell(...)" );
}

peano4::grid::TraversalObserver* peano4::grid::EmptyTraversalObserver::clone([[maybe_unused]] int spacetreeId) {
  return new EmptyTraversalObserver();
}

std::vector< peano4::grid::GridControlEvent > peano4::grid::EmptyTraversalObserver::getGridControlEvents() const {
  return std::vector< peano4::grid::GridControlEvent >();
}
