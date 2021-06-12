#include "GridTraversalEventGenerator.h"
#include "grid.h"


tarch::logging::Log  peano4::grid::GridTraversalEventGenerator::_log( "peano4::grid::GridTraversalEventGenerator" );


peano4::grid::GridTraversalEventGenerator::GridTraversalEventGenerator(int id):
  _id(id) {
}


std::bitset<TwoPowerD> peano4::grid::GridTraversalEventGenerator::areVerticesLocal(
  GridVertex  vertices[TwoPowerD],
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set< int >&                    joinTriggered,
  const std::set< int >&                    joining
  ) const {
  std::bitset<TwoPowerD> bitset;
  for (int i=0; i<TwoPowerD; i++) {
    bitset.set(i,isVertexAdjacentToLocalSpacetree(vertices[i], splitTriggered, splitting, joinTriggered, joining, true, true));
  }
  return bitset;
}



bool peano4::grid::GridTraversalEventGenerator::isVertexAdjacentToLocalSpacetree(
  GridVertex                 vertex,
  const SplitSpecification&  splitTriggered,
  const std::set<int>&       splitting,
  const std::set< int >&     joinTriggered,
  const std::set< int >&     joining,
  bool        splittingIsConsideredLocal,
  bool        joiningIsConsideredLocal
) const {
  if (vertex.getState()==GridVertex::State::HangingVertex) {
    return false;
  }
  else {
    logTraceInWith3Arguments( "isVertexAdjacentToLocalSpacetree(...)", vertex.toString(), splittingIsConsideredLocal, joiningIsConsideredLocal );
    bool result = false;
    for(int i=0; i<TwoPowerD; i++) {
      assertion( splitTriggered.count( vertex.getAdjacentRanks(i) )<=1 );
      assertion( splitting.count( vertex.getAdjacentRanks(i) )<=1 );

      result |= vertex.getAdjacentRanks(i)==_id;

      result |= splitTriggered.count( vertex.getAdjacentRanks(i) )==1;
      result |= (splittingIsConsideredLocal and splitting.count( vertex.getAdjacentRanks(i) )==1);

      result |= (joiningIsConsideredLocal and joining.count( vertex.getAdjacentRanks(i) )==1);
    }
    logTraceOutWith1Argument( "isVertexAdjacentToLocalSpacetree(...)", result );
    return result;
  }
}


