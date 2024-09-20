#include "GridControlEvent.h"

#include <sstream>
#include <algorithm>

peano4::grid::GridControlEvent::GridControlEvent(RefinementControl  __refinementControl, tarch::la::Vector<Dimensions,double>  __offset, tarch::la::Vector<Dimensions,double>  __width, tarch::la::Vector<Dimensions,double>  __h){
  setRefinementControl( __refinementControl);
  setOffset( __offset);
  setWidth( __width);
  setH( __h);
}

std::string peano4::grid::GridControlEvent::toString() const {
  std::ostringstream out;
  out << "(";
  out << "refinementControl=" << (_refinementControl==RefinementControl::Refine? "Refine" : "")  << (_refinementControl==RefinementControl::Erase? "Erase" : "") ;
  out << ","; 
  out << "offset=" << _offset;
  out << ","; 
  out << "width=" << _width;
  out << ","; 
  out << "h=" << _h;
  out << ")";
  return out.str();
}

peano4::grid::GridControlEvent::RefinementControl   peano4::grid::GridControlEvent::getRefinementControl() const {
  return _refinementControl;
}

void   peano4::grid::GridControlEvent::setRefinementControl(RefinementControl value) {
  _refinementControl = value;
}

tarch::la::Vector<Dimensions,double>   peano4::grid::GridControlEvent::getOffset() const {
  return   _offset;
}

void   peano4::grid::GridControlEvent::setOffset(const tarch::la::Vector<Dimensions,double>& value) {
  _offset = value;
}

double   peano4::grid::GridControlEvent::getOffset(int index) const {
  return   _offset(index);
}

void   peano4::grid::GridControlEvent::setOffset(int index, double value) {
  _offset(index) = value;
}

tarch::la::Vector<Dimensions,double>   peano4::grid::GridControlEvent::getWidth() const {
  return   _width;
}

void   peano4::grid::GridControlEvent::setWidth(const tarch::la::Vector<Dimensions,double>& value) {
  _width = value;
}

double   peano4::grid::GridControlEvent::getWidth(int index) const {
  return   _width(index);
}

void   peano4::grid::GridControlEvent::setWidth(int index, double value) {
  _width(index) = value;
}

tarch::la::Vector<Dimensions,double>   peano4::grid::GridControlEvent::getH() const {
  return   _h;
}

void   peano4::grid::GridControlEvent::setH(const tarch::la::Vector<Dimensions,double>& value) {
  _h = value;
}

double   peano4::grid::GridControlEvent::getH(int index) const {
  return   _h(index);
}

void   peano4::grid::GridControlEvent::setH(int index, double value) {
  _h(index) = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::GridControlEvent::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridControlEvent::getForkDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridControlEvent::getGlobalCommunciationDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridControlEvent::getJoinDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridControlEvent::getBoundaryExchangeDatatype() {
  return Datatype;
}

[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::GridControlEvent::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridControlEvent::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridControlEvent::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridControlEvent::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridControlEvent::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::GridControlEvent::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::grid::GridControlEvent::getSenderRank() const {
  return _senderDestinationRank;
}

void peano4::grid::GridControlEvent::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::GridControlEvent  instances[2];

  int NumberOfAttributes = 0;
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
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._refinementControl), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._offset.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._width.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[counter] );
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


void peano4::grid::GridControlEvent::shutdownDatatype() {
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

void peano4::grid::GridControlEvent::send(const peano4::grid::GridControlEvent& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}

void peano4::grid::GridControlEvent::receive(peano4::grid::GridControlEvent& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}

void peano4::grid::GridControlEvent::send(
  const peano4::grid::GridControlEvent& buffer,
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

void peano4::grid::GridControlEvent::receive(
  peano4::grid::GridControlEvent& buffer,
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
void peano4::grid::GridControlEvent::sendAndPollDanglingMessages(const peano4::grid::GridControlEvent& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::grid::GridControlEvent::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::grid::GridControlEvent::receiveAndPollDanglingMessages(peano4::grid::GridControlEvent& message, int source, int tag, MPI_Comm communicator ) {
  peano4::grid::GridControlEvent::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    