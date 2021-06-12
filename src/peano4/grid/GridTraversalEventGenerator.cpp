#include "GridTraversalEventGenerator.h"
#include "grid.h"


tarch::logging::Log  peano4::grid::GridTraversalEventGenerator::_log( "peano4::grid::GridTraversalEventGenerator" );


peano4::grid::GridTraversalEventGenerator::GridTraversalEventGenerator(int id):
  _id(id) {
}
