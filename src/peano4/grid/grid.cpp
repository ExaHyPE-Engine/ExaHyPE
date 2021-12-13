#include "grid.h"
#include "GridStatistics.h"
#include "GridControlEvent.h"
#include "Spacetree.h"
#include "AutomatonState.h"

#include "peano4/utils/Loop.h"



bool peano4::grid::isContained( const peano4::grid::AutomatonState& state, const peano4::grid::GridControlEvent& event, double upscaleAutomatonState ) {
  assertion1(upscaleAutomatonState>=1.0,upscaleAutomatonState);
  return tarch::la::allGreaterEquals( state.getX() + (0.5-0.5*upscaleAutomatonState)*state.getH(), event.getOffset() )
    and
    tarch::la::allSmallerEquals( state.getX() + (0.5+0.5*upscaleAutomatonState)*state.getH(), event.getOffset()+event.getWidth() );
}


bool peano4::grid::overlaps( const peano4::grid::AutomatonState& state, const peano4::grid::GridControlEvent& event ) {
  return tarch::la::allGreaterEquals( state.getX()+ state.getH(), event.getOffset() )
    and
    tarch::la::allSmallerEquals( state.getX(), event.getOffset()+event.getWidth() );
}


bool peano4::grid::overlaps( const tarch::la::Vector<Dimensions,double>& x, const GridControlEvent& event ) {
  return tarch::la::allGreaterEquals( x, event.getOffset() )
    and
    tarch::la::allSmallerEquals( x, event.getOffset()+event.getWidth() );
}


peano4::grid::GridVertex peano4::grid::createVertex(
  GridVertex::State                            state,
  const tarch::la::Vector<Dimensions,double>&  x,
  int                                          level,
  const tarch::la::Vector<TwoPowerD,int>&      adjacentRanks,
  bool                                         isNewFineGridVertex
) {
  GridVertex result;

  result.setState( state );
  result.setAdjacentRanks( adjacentRanks );
  result.setBackupOfAdjacentRanks( adjacentRanks );
  result.setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(not isNewFineGridVertex);
  result.setIsAntecessorOfRefinedVertexInCurrentTreeSweep(not isNewFineGridVertex);
  result.setX(x);
  result.setLevel(level);

  // not required logically, but makes valgrind's memchecker happy
  result.setNumberOfAdjacentRefinedLocalCells(0);

  // @todo raus
  //result.setBackupOfAdjacentRanks( tarch::la::Vector<TwoPowerD,int>(InvalidRank) );

  return result;
}


std::vector< peano4::grid::GridControlEvent > peano4::grid::merge( std::vector< GridControlEvent >   events, const double Tolerance ) {
  static tarch::logging::Log _log( "peano4::grid" );
  logTraceInWith1Argument( "merge(...)", events.size() );

  std::list< peano4::grid::GridControlEvent > inputEvents;
  inputEvents.insert( inputEvents.begin(), events.begin(), events.end() );

  logDebug( "merge(...)", "have a total of " << inputEvents.size() << " event(s)");
  internal::removeEraseEventsThatAreCancelledByRefineEvents( inputEvents );
  logDebug( "merge(...)", "reduced them to a total of " << inputEvents.size() << " event(s) by eliminating erase events that overlap with refinement");

  std::list< peano4::grid::GridControlEvent > eraseEvents;
  std::list< peano4::grid::GridControlEvent >::iterator inputP = inputEvents.begin();
  while ( inputP!=inputEvents.end() ) {
    if (inputP->getRefinementControl()==GridControlEvent::RefinementControl::Erase) {
      eraseEvents.push_back(*inputP);
      inputP = inputEvents.erase(inputP);
    }
    else inputP++;
  }

  std::list< peano4::grid::GridControlEvent >::iterator eraseP = eraseEvents.begin();
  logDebug( "merge(...)", "extracted " << eraseEvents.size() << " erase event(s), while I have " << inputEvents.size() << " refinement event(s) left");

  internal::mergeAdjacentRefinementEvents( inputEvents );
  logDebug( "merge(...)", "reduced remaining input set to " << inputEvents.size() << " event(s) by merging refinement events");

  int oldSetSize = -1;
  while (oldSetSize!=eraseEvents.size()) {
    oldSetSize = eraseEvents.size();
    std::list< peano4::grid::GridControlEvent > additionalEraseEvents;

    std::list< peano4::grid::GridControlEvent >::iterator eraseP  = eraseEvents.begin();
    std::list< peano4::grid::GridControlEvent >::iterator erasePP = eraseEvents.begin();
    while (eraseP!=eraseEvents.end()) {
      while (erasePP!=eraseEvents.end()) {
        assertion(eraseP->getRefinementControl()==GridControlEvent::RefinementControl::Erase);
        assertion(erasePP->getRefinementControl()==GridControlEvent::RefinementControl::Erase);

        if ( eraseP!=erasePP and internal::twoEventsAreAdjacent(*eraseP,*erasePP) ) {
          GridControlEvent newEvent = internal::createBoundingBoxEvent(*eraseP,*erasePP);
          // if we push back, we would (a) compare again against P which is inefficient
          // and and (b) maybe even enter a infinite iteration if we insert
          // eraseEvents.push_back( newEvent );

          bool alreadyInSet = false;
          for (auto& pp: eraseEvents) {
            alreadyInSet |= internal::equals( newEvent, pp );
          }
          for (auto& pp: additionalEraseEvents) {
            alreadyInSet |= internal::equals( newEvent, pp );
          }

          if (not alreadyInSet) {
            additionalEraseEvents.push_back( newEvent );
            logDebug( "merge(...)",
              "added a new, merged erase event to set: " << newEvent.toString() <<
              " (#erase events=" << eraseEvents.size() <<
              ", #input events=" << inputEvents.size() <<
              ", #additional events=" << additionalEraseEvents.size() << ")"
            );
          }
        }

        erasePP++;
      }
      eraseP++;
    }

    logDebug( "merge(...)", "add " << additionalEraseEvents.size() << " new erase events" );
    eraseEvents.insert(eraseEvents.begin(),additionalEraseEvents.begin(),additionalEraseEvents.end());
  }

  logDebug( "merge(...)", "add back the " << eraseEvents.size() << " potential identified erase to the input events" );
  inputEvents.insert( inputEvents.begin(), eraseEvents.begin(), eraseEvents.end() );

  internal::removeEraseEventsCoverredByOtherEraseEvents( eraseEvents );
  logDebug( "merge(...)", "reduced to " << inputEvents.size() << " event(s) by eliminating duplicate erase events");

  std::vector< peano4::grid::GridControlEvent > result;
  result.insert( result.begin(), inputEvents.begin(), inputEvents.end() );
  logTraceOutWith1Argument( "merge(...)", result.size() );
  return result;
}


void peano4::grid::internal::removeEraseEventsThatAreCancelledByRefineEvents( std::list< peano4::grid::GridControlEvent >&  inputEvents ) {
  static tarch::logging::Log _log( "peano4::grid" );
  bool hasMergedRefinementEventOrCancelledErase = true;
  while (hasMergedRefinementEventOrCancelledErase) {
    std::list< peano4::grid::GridControlEvent >::iterator inputP = inputEvents.begin();
    hasMergedRefinementEventOrCancelledErase = false;
    while (inputP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
    //while (inputP!=inputEvents.end()) {
      std::list< GridControlEvent >::iterator inputPP = inputEvents.begin();
      while (inputPP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
      //while (inputP!=inputEvents.end() and inputPP!=inputEvents.end()) {
        logDebug( "removeEraseEventsThatAreCancelledByRefineEvents(...)", "compare " << inputP->toString() << " vs " << inputPP->toString()  );

        if ( inputP!=inputPP and refinementEventOverrulesCoarsening(*inputP,*inputPP) ) {
          logDebug( "removeEraseEventsThatAreCancelledByRefineEvents(...)", "drop event " << inputPP->toString() );
          inputPP = inputEvents.erase(inputPP);
          hasMergedRefinementEventOrCancelledErase = true;
        }
        else if ( inputP!=inputPP and refinementEventOverrulesCoarsening(*inputPP,*inputP) ) {
          logDebug( "removeEraseEventsThatAreCancelledByRefineEvents(...)", "drop event " << inputP->toString() );
          inputP = inputEvents.erase(inputP);
          hasMergedRefinementEventOrCancelledErase = true;
        }
        else {
          inputPP++;
        }
      }
      if ( not hasMergedRefinementEventOrCancelledErase ) {
        inputP++;
      }
    }
  }
}


void peano4::grid::internal::removeEraseEventsCoverredByOtherEraseEvents( std::list< peano4::grid::GridControlEvent >&  inputEvents ) {
  static tarch::logging::Log _log( "peano4::grid" );
  bool hasMergedRefinementEventOrCancelledErase = true;
  while (hasMergedRefinementEventOrCancelledErase) {
    std::list< peano4::grid::GridControlEvent >::iterator inputP = inputEvents.begin();
    hasMergedRefinementEventOrCancelledErase = false;
    while (inputP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
    //while (inputP!=inputEvents.end()) {
      std::list< GridControlEvent >::iterator inputPP = inputEvents.begin();
      while (inputPP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
      //while (inputP!=inputEvents.end() and inputPP!=inputEvents.end()) {
        logDebug( "removeEraseEventsCoverredByOtherEraseEvents(...)", "compare " << inputP->toString() << " vs " << inputPP->toString()  );

        if (
          inputP!=inputPP
          and
          eraseEventCoveredByOtherEraseEvent(*inputP,*inputPP)
        ) {
          logDebug( "removeEraseEventsCoverredByOtherEraseEvents(...)", "throw away " << inputP->toString() << " as covered by " << inputPP->toString() );
          inputP  = inputEvents.erase(inputP);
          hasMergedRefinementEventOrCancelledErase = true;
        }
        else {
          inputPP++;
        }
      }

      if ( not hasMergedRefinementEventOrCancelledErase ) {
        inputP++;
      }
    }
  }
}


void peano4::grid::internal::mergeAdjacentRefinementEvents( std::list< peano4::grid::GridControlEvent >&  inputEvents ) {
  static tarch::logging::Log _log( "peano4::grid" );
  bool hasMergedRefinementEventOrCancelledErase = true;
  while (hasMergedRefinementEventOrCancelledErase) {
    std::list< peano4::grid::GridControlEvent >::iterator inputP = inputEvents.begin();
    hasMergedRefinementEventOrCancelledErase = false;
    while (inputP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
    //while (inputP!=inputEvents.end()) {
      std::list< GridControlEvent >::iterator inputPP = inputEvents.begin();
      while (inputPP!=inputEvents.end() and not hasMergedRefinementEventOrCancelledErase) {
      //while (inputP!=inputEvents.end() and inputPP!=inputEvents.end()) {
        logDebug( "mergeAdjacentRefinementEvents(...)", "compare " << inputP->toString() << " vs " << inputPP->toString()  );

        if (
          inputP!=inputPP
          and
          inputP->getRefinementControl()==GridControlEvent::RefinementControl::Refine
          and
          inputPP->getRefinementControl()==GridControlEvent::RefinementControl::Refine
          and
          twoEventsAreAdjacent(*inputP,*inputPP)
        ) {
          GridControlEvent newEvent = createBoundingBoxEvent(*inputP,*inputPP);
          logDebug( "mergeAdjacentRefinementEvents(...)", "merge two adjacent events " << inputP->toString() << " and " << inputPP->toString() << ") into " << newEvent.toString() );
          inputPP = inputEvents.erase(inputPP);
          inputP  = inputEvents.erase(inputP);
          inputEvents.push_back(newEvent);
          hasMergedRefinementEventOrCancelledErase = true;
        }
        else {
          inputPP++;
        }
      }

      if ( not hasMergedRefinementEventOrCancelledErase ) {
        inputP++;
      }
    }
  }
}


peano4::grid::GridStatistics operator+( peano4::grid::GridStatistics lhs, peano4::grid::GridStatistics rhs ) {
  return peano4::grid::GridStatistics(
    lhs.getNumberOfLocalUnrefinedCells()  + rhs.getNumberOfLocalUnrefinedCells(),
    lhs.getNumberOfRemoteUnrefinedCells() + rhs.getNumberOfRemoteUnrefinedCells(),
    lhs.getNumberOfLocalRefinedCells()  + rhs.getNumberOfLocalRefinedCells(),
    lhs.getNumberOfRemoteRefinedCells() + rhs.getNumberOfRemoteRefinedCells(),
    std::min(lhs.getStationarySweeps(),rhs.getStationarySweeps()),
    lhs.getCoarseningHasBeenVetoed() | rhs.getCoarseningHasBeenVetoed(),
    lhs.getRemovedEmptySubtree() | rhs.getRemovedEmptySubtree(),
    tarch::la::min( lhs.getMinH(), rhs.getMinH() )
  );
}


void peano4::grid::clear( GridStatistics& statistics, bool isGlobalMasterTree ) {
  statistics.setNumberOfLocalUnrefinedCells(0);
  statistics.setNumberOfRemoteUnrefinedCells(0);
  statistics.setNumberOfLocalRefinedCells(isGlobalMasterTree ? 1 : 0);
  statistics.setNumberOfRemoteRefinedCells(isGlobalMasterTree ? 0 : 1);
  statistics.setCoarseningHasBeenVetoed(false);
  statistics.setRemovedEmptySubtree(false);
  statistics.setStationarySweeps( statistics.getStationarySweeps()+1 );
  statistics.setMinH( tarch::la::Vector<Dimensions,double>( std::numeric_limits<double>::max() ) );
}


bool peano4::grid::isSpacetreeNodeRefined(GridVertex  vertices[TwoPowerD]) {
  bool result = false;
  dfor2(k)
    result |= willVertexBeRefined( vertices[kScalar] );
    result |= hasVertexBeenRefined( vertices[kScalar] );
  enddforx
  return result;
}


bool peano4::grid::willVertexBeRefined(const GridVertex&  vertex) {
  return vertex.getState() == GridVertex::State::Refining
      or vertex.getState() == GridVertex::State::Refined
      or vertex.getState() == GridVertex::State::EraseTriggered;
}


bool peano4::grid::hasVertexBeenRefined(const GridVertex&  vertex) {
  return vertex.getState() == GridVertex::State::Refined
      or vertex.getState() == GridVertex::State::EraseTriggered
      or vertex.getState() == GridVertex::State::Erasing;
}


std::bitset<TwoPowerD> peano4::grid::willVerticesBeRefined(GridVertex  vertices[TwoPowerD]) {
  std::bitset<TwoPowerD> bitset;
  for (int i=0; i<TwoPowerD; i++) {
     assertion( not willVertexBeRefined(vertices[i]) or vertices[i].getState()!=GridVertex::State::HangingVertex );
     bitset.set(i,willVertexBeRefined(vertices[i]));
  }
  return bitset;
}


std::bitset<TwoPowerD> peano4::grid::haveVerticesBeenRefined(GridVertex  vertices[TwoPowerD]) {
  std::bitset<TwoPowerD> bitset;
  for (int i=0; i<TwoPowerD; i++) {
     assertion( not hasVertexBeenRefined(vertices[i]) or vertices[i].getState()!=GridVertex::State::HangingVertex );
     bitset.set(i,hasVertexBeenRefined(vertices[i]));
  }
  return bitset;
}


std::string peano4::grid::toString( VertexType type ) {
  switch (type) {
    case VertexType::New:
      return "new";
    case VertexType::Hanging:
      return "hanging";
    case VertexType::Persistent:
      return "persistent";
    case VertexType::Delete:
      return "delete";
  }
  return "<undef>";
}


std::string peano4::grid::toString( FaceType type ) {
  switch (type) {
    case FaceType::New:
      return "new";
    case FaceType::Hanging:
      return "hanging";
    case FaceType::Persistent:
      return "persistent";
    case FaceType::Delete:
      return "delete";
  }
  return "<undef>";
}


std::string peano4::grid::toString( CellType type ) {
  switch (type) {
    case CellType::New:
      return "new";
    case CellType::Persistent:
      return "persistent";
    case CellType::Delete:
      return "delete";
  }
  return "<undef>";
}


std::string peano4::grid::toString( SpacetreeState state ) {
  switch (state) {
    case SpacetreeState::EmptyRun:
      return "empty-run";
    case SpacetreeState::NewRoot:
      return "new-root";
    case SpacetreeState::NewFromSplit:
      return "new-from-split";
    case SpacetreeState::Running:
      return "running";
    case SpacetreeState::JoinTriggered:
      return "join-triggered";
    case SpacetreeState::Joining:
      return "joining";
    case SpacetreeState::Joined:
      return "joined";
  }
  return "<undef>";
}


bool peano4::grid::internal::twoEventsOverlap( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs ) {
  bool twoEventsOverlap     = true;
  for (int d=0; d<Dimensions; d++) {
    twoEventsOverlap &= lhs.getOffset()(d)+lhs.getWidth()(d) >= rhs.getOffset()(d);
    twoEventsOverlap &= lhs.getOffset()(d)                   <= rhs.getOffset()(d) + rhs.getWidth()(d);
  }
  return twoEventsOverlap;
};


bool peano4::grid::internal::equals( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs ) {
  bool sameType =
    (lhs.getRefinementControl()==GridControlEvent::RefinementControl::Erase and rhs.getRefinementControl()==GridControlEvent::RefinementControl::Erase )
    or
    (lhs.getRefinementControl()==GridControlEvent::RefinementControl::Refine and rhs.getRefinementControl()==GridControlEvent::RefinementControl::Refine );

  return sameType
     and tarch::la::equals( lhs.getOffset(), rhs.getOffset() )
     and tarch::la::equals( lhs.getWidth(), rhs.getWidth() )
     and tarch::la::equals( lhs.getH(), rhs.getH() );
};


bool peano4::grid::internal::eraseEventCoveredByOtherEraseEvent( const peano4::grid::GridControlEvent& smaller, const peano4::grid::GridControlEvent& bigger ) {
  bool contained     = true;
  for (int d=0; d<Dimensions; d++) {
    contained &= smaller.getOffset()(d)                         >= bigger.getOffset()(d);
    contained &= smaller.getOffset()(d) + smaller.getWidth()(d) <= bigger.getOffset()(d) + bigger.getWidth()(d);
  }
  return contained
     and smaller.getRefinementControl()==GridControlEvent::RefinementControl::Erase
     and bigger.getRefinementControl()==GridControlEvent::RefinementControl::Erase
     and tarch::la::equals( smaller.getH(), bigger.getH() );
};


bool peano4::grid::internal::refinementEventOverrulesCoarsening( const peano4::grid::GridControlEvent& refineEvent, const peano4::grid::GridControlEvent& eraseEvent ) {
  return twoEventsOverlap(refineEvent, eraseEvent)
     and refineEvent.getRefinementControl()==GridControlEvent::RefinementControl::Refine
     and eraseEvent.getRefinementControl()==GridControlEvent::RefinementControl::Erase
     and tarch::la::oneSmallerEquals( 1.0/3.0*refineEvent.getH(), eraseEvent.getH() );
};


bool peano4::grid::internal::twoEventsAreAdjacent( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs ) {
  tarch::la::Vector<Dimensions,double> boundingEventOffset = tarch::la::min( lhs.getOffset(), rhs.getOffset() );
  tarch::la::Vector<Dimensions,double> boundingEventSize   = tarch::la::max( lhs.getOffset()+lhs.getWidth(), rhs.getOffset()+rhs.getWidth() ) - boundingEventOffset;

  return tarch::la::equals( lhs.getH(), rhs.getH() )
     and tarch::la::volume(boundingEventSize) <= tarch::la::volume(lhs.getWidth()) + tarch::la::volume(rhs.getWidth());
};


peano4::grid::GridControlEvent peano4::grid::internal::createBoundingBoxEvent( const peano4::grid::GridControlEvent& lhs, const peano4::grid::GridControlEvent& rhs ) {
  tarch::la::Vector<Dimensions,double> boundingEventOffset = tarch::la::min( lhs.getOffset(), rhs.getOffset() );
  tarch::la::Vector<Dimensions,double> boundingEventSize   = tarch::la::max( lhs.getOffset()+lhs.getWidth(), rhs.getOffset()+rhs.getWidth() ) - boundingEventOffset;
  return GridControlEvent( rhs.getRefinementControl(), boundingEventOffset, boundingEventSize, rhs.getH() );
};


void peano4::grid::reduceGridControlEvents( std::vector<GridControlEvent>& events ) {
  #ifdef Parallel
/*
  std::vector<GridControlEvent> copyOfEvents = events;

  int localSize  = events.size();
  int globalSize;

  MPI_Allreduce(&localSize, globalSize, 1, MPI_INT, MPI_SUM, tarch::mpi::Rank::getInstance().getCommunicator());

  logInfo( "")
*/
  #endif
}
