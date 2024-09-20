#include "GridStatistics.h"

#include <sstream>
#include <algorithm>

peano4::grid::GridStatistics::GridStatistics(int  __numberOfLocalUnrefinedCells, int  __numberOfRemoteUnrefinedCells, int  __numberOfLocalRefinedCells, int  __numberOfRemoteRefinedCells, int  __stationarySweeps, bool  __coarseningHasBeenVetoed, bool  __removedEmptySubtree, tarch::la::Vector<Dimensions,double>  __minH){
  setNumberOfLocalUnrefinedCells( __numberOfLocalUnrefinedCells);
  setNumberOfRemoteUnrefinedCells( __numberOfRemoteUnrefinedCells);
  setNumberOfLocalRefinedCells( __numberOfLocalRefinedCells);
  setNumberOfRemoteRefinedCells( __numberOfRemoteRefinedCells);
  setStationarySweeps( __stationarySweeps);
  setCoarseningHasBeenVetoed( __coarseningHasBeenVetoed);
  setRemovedEmptySubtree( __removedEmptySubtree);
  setMinH( __minH);
}

std::string peano4::grid::GridStatistics::toString() const {
  std::ostringstream out;
  out << "(";
  out << "numberOfLocalUnrefinedCells=" << _numberOfLocalUnrefinedCells;
  out << ","; 
  out << "numberOfRemoteUnrefinedCells=" << _numberOfRemoteUnrefinedCells;
  out << ","; 
  out << "numberOfLocalRefinedCells=" << _numberOfLocalRefinedCells;
  out << ","; 
  out << "numberOfRemoteRefinedCells=" << _numberOfRemoteRefinedCells;
  out << ","; 
  out << "stationarySweeps=" << _stationarySweeps;
  out << ","; 
  out << "coarseningHasBeenVetoed=" << _coarseningHasBeenVetoed;
  out << ","; 
  out << "removedEmptySubtree=" << _removedEmptySubtree;
  out << ","; 
  out << "minH=" << _minH;
  out << ")";
  return out.str();
}

int   peano4::grid::GridStatistics::getNumberOfLocalUnrefinedCells() const {
  return _numberOfLocalUnrefinedCells;
}

void   peano4::grid::GridStatistics::setNumberOfLocalUnrefinedCells(int value) {
  _numberOfLocalUnrefinedCells = value;
}

int   peano4::grid::GridStatistics::getNumberOfRemoteUnrefinedCells() const {
  return _numberOfRemoteUnrefinedCells;
}

void   peano4::grid::GridStatistics::setNumberOfRemoteUnrefinedCells(int value) {
  _numberOfRemoteUnrefinedCells = value;
}

int   peano4::grid::GridStatistics::getNumberOfLocalRefinedCells() const {
  return _numberOfLocalRefinedCells;
}

void   peano4::grid::GridStatistics::setNumberOfLocalRefinedCells(int value) {
  _numberOfLocalRefinedCells = value;
}

int   peano4::grid::GridStatistics::getNumberOfRemoteRefinedCells() const {
  return _numberOfRemoteRefinedCells;
}

void   peano4::grid::GridStatistics::setNumberOfRemoteRefinedCells(int value) {
  _numberOfRemoteRefinedCells = value;
}

int   peano4::grid::GridStatistics::getStationarySweeps() const {
  return _stationarySweeps;
}

void   peano4::grid::GridStatistics::setStationarySweeps(int value) {
  _stationarySweeps = value;
}

bool   peano4::grid::GridStatistics::getCoarseningHasBeenVetoed() const {
  return _coarseningHasBeenVetoed;
}

void   peano4::grid::GridStatistics::setCoarseningHasBeenVetoed(bool value) {
  _coarseningHasBeenVetoed = value;
}

bool   peano4::grid::GridStatistics::getRemovedEmptySubtree() const {
  return _removedEmptySubtree;
}

void   peano4::grid::GridStatistics::setRemovedEmptySubtree(bool value) {
  _removedEmptySubtree = value;
}

tarch::la::Vector<Dimensions,double>   peano4::grid::GridStatistics::getMinH() const {
  return   _minH;
}

void   peano4::grid::GridStatistics::setMinH(const tarch::la::Vector<Dimensions,double>& value) {
  _minH = value;
}

double   peano4::grid::GridStatistics::getMinH(int index) const {
  return   _minH(index);
}

void   peano4::grid::GridStatistics::setMinH(int index, double value) {
  _minH(index) = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::GridStatistics::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridStatistics::getForkDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridStatistics::getGlobalCommunciationDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridStatistics::getJoinDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridStatistics::getBoundaryExchangeDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridStatistics::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridStatistics::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridStatistics::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridStatistics::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridStatistics::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridStatistics::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::grid::GridStatistics::getSenderRank() const {
  return _senderDestinationRank;
}

void peano4::grid::GridStatistics::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::GridStatistics  instances[2];

  int NumberOfAttributes = 0;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes];
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];

  int counter            = 0;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._numberOfLocalUnrefinedCells), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._numberOfRemoteUnrefinedCells), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._numberOfLocalRefinedCells), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._numberOfRemoteRefinedCells), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._stationarySweeps), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._coarseningHasBeenVetoed), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._removedEmptySubtree), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._minH.data()[0]), &disp[counter] );
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


void peano4::grid::GridStatistics::shutdownDatatype() {
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

void peano4::grid::GridStatistics::send(const peano4::grid::GridStatistics& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridStatistics::receive(peano4::grid::GridStatistics& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridStatistics::send(
  const peano4::grid::GridStatistics& buffer,
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

void peano4::grid::GridStatistics::receive(
  peano4::grid::GridStatistics& buffer,
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
void peano4::grid::GridStatistics::sendAndPollDanglingMessages(const peano4::grid::GridStatistics& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::grid::GridStatistics::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridStatistics", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridStatistics", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::grid::GridStatistics::receiveAndPollDanglingMessages(peano4::grid::GridStatistics& message, int source, int tag, MPI_Comm communicator ) {
  peano4::grid::GridStatistics::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    