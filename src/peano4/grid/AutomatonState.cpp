#include "AutomatonState.h"

#include <sstream>
#include <algorithm>

peano4::grid::AutomatonState::AutomatonState(int  __level, tarch::la::Vector<Dimensions,double>  __x, tarch::la::Vector<Dimensions,double>  __h, bool  __inverted, std::bitset<Dimensions>  __evenFlags, tarch::la::Vector<DimensionsTimesTwo,int>  __accessNumber){
  setLevel( __level);
  setX( __x);
  setH( __h);
  setInverted( __inverted);
  setEvenFlags( __evenFlags);
  setAccessNumber( __accessNumber);
}

peano4::grid::AutomatonState::AutomatonState( const AutomatonState& copy ) {
  setLevel( copy.getLevel() );
  setX( copy.getX() );
  setH( copy.getH() );
  setInverted( copy.getInverted() );
  setEvenFlags( copy.getEvenFlags() );
  setAccessNumber( copy.getAccessNumber() );
}

peano4::grid::AutomatonState& peano4::grid::AutomatonState::operator = (const AutomatonState& other) {
  if (this == &other) {
    return *this; // Self-assignment check
  }

  setLevel(other.getLevel());
  setX(other.getX());
  setH(other.getH());
  setInverted(other.getInverted());
  setEvenFlags(other.getEvenFlags());
  setAccessNumber(other.getAccessNumber());

  return *this;
}

std::string peano4::grid::AutomatonState::toString() const {
  std::ostringstream out;
  out << "(";
  out << "level=" << _level;
  out << ","; 
  out << "x=" << getX();
  out << ","; 
  out << "h=" << getH();
  out << ","; 
  out << "inverted=" << _inverted;
  out << ","; 
  out << "evenFlags=" << getEvenFlags();
  out << ","; 
  out << "accessNumber=" << getAccessNumber();
  out << ")";
  return out.str();
}





int   peano4::grid::AutomatonState::getLevel() const {
  return _level;
}


void   peano4::grid::AutomatonState::setLevel(int value) {
  _level = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::AutomatonState::getX() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _x[i];
  }
  return result;
      }


void   peano4::grid::AutomatonState::setX(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _x[i] = value(i);
  }
      }


double   peano4::grid::AutomatonState::getX(int index) const {
  return   _x[index];
}


void   peano4::grid::AutomatonState::setX(int index, double value) {
  _x[index] = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::AutomatonState::getH() const {

  tarch::la::Vector<Dimensions,double> result;
  for( int i=0; i<Dimensions; i++) {
    result(i) =   _h[i];
  }
  return result;
      }


void   peano4::grid::AutomatonState::setH(const tarch::la::Vector<Dimensions,double>& value) {

  for( int i=0; i<Dimensions; i++) {
      _h[i] = value(i);
  }
      }


double   peano4::grid::AutomatonState::getH(int index) const {
  return   _h[index];
}


void   peano4::grid::AutomatonState::setH(int index, double value) {
  _h[index] = value;
}


bool   peano4::grid::AutomatonState::getInverted() const {
  return _inverted;
}


void   peano4::grid::AutomatonState::setInverted(bool value) {
  _inverted = value;
}


std::bitset<Dimensions>   peano4::grid::AutomatonState::getEvenFlags() const {

  std::bitset<Dimensions> result;
  for (int i=0; i<Dimensions; i++) result[i] =   _evenFlags[i];
  return result;
}


void   peano4::grid::AutomatonState::setEvenFlags(const std::bitset<Dimensions>&  value) {

  for (int i=0; i<Dimensions; i++)   _evenFlags[i]=value[i];
}


bool   peano4::grid::AutomatonState::getEvenFlags(int index) const {
  return   _evenFlags[index];
}


void   peano4::grid::AutomatonState::setEvenFlags(int index, bool value) {
  _evenFlags[index] = value;
}


void   peano4::grid::AutomatonState::flipEvenFlags(int index) {
  _evenFlags[index] = not   _evenFlags[index];
}


tarch::la::Vector<DimensionsTimesTwo,int>   peano4::grid::AutomatonState::getAccessNumber() const {

  tarch::la::Vector<DimensionsTimesTwo,int> result;
  for( int i=0; i<DimensionsTimesTwo; i++) {
    result(i) =   _accessNumber[i];
  }
  return result;
      }


void   peano4::grid::AutomatonState::setAccessNumber(const tarch::la::Vector<DimensionsTimesTwo,int>& value) {

  for( int i=0; i<DimensionsTimesTwo; i++) {
      _accessNumber[i] = value(i);
  }
      }


int   peano4::grid::AutomatonState::getAccessNumber(int index) const {
  return   _accessNumber[index];
}


void   peano4::grid::AutomatonState::setAccessNumber(int index, int value) {
  _accessNumber[index] = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype peano4::grid::AutomatonState::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::AutomatonState::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::AutomatonState::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::AutomatonState::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::AutomatonState::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype peano4::grid::AutomatonState::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void peano4::grid::AutomatonState::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::AutomatonState::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::AutomatonState::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::AutomatonState::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void peano4::grid::AutomatonState::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int peano4::grid::AutomatonState::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::AutomatonState::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  peano4::grid::AutomatonState  instances[2];

  int NumberOfAttributes = 0;
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
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_INT;
  blocklen[counter] = DimensionsTimesTwo;
  counter++;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
  MPI_Get_address( &(instances[0]._level), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._inverted), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._evenFlags), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._accessNumber.data()[0]), &disp[counter] );
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


void peano4::grid::AutomatonState::shutdownDatatype() {
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


void peano4::grid::AutomatonState::send(const peano4::grid::AutomatonState& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::AutomatonState::receive(peano4::grid::AutomatonState& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::AutomatonState::send(
  const peano4::grid::AutomatonState& buffer,
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


void peano4::grid::AutomatonState::receive(
  peano4::grid::AutomatonState& buffer,
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
void peano4::grid::AutomatonState::sendAndPollDanglingMessages(const peano4::grid::AutomatonState& message, int destination, int tag, MPI_Comm communicator ) {
  peano4::grid::AutomatonState::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::AutomatonState", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::AutomatonState", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::grid::AutomatonState::receiveAndPollDanglingMessages(peano4::grid::AutomatonState& message, int source, int tag, MPI_Comm communicator ) {
  peano4::grid::AutomatonState::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    