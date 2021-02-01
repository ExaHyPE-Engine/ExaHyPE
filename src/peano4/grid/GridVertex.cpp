#include "GridVertex.h"



#include <sstream>
#include <algorithm>



peano4::grid::GridVertex::GridVertex(State  __state, tarch::la::Vector<TwoPowerD,int>  __adjacentRanks, tarch::la::Vector<TwoPowerD,int>  __backupOfAdjacentRanks, bool  __hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep, bool  __isAntecessorOfRefinedVertexInCurrentTreeSweep, int  __numberOfAdjacentRefinedLocalCells, tarch::la::Vector<Dimensions,double>  __x, int  __level):
    _state(__state)
  , _adjacentRanks(__adjacentRanks)
  , _backupOfAdjacentRanks(__backupOfAdjacentRanks)
  , _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(__hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep)
  , _isAntecessorOfRefinedVertexInCurrentTreeSweep(__isAntecessorOfRefinedVertexInCurrentTreeSweep)
  , _numberOfAdjacentRefinedLocalCells(__numberOfAdjacentRefinedLocalCells)
  , _x(__x)
  , _level(__level)
  {}



std::string peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "(";
  out << "state=" << (_state==State::HangingVertex? "HangingVertex" : "")  << (_state==State::New? "New" : "")  << (_state==State::Unrefined? "Unrefined" : "")  << (_state==State::Refined? "Refined" : "")  << (_state==State::RefinementTriggered? "RefinementTriggered" : "")  << (_state==State::Refining? "Refining" : "")  << (_state==State::EraseTriggered? "EraseTriggered" : "")  << (_state==State::Erasing? "Erasing" : "")  << (_state==State::Delete? "Delete" : "") ;
  out << ","; 
  out << "adjacentRanks=" << _adjacentRanks;
  out << ","; 
  out << "backupOfAdjacentRanks=" << _backupOfAdjacentRanks;
  out << ","; 
  out << "hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=" << _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep;
  out << ","; 
  out << "isAntecessorOfRefinedVertexInCurrentTreeSweep=" << _isAntecessorOfRefinedVertexInCurrentTreeSweep;
  out << ","; 
  out << "numberOfAdjacentRefinedLocalCells=" << _numberOfAdjacentRefinedLocalCells;
  out << ","; 
  out << "x=" << _x;
  out << ","; 
  out << "level=" << _level;
  out << ")";
  return out.str();
}


peano4::grid::GridVertex::State   peano4::grid::GridVertex::getState() const {
  return _state;
}


void   peano4::grid::GridVertex::setState(State value) {
  _state = value;
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridVertex::getAdjacentRanks() const {
  return _adjacentRanks;
}


void   peano4::grid::GridVertex::setAdjacentRanks(const tarch::la::Vector<TwoPowerD,int>& value) {
  _adjacentRanks = value;
}


int   peano4::grid::GridVertex::getAdjacentRanks(int index) const {
  return _adjacentRanks(index);
}


void   peano4::grid::GridVertex::setAdjacentRanks(int index, int value) {
  _adjacentRanks(index) = value;
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridVertex::getBackupOfAdjacentRanks() const {
  return _backupOfAdjacentRanks;
}


void   peano4::grid::GridVertex::setBackupOfAdjacentRanks(const tarch::la::Vector<TwoPowerD,int>& value) {
  _backupOfAdjacentRanks = value;
}


int   peano4::grid::GridVertex::getBackupOfAdjacentRanks(int index) const {
  return _backupOfAdjacentRanks(index);
}


void   peano4::grid::GridVertex::setBackupOfAdjacentRanks(int index, int value) {
  _backupOfAdjacentRanks(index) = value;
}


bool   peano4::grid::GridVertex::getHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep() const {
  return _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep;
}


void   peano4::grid::GridVertex::setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(bool value) {
  _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep = value;
}


bool   peano4::grid::GridVertex::getIsAntecessorOfRefinedVertexInCurrentTreeSweep() const {
  return _isAntecessorOfRefinedVertexInCurrentTreeSweep;
}


void   peano4::grid::GridVertex::setIsAntecessorOfRefinedVertexInCurrentTreeSweep(bool value) {
  _isAntecessorOfRefinedVertexInCurrentTreeSweep = value;
}


int   peano4::grid::GridVertex::getNumberOfAdjacentRefinedLocalCells() const {
  return _numberOfAdjacentRefinedLocalCells;
}


void   peano4::grid::GridVertex::setNumberOfAdjacentRefinedLocalCells(int value) {
  _numberOfAdjacentRefinedLocalCells = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridVertex::getX() const {
  return _x;
}


void   peano4::grid::GridVertex::setX(const tarch::la::Vector<Dimensions,double>& value) {
  _x = value;
}


double   peano4::grid::GridVertex::getX(int index) const {
  return _x(index);
}


void   peano4::grid::GridVertex::setX(int index, double value) {
  _x(index) = value;
}


int   peano4::grid::GridVertex::getLevel() const {
  return _level;
}


void   peano4::grid::GridVertex::setLevel(int value) {
  _level = value;
}






#ifdef Parallel
void peano4::grid::GridVertex::sendAndPollDanglingMessages(const peano4::grid::GridVertex& message, int destination, int tag ) {
  peano4::grid::GridVertex::send(
    message, destination, tag,
    [&]() {
      auto  timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridVertex", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridVertex", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::grid::GridVertex::receiveAndPollDanglingMessages(peano4::grid::GridVertex& message, int source, int tag ) {
  peano4::grid::GridVertex::receive(
    message, source, tag,
    [&]() {
      auto timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::grid::GridVertex::Datatype;

int peano4::grid::GridVertex::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::GridVertex::initDatatype() {
  peano4::grid::GridVertex  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_INT, MPI_INT, MPI_INT, MPI_BYTE, MPI_BYTE, MPI_INT, MPI_DOUBLE, MPI_INT };
    
  int blocklen[] = { 1, TwoPowerD, TwoPowerD, 1, 1, 1, Dimensions, 1 };

  const int NumberOfAttributes = 8;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._state), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._adjacentRanks.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._backupOfAdjacentRanks.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isAntecessorOfRefinedVertexInCurrentTreeSweep), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._numberOfAdjacentRefinedLocalCells), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._level), &disp[currentAddress] );
  currentAddress++;

  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i=NumberOfAttributes-1; i>=0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int errorCode = 0; 
  MPI_Datatype tmpType; 
  errorCode += MPI_Type_create_struct( NumberOfAttributes, blocklen, disp, subtypes, &tmpType );
  errorCode += MPI_Type_create_resized( tmpType, offset, extent, &Datatype );
  errorCode += MPI_Type_commit( &Datatype );
  errorCode += MPI_Type_free( &tmpType );
  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;
}


void peano4::grid::GridVertex::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::grid::GridVertex::send(const peano4::grid::GridVertex& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridVertex::receive(peano4::grid::GridVertex& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridVertex::send(const peano4::grid::GridVertex& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::grid::GridVertex::receive(peano4::grid::GridVertex& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
  MPI_Status  status;
  MPI_Request receiveRequestHandle; 
  int         flag = 0; 
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle ); 
  MPI_Test( &receiveRequestHandle, &flag, &status ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status ); 
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}

#endif
