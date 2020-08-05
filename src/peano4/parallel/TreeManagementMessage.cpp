#include "TreeManagementMessage.h"



#include <sstream>
#include <algorithm>



peano4::parallel::TreeManagementMessage::TreeManagementMessage(int  __masterSpacetreeId, int  __workerSpacetreeId, Action  __action):
    _masterSpacetreeId(__masterSpacetreeId)
  , _workerSpacetreeId(__workerSpacetreeId)
  , _action(__action)
  {}



std::string peano4::parallel::TreeManagementMessage::toString() const {
  std::ostringstream out;
  out << "(";
  out << "masterSpacetreeId=" << _masterSpacetreeId;
  out << ","; 
  out << "workerSpacetreeId=" << _workerSpacetreeId;
  out << ","; 
  out << "action=" << (_action==Action::RequestNewRemoteTree? "RequestNewRemoteTree" : "")  << (_action==Action::CreateNewRemoteTree? "CreateNewRemoteTree" : "")  << (_action==Action::RemoveChildTreeFromBooksAsChildBecameEmpty? "RemoveChildTreeFromBooksAsChildBecameEmpty" : "")  << (_action==Action::JoinWithWorker? "JoinWithWorker" : "")  << (_action==Action::Acknowledgement? "Acknowledgement" : "") ;
  out << ")";
  return out.str();
}


int   peano4::parallel::TreeManagementMessage::getMasterSpacetreeId() const {
  return _masterSpacetreeId;
}


void   peano4::parallel::TreeManagementMessage::setMasterSpacetreeId(int value) {
  _masterSpacetreeId = value;
}


int   peano4::parallel::TreeManagementMessage::getWorkerSpacetreeId() const {
  return _workerSpacetreeId;
}


void   peano4::parallel::TreeManagementMessage::setWorkerSpacetreeId(int value) {
  _workerSpacetreeId = value;
}


peano4::parallel::TreeManagementMessage::Action   peano4::parallel::TreeManagementMessage::getAction() const {
  return _action;
}


void   peano4::parallel::TreeManagementMessage::setAction(Action value) {
  _action = value;
}






#ifdef Parallel
void peano4::parallel::TreeManagementMessage::sendAndPollDanglingMessages(const peano4::parallel::TreeManagementMessage& message, int destination, int tag ) {
  peano4::parallel::TreeManagementMessage::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::parallel::TreeManagementMessage", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::parallel::TreeManagementMessage", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::parallel::TreeManagementMessage::receiveAndPollDanglingMessages(peano4::parallel::TreeManagementMessage& message, int source, int tag ) {
  peano4::parallel::TreeManagementMessage::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::parallel::TreeManagementMessage", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::parallel::TreeManagementMessage", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::parallel::TreeManagementMessage::Datatype;

int peano4::parallel::TreeManagementMessage::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::parallel::TreeManagementMessage::initDatatype() {
  peano4::parallel::TreeManagementMessage  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_INT, MPI_INT, MPI_INT };
    
  int blocklen[] = { 1, 1, 1 };

  const int NumberOfAttributes = 3;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._masterSpacetreeId), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._workerSpacetreeId), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._action), &disp[currentAddress] );
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


void peano4::parallel::TreeManagementMessage::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::parallel::TreeManagementMessage::send(const peano4::parallel::TreeManagementMessage& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::parallel::TreeManagementMessage::receive(peano4::parallel::TreeManagementMessage& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::parallel::TreeManagementMessage::send(const peano4::parallel::TreeManagementMessage& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::parallel::TreeManagementMessage::receive(peano4::parallel::TreeManagementMessage& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
