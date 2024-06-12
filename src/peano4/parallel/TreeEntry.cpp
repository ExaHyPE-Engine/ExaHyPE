#include "TreeEntry.h"



#include <sstream>
#include <algorithm>



peano4::parallel::TreeEntry::TreeEntry(int  __id, int  __master){
setId( __id);
setMaster( __master);
}



std::string peano4::parallel::TreeEntry::toString() const {
  std::ostringstream out;
  out << "(";
  out << "id=" << _id;
  out << ","; 
  out << "master=" << _master;
  out << ")";
  return out.str();
}





int   peano4::parallel::TreeEntry::getId() const {
  return _id;
}


void   peano4::parallel::TreeEntry::setId(int value) {
  _id = value;
}


int   peano4::parallel::TreeEntry::getMaster() const {
  return _master;
}


void   peano4::parallel::TreeEntry::setMaster(int value) {
  _master = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::parallel::TreeEntry::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::parallel::TreeEntry::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::parallel::TreeEntry::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::parallel::TreeEntry::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::parallel::TreeEntry::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::parallel::TreeEntry::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::parallel::TreeEntry::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::parallel::TreeEntry::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::parallel::TreeEntry::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::parallel::TreeEntry::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::parallel::TreeEntry::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::parallel::TreeEntry::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::parallel::TreeEntry::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::parallel::TreeEntry  instances[2];

  int NumberOfAttributes = 0;
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
   
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  
  counter = 0;
  MPI_Get_address( &(instances[0]._id), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._master), &disp[counter] );
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


void peano4::parallel::TreeEntry::shutdownDatatype() {
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


void peano4::parallel::TreeEntry::send(const peano4::parallel::TreeEntry& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::parallel::TreeEntry::receive(peano4::parallel::TreeEntry& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::parallel::TreeEntry::send(
  const peano4::parallel::TreeEntry& buffer, 
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


void peano4::parallel::TreeEntry::receive(
  peano4::parallel::TreeEntry& buffer, 
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
void peano4::parallel::TreeEntry::sendAndPollDanglingMessages(const peano4::parallel::TreeEntry& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::parallel::TreeEntry::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::parallel::TreeEntry", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::parallel::TreeEntry", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::parallel::TreeEntry::receiveAndPollDanglingMessages(peano4::parallel::TreeEntry& message, int source, int tag, MPI_Comm communicator ) {
  peano4::parallel::TreeEntry::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::parallel::TreeEntry", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::parallel::TreeEntry", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    