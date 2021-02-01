#include "GridStatistics.h"



#include <sstream>
#include <algorithm>



peano4::grid::GridStatistics::GridStatistics(int  __numberOfLocalUnrefinedCells, int  __numberOfRemoteUnrefinedCells, int  __numberOfLocalRefinedCells, int  __numberOfRemoteRefinedCells, int  __stationarySweeps, bool  __coarseningHasBeenVetoed, tarch::la::Vector<Dimensions,double>  __minH):
    _numberOfLocalUnrefinedCells(__numberOfLocalUnrefinedCells)
  , _numberOfRemoteUnrefinedCells(__numberOfRemoteUnrefinedCells)
  , _numberOfLocalRefinedCells(__numberOfLocalRefinedCells)
  , _numberOfRemoteRefinedCells(__numberOfRemoteRefinedCells)
  , _stationarySweeps(__stationarySweeps)
  , _coarseningHasBeenVetoed(__coarseningHasBeenVetoed)
  , _minH(__minH)
  {}



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


tarch::la::Vector<Dimensions,double>   peano4::grid::GridStatistics::getMinH() const {
  return _minH;
}


void   peano4::grid::GridStatistics::setMinH(const tarch::la::Vector<Dimensions,double>& value) {
  _minH = value;
}


double   peano4::grid::GridStatistics::getMinH(int index) const {
  return _minH(index);
}


void   peano4::grid::GridStatistics::setMinH(int index, double value) {
  _minH(index) = value;
}






#ifdef Parallel
void peano4::grid::GridStatistics::sendAndPollDanglingMessages(const peano4::grid::GridStatistics& message, int destination, int tag ) {
  peano4::grid::GridStatistics::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridStatistics", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridStatistics", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::grid::GridStatistics::receiveAndPollDanglingMessages(peano4::grid::GridStatistics& message, int source, int tag ) {
  peano4::grid::GridStatistics::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridStatistics", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::grid::GridStatistics::Datatype;

int peano4::grid::GridStatistics::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::GridStatistics::initDatatype() {
  peano4::grid::GridStatistics  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_BYTE, MPI_DOUBLE };
    
  int blocklen[] = { 1, 1, 1, 1, 1, 1, Dimensions };

  const int NumberOfAttributes = 7;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._numberOfLocalUnrefinedCells), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._numberOfRemoteUnrefinedCells), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._numberOfLocalRefinedCells), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._numberOfRemoteRefinedCells), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._stationarySweeps), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._coarseningHasBeenVetoed), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._minH.data()[0]), &disp[currentAddress] );
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


void peano4::grid::GridStatistics::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::grid::GridStatistics::send(const peano4::grid::GridStatistics& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridStatistics::receive(peano4::grid::GridStatistics& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridStatistics::send(const peano4::grid::GridStatistics& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::grid::GridStatistics::receive(peano4::grid::GridStatistics& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
