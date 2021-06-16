#include "grid.h"
#include "GridStatistics.h"
#include "GridControlEvent.h"
#include "Spacetree.h"
#include "AutomatonState.h"

#include "peano4/utils/Loop.h"



bool peano4::grid::overlaps( const peano4::grid::AutomatonState& state, const peano4::grid::GridControlEvent& event ) {
  return tarch::la::allGreaterEquals( state.getX(), event.getOffset() )
    and
    tarch::la::allSmallerEquals( state.getX() + state.getH(), event.getOffset()+event.getWidth() );
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
  result.setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(not isNewFineGridVertex);
  result.setIsAntecessorOfRefinedVertexInCurrentTreeSweep(not isNewFineGridVertex);
  result.setX(x);
  result.setLevel(level);

  // not required logically, but makes valgrind's memchecker happy
  result.setBackupOfAdjacentRanks( tarch::la::Vector<TwoPowerD,int>(InvalidRank) );
  result.setNumberOfAdjacentRefinedLocalCells(0);

  return result;
}


std::vector< peano4::grid::GridControlEvent > peano4::grid::merge( std::vector< GridControlEvent>   events, const double Tolerance ) {
  static tarch::logging::Log _log( "peano4::grid" );
  logTraceInWith1Argument( "merge(...)", events.size() );

  std::vector< peano4::grid::GridControlEvent > result;

  for ( auto currentEvent: events ) {
    std::vector< GridControlEvent >::iterator i = result.begin();

    bool hasInserted = false;

    while (i!=result.end()) {
      // fuse two adjacent refinement commands
      tarch::la::Vector<Dimensions,double> boundingEventOffset = tarch::la::min( i->getOffset(), currentEvent.getOffset() );
      tarch::la::Vector<Dimensions,double> boundingEventSize   = tarch::la::max( i->getOffset()+i->getWidth(), currentEvent.getOffset()+currentEvent.getWidth() ) - boundingEventOffset;

      logDebug( "merge(...)", "compare " << currentEvent.toString() << "+" << i->toString() << " vs. a fused event of " << boundingEventOffset << "x" << boundingEventSize );

      bool twoEventsAreAdjacent = tarch::la::volume(boundingEventSize) <= (1.0+Tolerance) * (tarch::la::volume(i->getWidth()) + tarch::la::volume(currentEvent.getWidth()));

      auto refinementEventOverrulesCoarsening = [&]( const auto& refineEvent, const auto& eraseEvent ) -> bool {
        bool twoEventsOverlap     = true;
        for (int d=0; d<Dimensions; d++) {
          twoEventsOverlap &= refineEvent.getOffset()(d)+refineEvent.getWidth()(d) >= eraseEvent.getOffset()(d);
          twoEventsOverlap &= refineEvent.getOffset()(d)                           <= eraseEvent.getOffset()(d) + eraseEvent.getWidth()(d);
        }
        return twoEventsOverlap
           and refineEvent.getRefinementControl()==GridControlEvent::RefinementControl::Refine
           and eraseEvent.getRefinementControl()==GridControlEvent::RefinementControl::Erase
           and tarch::la::oneSmallerEquals( 1.0/3.0*refineEvent.getH(), eraseEvent.getH() );
      };

      if (
        refinementEventOverrulesCoarsening(*i,currentEvent)
      ) {
        logInfo( "merge(...)", "drop event " << currentEvent.toString() );
        i = result.end();
        hasInserted = true;
      }
      else if (
        refinementEventOverrulesCoarsening(currentEvent,*i)
      ) {
        logInfo( "merge(...)", "replace event " << i->toString() << " with " << currentEvent.toString() );
        *i = currentEvent;
        i = result.end();
        hasInserted = true;
      }
      else if (
        (i->getRefinementControl()==GridControlEvent::RefinementControl::Refine and currentEvent.getRefinementControl()==GridControlEvent::RefinementControl::Refine)
        and
        tarch::la::equals( i->getH(), currentEvent.getH(), tarch::la::relativeEps( tarch::la::max(i->getH()), tarch::la::max(currentEvent.getH()), 0.1 ))
        and
        twoEventsAreAdjacent
      ) {
        *i = GridControlEvent(
          GridControlEvent::RefinementControl::Refine,
          boundingEventOffset, boundingEventSize, i->getH()
        );
        logDebug( "merge(...)", "merged into " << currentEvent.toString() );
        hasInserted = true;
        i           = result.end();
      }
      else if (
        (i->getRefinementControl()==GridControlEvent::RefinementControl::Erase and currentEvent.getRefinementControl()==GridControlEvent::RefinementControl::Erase)
        and
        tarch::la::equals( i->getH(), currentEvent.getH(), tarch::la::relativeEps( tarch::la::max(i->getH()), tarch::la::max(currentEvent.getH()), 0.1 ))
        and
        twoEventsAreAdjacent
      ) {
        *i = GridControlEvent(
          GridControlEvent::RefinementControl::Erase,
          boundingEventOffset, boundingEventSize, i->getH()
        );
        logDebug( "merge(...)", "merged into " << currentEvent.toString() );
        hasInserted = true;
        i           = result.end();
      }
      else {
        i++;
      }
    }

    if (not hasInserted ) {
      result.push_back( currentEvent );
    }
  }

  logTraceOutWith1Argument( "merge(...)", events.size() );
  return events.size()>result.size() ? merge(result, Tolerance) : result;
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
    result |= isVertexRefined( vertices[kScalar] );
  enddforx
  return result;
}


bool peano4::grid::isVertexRefined(GridVertex  vertex) {
  return vertex.getState() == GridVertex::State::Refining
      or vertex.getState() == GridVertex::State::Refined
      or vertex.getState() == GridVertex::State::EraseTriggered
      or vertex.getState() == GridVertex::State::Erasing;
}


std::bitset<TwoPowerD> peano4::grid::areVerticesRefined(GridVertex  vertices[TwoPowerD]) {
  std::bitset<TwoPowerD> bitset;
  for (int i=0; i<TwoPowerD; i++) {
     assertion( not isVertexRefined(vertices[i]) or vertices[i].getState()!=GridVertex::State::HangingVertex );
     bitset.set(i,isVertexRefined(vertices[i]));
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
