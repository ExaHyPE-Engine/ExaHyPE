#include "RefinementControl.h"

#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"
#include "peano4/grid/grid.h"


exahype2::RefinementCommand exahype2::getDefaultRefinementCommand() {
  return RefinementCommand::Erase;
}


exahype2::RefinementCommand operator&&( exahype2::RefinementCommand lhs, exahype2::RefinementCommand rhs) {
  if (
    lhs == exahype2::RefinementCommand::Refine or rhs == exahype2::RefinementCommand::Refine
  ) {
    return exahype2::RefinementCommand::Refine;
  }
  else if (
    lhs == exahype2::RefinementCommand::Keep or rhs == exahype2::RefinementCommand::Keep
  ) {
    return exahype2::RefinementCommand::Keep;
  }
  else {
    return exahype2::RefinementCommand::Erase;
  }
}


std::string toString( exahype2::RefinementCommand value ) {
  switch (value) {
    case exahype2::RefinementCommand::Refine:  return "refine";
    case exahype2::RefinementCommand::Keep:    return "keep";
    case exahype2::RefinementCommand::Erase:   return "erase";
  }
  return "<undef>";
}


tarch::logging::Log  exahype2::RefinementControl::_log( "exahype2::RefinementControl" );


int exahype2::RefinementControl::_reductionTag( tarch::mpi::Rank::reserveFreeTag("exahype2::RefinementControl") );


tarch::multicore::BooleanSemaphore  exahype2::RefinementControl::_semaphore;


exahype2::RefinementControl::RefinementControl(double tolerance):
  _Tolerance( tolerance ) {
  tarch::services::ServiceRepository::getInstance().addService( this, "exahype2::RefinementControl" );
}


exahype2::RefinementControl::~RefinementControl() {
  tarch::services::ServiceRepository::getInstance().removeService( this );
}


void exahype2::RefinementControl::shutdown() {
}


void exahype2::RefinementControl::receiveDanglingMessages() {
  #ifdef Parallel
  for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
    if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
      MPI_Status status;
      int        flag;
      MPI_Iprobe(rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), &flag, &status);
      if (flag) {
        int numberOfMessages;
        MPI_Get_count( &status, peano4::grid::GridControlEvent::getDatatype(), &numberOfMessages );
        logInfo( "finishStep()", "got " << numberOfMessages << " event(s) from rank " << rank );
        std::vector< peano4::grid::GridControlEvent > receiveBuffer;
        receiveBuffer.resize( numberOfMessages );
        MPI_Recv( receiveBuffer.data(), numberOfMessages, peano4::grid::GridControlEvent::getDatatype(), rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), MPI_STATUS_IGNORE );

        tarch::multicore::Lock lock( _semaphore );
        for (auto p: receiveBuffer) {
          _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(p, 2) );
        }
      }
    }
  }
  #endif
}


void exahype2::RefinementControl::clear() {
  tarch::multicore::Lock lock( _semaphore );
  logDebug( "clear()", "clear list of control events" );
  _newEvents.clear();
}


std::vector< peano4::grid::GridControlEvent >  exahype2::RefinementControl::getGridControlEvents() const {
  logDebug( "getGridControlEvents()", "return " << _committedEvents.size() << " grid control events" );
  return _committedEvents;
}


void exahype2::RefinementControl::addCommand(
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  exahype2::RefinementCommand                  command,
  int                                          lifetime
) {
  logTraceInWith3Arguments( "addCommand()", x, h, ::toString(command) );
  tarch::multicore::Lock lock( _semaphore );

  switch (command) {
    case ::exahype2::RefinementCommand::Refine:
      {
        tarch::la::Vector<Dimensions,double> expandedWidth   = (1.0+_Tolerance) * h;
        tarch::la::Vector<Dimensions,double> shift           = 0.5 * (expandedWidth - h);
        tarch::la::Vector<Dimensions,double> refinedMeshSize = 1.0/3.0 * h;

        peano4::grid::GridControlEvent newEvent(
          peano4::grid::GridControlEvent::RefinementControl::Refine,
          x-0.5 * h - shift,
          expandedWidth,
          refinedMeshSize
        );

        assertionNumericalEquals1( newEvent.getWidth(0), newEvent.getWidth(1), newEvent.toString() );
        assertionNumericalEquals1( newEvent.getH(0), newEvent.getH(1), newEvent.toString() );
        _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(newEvent, lifetime) );
        logDebug( "addCommend()", "added refinement for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)" );
      }
      break;
    case ::exahype2::RefinementCommand::Keep:
      break;
    case ::exahype2::RefinementCommand::Erase:
      {
        tarch::la::Vector<Dimensions,double> expandedWidth   = (1.0+_Tolerance) * h;
        tarch::la::Vector<Dimensions,double> shift           = 0.5 * (expandedWidth - h);
        tarch::la::Vector<Dimensions,double> refinedMeshSize = 3.1 * h;

        peano4::grid::GridControlEvent newEvent(
          peano4::grid::GridControlEvent::RefinementControl::Erase,
          x-0.5 * h - shift,
          expandedWidth,
          refinedMeshSize
        );

        assertionNumericalEquals1( newEvent.getWidth(0), newEvent.getWidth(1), newEvent.toString() );
        assertionNumericalEquals1( newEvent.getH(0), newEvent.getH(1), newEvent.toString() );
        _newEvents.push_back( std::pair<peano4::grid::GridControlEvent,int>(newEvent, lifetime) );
        logDebug( "addCommend()", "added erase for x=" << x << ", h=" << h << ": " << newEvent.toString() << " (total of " << _newEvents.size() << " instructions)" );
      }
      break;
  }
  logTraceOutWith1Argument( "addCommand()", _newEvents.size() );
}


#ifdef Parallel
void exahype2::RefinementControl::freeAllPendingSendRequests() {
  while (not _sendRequests.empty()) {
    std::vector<MPI_Request*>::iterator p = _sendRequests.begin();
    while (p!=_sendRequests.end()) {
      if (*p==nullptr) {
        p = _sendRequests.erase(p);
      }
      else {
        int flag;
        MPI_Test( *p, &flag, MPI_STATUS_IGNORE );
        if (flag) {
          *p = nullptr;
        }
        p++;
      }
    }
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  }
}


void exahype2::RefinementControl::triggerSendOfCopyOfCommittedEvents() {
  _sendRequests.clear();

  logInfo( "finishStep()", "share my " << _copyOfCommittedEvents.size() << " event(s) with others" );
  if ( _copyOfCommittedEvents.size()>0 ) {
    _sendRequests = std::vector< MPI_Request* >( tarch::mpi::Rank::getInstance().getNumberOfRanks(), nullptr );
    for (int rank=0; rank<tarch::mpi::Rank::getInstance().getNumberOfRanks(); rank++) {
      if (rank!=tarch::mpi::Rank::getInstance().getRank()) {
        _sendRequests[rank] = new MPI_Request;
        MPI_Isend( _copyOfCommittedEvents.data(), _copyOfCommittedEvents.size(), peano4::grid::GridControlEvent::getDatatype(), rank, _reductionTag, tarch::mpi::Rank::getInstance().getCommunicator(), _sendRequests[rank] );
      }
    }
  }
}
#endif


void exahype2::RefinementControl::finishStep() {
  if (not _newEvents.empty()) {
    logInfo( "finishStep()", "activate " << _newEvents.size() << " refinement/erase instructions (can be taken into account in next grid sweep)" );
  }

  {
    tarch::multicore::Lock lock( _semaphore );
    _committedEvents.clear();

    auto p = _newEvents.begin();
    while (p!=_newEvents.end()) {
      _committedEvents.push_back( p->first );
      p->second--;
      if (p->second<=0) {
        p = _newEvents.erase(p);
      }
      else {
        p++;
      }
    }
    _newEvents.clear();
  }

  #ifdef Parallel
  freeAllPendingSendRequests();
  _copyOfCommittedEvents = _committedEvents;
  triggerSendOfCopyOfCommittedEvents();
  #endif
}


void exahype2::RefinementControl::merge( const RefinementControl& control ) {
  tarch::multicore::Lock lock(_semaphore);
  _newEvents.insert( _newEvents.end(), control._newEvents.begin(), control._newEvents.end() );
}


std::string exahype2::RefinementControl::toString() const {
  std::ostringstream msg;
  msg << "("
      << "#new-events=" << _newEvents.size()
      << ",#committed-events=" << _committedEvents.size()
      << ")";
  return msg.str();
}
