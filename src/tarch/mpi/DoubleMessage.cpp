#include "DoubleMessage.h"



#include <sstream>
#include <algorithm>



tarch::mpi::DoubleMessage::DoubleMessage(double  __value):
    _value(__value)
  {}



std::string tarch::mpi::DoubleMessage::toString() const {
  std::ostringstream out;
  out << "(";
  out << "value=" << _value;
  out << ")";
  return out.str();
}


double   tarch::mpi::DoubleMessage::getValue() const {
  return _value;
}


void   tarch::mpi::DoubleMessage::setValue(double value) {
  _value = value;
}






#ifdef Parallel
void tarch::mpi::DoubleMessage::sendAndPollDanglingMessages(const tarch::mpi::DoubleMessage& message, int destination, int tag ) {
  tarch::mpi::DoubleMessage::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::DoubleMessage", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::DoubleMessage", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void tarch::mpi::DoubleMessage::receiveAndPollDanglingMessages(tarch::mpi::DoubleMessage& message, int source, int tag ) {
  tarch::mpi::DoubleMessage::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::DoubleMessage", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::DoubleMessage", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype tarch::mpi::DoubleMessage::Datatype;

int tarch::mpi::DoubleMessage::getSenderRank() const {
  return _senderDestinationRank;
}


void tarch::mpi::DoubleMessage::initDatatype() {
  tarch::mpi::DoubleMessage  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_DOUBLE };
    
  int blocklen[] = { 1 };

  const int NumberOfAttributes = 1;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._value), &disp[currentAddress] );
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


void tarch::mpi::DoubleMessage::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void tarch::mpi::DoubleMessage::send(const tarch::mpi::DoubleMessage& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void tarch::mpi::DoubleMessage::receive(tarch::mpi::DoubleMessage& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void tarch::mpi::DoubleMessage::send(const tarch::mpi::DoubleMessage& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void tarch::mpi::DoubleMessage::receive(tarch::mpi::DoubleMessage& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
