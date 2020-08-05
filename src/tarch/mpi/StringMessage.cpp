#include "StringMessage.h"

#include "tarch/logging/Log.h"
#include "tarch/services/ServiceRepository.h"

#include <sstream>
#include <algorithm>



std::string tarch::mpi::StringMessage::toString() const {
  std::ostringstream out;
  out << "(";
  out << "data=" << getData();
  out << ",rank=" << _senderDestinationRank;
  out << ")";
  return out.str();
}


std::string   tarch::mpi::StringMessage::getData() const {
  return _data;
}


void   tarch::mpi::StringMessage::setData(const std::string& value) {
  _data = value;
}


#ifdef Parallel
void tarch::mpi::StringMessage::sendAndPollDanglingMessages(const tarch::mpi::StringMessage& message, int destination, int tag ) {
  tarch::mpi::StringMessage::send(
    message, destination, tag,
    [&]() {
      auto timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::StringMessage", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::StringMessage", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void tarch::mpi::StringMessage::receiveAndPollDanglingMessages(tarch::mpi::StringMessage& message, int source, int tag ) {
  tarch::mpi::StringMessage::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "tarch::mpi::StringMessage", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "tarch::mpi::StringMessage", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

int tarch::mpi::StringMessage::getSenderRank() const {
  return _senderDestinationRank;
}


void tarch::mpi::StringMessage::send(const tarch::mpi::StringMessage& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( buffer._data.c_str(), buffer._data.size()+1, MPI_CHAR, destination, tag, communicator);
}


void tarch::mpi::StringMessage::receive(tarch::mpi::StringMessage& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Probe( source, tag, communicator, &status );
  int count;
  MPI_Get_count( &status, MPI_CHAR, &count );
  char tmp[count];
  MPI_Recv( &tmp, count, MPI_CHAR, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
  buffer._data = tmp;
}


void tarch::mpi::StringMessage::send(const tarch::mpi::StringMessage& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( buffer._data.c_str(), buffer._data.size()+1, MPI_CHAR, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void tarch::mpi::StringMessage::receive(tarch::mpi::StringMessage& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle; 
  int         flag = 0; 
  MPI_Iprobe( source, tag, communicator, &flag, &status );
  while (!flag) { 
    waitFunctor();
    MPI_Iprobe( source, tag, communicator, &flag, &status );
  }
  int count;
  MPI_Get_count( &status, MPI_CHAR, &count );
  char tmp[count];
  MPI_Recv( &tmp, count, MPI_CHAR, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
  buffer._data = tmp;
}


bool tarch::mpi::StringMessage::isMessageInQueue(int tag, MPI_Comm communicator) {
  int  flag        = 0;
  MPI_Iprobe(
    MPI_ANY_SOURCE, tag,
    communicator, &flag, MPI_STATUS_IGNORE
  );
  return flag;
}

#endif
