#include "GeneratedTestObject_Flattened.h"



#include <sstream>
#include <algorithm>



peano4::stacks::tests::GeneratedTestObject_Flattened::GeneratedTestObject_Flattened(int  __id, tarch::la::Vector<256,double>  __data){
setId( __id);
setData( __data);
}



std::string peano4::stacks::tests::GeneratedTestObject_Flattened::toString() const {
  std::ostringstream out;
  out << "(";
  out << "id=" << _id;
  out << ","; 
  out << "data=" << _data;
  out << ")";
  return out.str();
}





int   peano4::stacks::tests::GeneratedTestObject_Flattened::getId() const {
  return _id;
}


void   peano4::stacks::tests::GeneratedTestObject_Flattened::setId(int value) {
  _id = value;
}


tarch::la::Vector<256,double>   peano4::stacks::tests::GeneratedTestObject_Flattened::getData() const {
  return   _data;
}


void   peano4::stacks::tests::GeneratedTestObject_Flattened::setData(const tarch::la::Vector<256,double>& value) {
  _data = value;
}


double   peano4::stacks::tests::GeneratedTestObject_Flattened::getData(int index) const {
  return   _data(index);
}


void   peano4::stacks::tests::GeneratedTestObject_Flattened::setData(int index, double value) {
  _data(index) = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::stacks::tests::GeneratedTestObject_Flattened::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::stacks::tests::GeneratedTestObject_Flattened::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::stacks::tests::GeneratedTestObject_Flattened::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::stacks::tests::GeneratedTestObject_Flattened::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::stacks::tests::GeneratedTestObject_Flattened::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::stacks::tests::GeneratedTestObject_Flattened::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::stacks::tests::GeneratedTestObject_Flattened::getSenderRank() const {
  return _senderDestinationRank;
}



void peano4::stacks::tests::GeneratedTestObject_Flattened::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::stacks::tests::GeneratedTestObject_Flattened  instances[2];

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
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 256;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._id), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._data.data()[0]), &disp[counter] );
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


void peano4::stacks::tests::GeneratedTestObject_Flattened::shutdownDatatype() {
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


void peano4::stacks::tests::GeneratedTestObject_Flattened::send(const peano4::stacks::tests::GeneratedTestObject_Flattened& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::stacks::tests::GeneratedTestObject_Flattened::receive(peano4::stacks::tests::GeneratedTestObject_Flattened& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::stacks::tests::GeneratedTestObject_Flattened::send(
  const peano4::stacks::tests::GeneratedTestObject_Flattened& buffer,
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


void peano4::stacks::tests::GeneratedTestObject_Flattened::receive(
  peano4::stacks::tests::GeneratedTestObject_Flattened& buffer,
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
void peano4::stacks::tests::GeneratedTestObject_Flattened::sendAndPollDanglingMessages(const peano4::stacks::tests::GeneratedTestObject_Flattened& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::stacks::tests::GeneratedTestObject_Flattened::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::stacks::tests::GeneratedTestObject_Flattened", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::stacks::tests::GeneratedTestObject_Flattened", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::stacks::tests::GeneratedTestObject_Flattened::receiveAndPollDanglingMessages(peano4::stacks::tests::GeneratedTestObject_Flattened& message, int source, int tag, MPI_Comm communicator ) {
  peano4::stacks::tests::GeneratedTestObject_Flattened::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::stacks::tests::GeneratedTestObject_Flattened", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::stacks::tests::GeneratedTestObject_Flattened", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    