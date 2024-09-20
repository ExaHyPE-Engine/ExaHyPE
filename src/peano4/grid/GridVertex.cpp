#include "GridVertex.h"

#include <sstream>
#include <algorithm>

peano4::grid::GridVertex::GridVertex(
  [[maybe_unused]] State __state,
  [[maybe_unused]] tarch::la::Vector<TwoPowerD, int> __adjacentRanks,
  [[maybe_unused]] tarch::la::Vector<TwoPowerD, int> __backupOfAdjacentRanks,
  [[maybe_unused]] bool __hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep,
  [[maybe_unused]] bool __isAntecessorOfRefinedVertexInCurrentTreeSweep,
  [[maybe_unused]] bool __hasBeenParentOfSubtreeVertexInPreviousTreeSweep,
  [[maybe_unused]] bool __isParentOfSubtreeVertexInCurrentTreeSweep,
  [[maybe_unused]] int __numberOfAdjacentRefinedLocalCells,
  [[maybe_unused]] tarch::la::Vector<Dimensions, double> __x,
  [[maybe_unused]] int __level
) {
  setState( __state);
  setAdjacentRanks( __adjacentRanks);
  setBackupOfAdjacentRanks( __backupOfAdjacentRanks);
  setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep( __hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep);
  setIsAntecessorOfRefinedVertexInCurrentTreeSweep( __isAntecessorOfRefinedVertexInCurrentTreeSweep);
  setHasBeenParentOfSubtreeVertexInPreviousTreeSweep( __hasBeenParentOfSubtreeVertexInPreviousTreeSweep);
  setIsParentOfSubtreeVertexInCurrentTreeSweep( __isParentOfSubtreeVertexInCurrentTreeSweep);
  setNumberOfAdjacentRefinedLocalCells( __numberOfAdjacentRefinedLocalCells);
  #if PeanoDebug>0
  setX( __x);
  #endif 
  setLevel( __level);
}

peano4::grid::GridVertex::GridVertex( const GridVertex& copy ) {
  setState( copy.getState() );
  setAdjacentRanks( copy.getAdjacentRanks() );
  setBackupOfAdjacentRanks( copy.getBackupOfAdjacentRanks() );
  setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep( copy.getHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep() );
  setIsAntecessorOfRefinedVertexInCurrentTreeSweep( copy.getIsAntecessorOfRefinedVertexInCurrentTreeSweep() );
  setHasBeenParentOfSubtreeVertexInPreviousTreeSweep( copy.getHasBeenParentOfSubtreeVertexInPreviousTreeSweep() );
  setIsParentOfSubtreeVertexInCurrentTreeSweep( copy.getIsParentOfSubtreeVertexInCurrentTreeSweep() );
  setNumberOfAdjacentRefinedLocalCells( copy.getNumberOfAdjacentRefinedLocalCells() );
#if PeanoDebug>0
  setX( copy.getX() );
#endif
  setLevel( copy.getLevel() );
}

peano4::grid::GridVertex& peano4::grid::GridVertex::operator = (const GridVertex& other) {
  if (this == &other) {
    return *this; // Self-assignment check
  }

  setState(other.getState());
  setAdjacentRanks(other.getAdjacentRanks());
  setBackupOfAdjacentRanks(other.getBackupOfAdjacentRanks());
  setHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep(other.getHasBeenAntecessorOfRefinedVertexInPreviousTreeSweep());
  setIsAntecessorOfRefinedVertexInCurrentTreeSweep(other.getIsAntecessorOfRefinedVertexInCurrentTreeSweep());
  setHasBeenParentOfSubtreeVertexInPreviousTreeSweep(other.getHasBeenParentOfSubtreeVertexInPreviousTreeSweep());
  setIsParentOfSubtreeVertexInCurrentTreeSweep(other.getIsParentOfSubtreeVertexInCurrentTreeSweep());
  setNumberOfAdjacentRefinedLocalCells(other.getNumberOfAdjacentRefinedLocalCells());
#if PeanoDebug>0
  setX(other.getX());
#endif
  setLevel(other.getLevel());

  return *this;
}

std::string peano4::grid::GridVertex::toString() const {
  std::ostringstream out;
  out << "(";
  out << "state=" << (_state==State::HangingVertex? "HangingVertex" : "")  << (_state==State::New? "New" : "")  << (_state==State::Unrefined? "Unrefined" : "")  << (_state==State::Refined? "Refined" : "")  << (_state==State::RefinementTriggered? "RefinementTriggered" : "")  << (_state==State::Refining? "Refining" : "")  << (_state==State::EraseTriggered? "EraseTriggered" : "")  << (_state==State::Erasing? "Erasing" : "")  << (_state==State::Delete? "Delete" : "") ;
  out << ","; 
  out << "adjacentRanks=" << getAdjacentRanks();
  out << ","; 
  out << "backupOfAdjacentRanks=" << _backupOfAdjacentRanks;
  out << ","; 
  out << "hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep=" << _hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep;
  out << ","; 
  out << "isAntecessorOfRefinedVertexInCurrentTreeSweep=" << _isAntecessorOfRefinedVertexInCurrentTreeSweep;
  out << ","; 
  out << "hasBeenParentOfSubtreeVertexInPreviousTreeSweep=" << _hasBeenParentOfSubtreeVertexInPreviousTreeSweep;
  out << ","; 
  out << "isParentOfSubtreeVertexInCurrentTreeSweep=" << _isParentOfSubtreeVertexInCurrentTreeSweep;
  out << ","; 
  out << "numberOfAdjacentRefinedLocalCells=" << _numberOfAdjacentRefinedLocalCells;
#if PeanoDebug>0
  out << ","; 
  out << "x=" << getX();
#endif 
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

  tarch::la::Vector<TwoPowerD,int> result;
  for( int i=0; i<TwoPowerD; i++) {
    result(i) =   _adjacentRanks[i];
  }
  return result;
      }


void   peano4::grid::GridVertex::setAdjacentRanks(const tarch::la::Vector<TwoPowerD,int>& value) {

  for( int i=0; i<TwoPowerD; i++) {
      _adjacentRanks[i] = value(i);
  }
      }


int   peano4::grid::GridVertex::getAdjacentRanks(int index) const {
  return   _adjacentRanks[index];
}


void   peano4::grid::GridVertex::setAdjacentRanks(int index, int value) {
  _adjacentRanks[index] = value;
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridVertex::getBackupOfAdjacentRanks() const {
  return    _backupOfAdjacentRanks;
}


void   peano4::grid::GridVertex::setBackupOfAdjacentRanks(const tarch::la::Vector<TwoPowerD,int>& value) {
  _backupOfAdjacentRanks = value;
}


int   peano4::grid::GridVertex::getBackupOfAdjacentRanks(int index) const {
  return   _backupOfAdjacentRanks(index);
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


bool   peano4::grid::GridVertex::getHasBeenParentOfSubtreeVertexInPreviousTreeSweep() const {
  return _hasBeenParentOfSubtreeVertexInPreviousTreeSweep;
}


void   peano4::grid::GridVertex::setHasBeenParentOfSubtreeVertexInPreviousTreeSweep(bool value) {
  _hasBeenParentOfSubtreeVertexInPreviousTreeSweep = value;
}


bool   peano4::grid::GridVertex::getIsParentOfSubtreeVertexInCurrentTreeSweep() const {
  return _isParentOfSubtreeVertexInCurrentTreeSweep;
}


void   peano4::grid::GridVertex::setIsParentOfSubtreeVertexInCurrentTreeSweep(bool value) {
  _isParentOfSubtreeVertexInCurrentTreeSweep = value;
}


int   peano4::grid::GridVertex::getNumberOfAdjacentRefinedLocalCells() const {
  return _numberOfAdjacentRefinedLocalCells;
}


void   peano4::grid::GridVertex::setNumberOfAdjacentRefinedLocalCells(int value) {
  _numberOfAdjacentRefinedLocalCells = value;
}


#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   peano4::grid::GridVertex::getX() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _x[i];
  }
  return result;
      }


void   peano4::grid::GridVertex::setX(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _x[i] = value(i);
  }
      }


double   peano4::grid::GridVertex::getX(int index) const {
  return   _x[index];
}


void   peano4::grid::GridVertex::setX(int index, double value) {
  _x[index] = value;
}


#endif 


int   peano4::grid::GridVertex::getLevel() const {
  return _level;
}


void   peano4::grid::GridVertex::setLevel(int value) {
  _level = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::GridVertex::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridVertex::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridVertex::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridVertex::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridVertex::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridVertex::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridVertex::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridVertex::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridVertex::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridVertex::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridVertex::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::grid::GridVertex::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::GridVertex::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::GridVertex  instances[2];

  int NumberOfAttributes = 0;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes];
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];

  int counter            = 0;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoPowerD;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = TwoPowerD;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
#if PeanoDebug>0
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
#endif
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._state), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._adjacentRanks.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._backupOfAdjacentRanks.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hasBeenAntecessorOfRefinedVertexInPreviousTreeSweep), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isAntecessorOfRefinedVertexInCurrentTreeSweep), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._hasBeenParentOfSubtreeVertexInPreviousTreeSweep), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._isParentOfSubtreeVertexInCurrentTreeSweep), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._numberOfAdjacentRefinedLocalCells), &disp[counter] );
  counter++;

#if PeanoDebug>0
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[counter] );
  counter++;
#endif
  MPI_Get_address( &(instances[0]._level), &disp[counter] );
  counter++;

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

  delete[] subtypes;
  delete[] blocklen;
  delete[] disp;

  #else
  // invoke routine once to trigger lazy initialisation
  getForkDatatype();
  getJoinDatatype();
  getBoundaryExchangeDatatype();
  getMultiscaleDataExchangeDatatype();
  getGlobalCommunciationDatatype();
  #endif
}


void peano4::grid::GridVertex::shutdownDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunciationDatatype();
  #else
  MPI_Datatype type = Datatype;
  MPI_Type_free( &type );
  #endif
}


void peano4::grid::GridVertex::send(const peano4::grid::GridVertex& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridVertex::receive(peano4::grid::GridVertex& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridVertex::send(
  const peano4::grid::GridVertex& buffer,
  int destination,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  }
}


void peano4::grid::GridVertex::receive(
  peano4::grid::GridVertex& buffer,
  int source,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle );
  MPI_Test( &receiveRequestHandle, &flag, &status );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status );
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}
#endif

#ifdef Parallel
void peano4::grid::GridVertex::sendAndPollDanglingMessages(const peano4::grid::GridVertex& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::grid::GridVertex::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridVertex", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridVertex", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::grid::GridVertex::receiveAndPollDanglingMessages(peano4::grid::GridVertex& message, int source, int tag, MPI_Comm communicator ) {
  peano4::grid::GridVertex::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridVertex", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    