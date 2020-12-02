#include "EulerCellSemaphoreLabel.h"



#include <sstream>
#include <algorithm>



examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::EulerCellSemaphoreLabel(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, int  __SemaphoreNumber):
    _debugX(__debugX)
  , _debugH(__debugH)
  , _SemaphoreNumber(__SemaphoreNumber)
  {}



std::string examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::toString() const {
  std::ostringstream out;
  out << "(";
  out << "debugX=" << _debugX;
  out << ","; 
  out << "debugH=" << _debugH;
  out << ","; 
  out << "SemaphoreNumber=" << _SemaphoreNumber;
  out << ")";
  return out.str();
}


tarch::la::Vector<Dimensions,double>   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getDebugX() const {
  return _debugX;
}


void   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getDebugX(int index) const {
  return _debugX(index);
}


void   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::setDebugX(int index, double value) {
  _debugX(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getDebugH() const {
  return _debugH;
}


void   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getDebugH(int index) const {
  return _debugH(index);
}


void   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::setDebugH(int index, double value) {
  _debugH(index) = value;
}


int   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getSemaphoreNumber() const {
  return _SemaphoreNumber;
}


void   examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::setSemaphoreNumber(int value) {
  _SemaphoreNumber = value;
}






#ifdef Parallel

MPI_Datatype examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::Datatype;

int examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::getSenderRank() const {
  return _senderDestinationRank;
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::initDatatype() {
  examples::exahype2::euler::celldata::EulerCellSemaphoreLabel  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_INT };
    
  int blocklen[] = { Dimensions, Dimensions, 1 };

  const int NumberOfAttributes = 3;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._SemaphoreNumber), &disp[currentAddress] );
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


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::send(const examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::receive(examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::send(const examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::receive(examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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

#ifdef Parallel
void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::sendAndPollDanglingMessages(const examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& message, int destination, int tag ) {
  examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler::celldata::EulerCellSemaphoreLabel", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler::celldata::EulerCellSemaphoreLabel", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::receiveAndPollDanglingMessages(examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& message, int source, int tag ) {
  examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler::celldata::EulerCellSemaphoreLabel", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler::celldata::EulerCellSemaphoreLabel", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
void examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::merge(const examples::exahype2::euler::celldata::EulerCellSemaphoreLabel& neighbour, const peano4::datamanagement::CellMarker& marker) {
}


bool examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::exahype2::euler::celldata::EulerCellSemaphoreLabel::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
