#include "GridTraversalEventGenerator.h"
#include "grid.h"

#include "peano4/utils/Loop.h"


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


bool peano4::grid::GridTraversalEventGenerator::isSpacetreeNodeLocal(
  GridVertex    vertices[TwoPowerD],
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set< int >&                    joinTriggered,
  const std::set< int >&                    joining,
  bool          splittingIsConsideredLocal,
  bool          joiningIsConsideredLocal
) const {
  bool isLocal = true;
  dfor2(k)
    isLocal &= (
      (vertices[kScalar].getState()==GridVertex::State::HangingVertex)
      or
      (
        vertices[kScalar].getAdjacentRanks(TwoPowerD-kScalar-1)==_id
      )
      or
      ( splitTriggered.count(vertices[kScalar].getAdjacentRanks(TwoPowerD-kScalar-1)) > 0)
      or
      (
        splittingIsConsideredLocal and splitting.count(vertices[kScalar].getAdjacentRanks(TwoPowerD-kScalar-1))>0
      )
      or
      (
        joiningIsConsideredLocal and joining.count(vertices[kScalar].getAdjacentRanks(TwoPowerD-kScalar-1))>0
      )
    );
  enddforx

  return isLocal;
}


std::bitset<TwoTimesD> peano4::grid::GridTraversalEventGenerator::areFacesLocal(
  GridVertex  vertices[TwoPowerD],
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set< int >&                    joinTriggered,
  const std::set< int >&                    joining
) const {
  std::bitset<TwoTimesD> result;
  for (int faceNumber=0; faceNumber<2*Dimensions; faceNumber++) {
    bool isLocal = false;

    const int normal = faceNumber % Dimensions;
    for (int i=0; i<TwoPowerD; i++) {
      std::bitset<Dimensions> studiedVertex = i;
      studiedVertex.set(normal,faceNumber>=Dimensions);
      std::bitset<Dimensions> studiedEntry  = TwoPowerD - studiedVertex.to_ulong() - 1;

      studiedEntry.set(normal,0);
      int currentRank = vertices[studiedVertex.to_ulong()].getAdjacentRanks( studiedEntry.to_ulong() );
      isLocal |=  currentRank == _id;
      isLocal |=  splitTriggered.count(currentRank)>0;
      isLocal |=  splitting.count(currentRank)>0;

      studiedEntry.set(normal,1);
      currentRank = vertices[studiedVertex.to_ulong()].getAdjacentRanks( studiedEntry.to_ulong() );
      isLocal |= currentRank == _id;
      isLocal |=  splitTriggered.count(currentRank)>0;
      isLocal |=  splitting.count(currentRank)>0;
    }

    result[faceNumber] = isLocal;
  }
  return result;
}


std::bitset<TwoPowerD> peano4::grid::GridTraversalEventGenerator::areVerticesInsideDomain(GridVertex  vertices[TwoPowerD]) const {
  std::bitset<TwoPowerD> bitset;
  for (int i=0; i<TwoPowerD; i++) {
    bitset.set(i,
      tarch::la::equals( vertices[i].getBackupOfAdjacentRanks(), _id )
    );
  }
  return bitset;
}


peano4::grid::GridTraversalEvent peano4::grid::GridTraversalEventGenerator::createGenericCellTraversalEvent(
  GridVertex              fineGridVertices[TwoPowerD],
  const AutomatonState&                        state,
  const SplitSpecification&                 splitTriggered,
  const std::set<int>&                      splitting,
  const std::set< int >&                    joinTriggered,
  const std::set< int >&                    joining,
  const tarch::la::Vector<Dimensions,int>&  relativePositionToFather,
  bool                                      spacetreeStateIsRunning
) const {
  logTraceInWith3Arguments( "createGenericCellTraversalEvent(...)", state.toString(), relativePositionToFather, _id );
  GridTraversalEvent  event;
  event.setX( state.getX() + state.getH()*0.5 );
  event.setH( state.getH() );

  event.setIsRefined( areVerticesRefined(fineGridVertices) );
  event.setRelativePositionToFather( relativePositionToFather );

  event.setIsCellLocal(   isSpacetreeNodeLocal( fineGridVertices, splitTriggered, splitting, joinTriggered, joining, true, true) );
  event.setIsFaceLocal(   areFacesLocal(        fineGridVertices, splitTriggered, splitting, joinTriggered, joining) );
  event.setIsVertexLocal( areVerticesLocal(     fineGridVertices, splitTriggered, splitting, joinTriggered, joining) );

  event.setIsVertexInsideDomain( areVerticesInsideDomain(fineGridVertices) );

  event.setInvokingSpacetree( _id );
  event.setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(
        spacetreeStateIsRunning and
        joinTriggered.empty() and
        joining.empty() and
        splitTriggered.empty() and
        splitting.empty()
  );

  logTraceOut( "createGenericCellTraversalEvent(...)" );
  return event;
}
