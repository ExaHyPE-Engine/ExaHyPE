#include "euler2DCellLabel.h"



#include <sstream>
#include <algorithm>



examples::exahype2::euler2D::celldata::euler2DCellLabel::euler2DCellLabel(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, int  __SemaphoreNumber, double  __TimeStamp, double  __TimeStepSize, bool  __HasUpdated){
#if PeanoDebug>0
setDebugX( __debugX);
#endif 
#if PeanoDebug>0
setDebugH( __debugH);
#endif 
setSemaphoreNumber( __SemaphoreNumber);
setTimeStamp( __TimeStamp);
setTimeStepSize( __TimeStepSize);
setHasUpdated( __HasUpdated);
}



std::string examples::exahype2::euler2D::celldata::euler2DCellLabel::toString() const {
  std::ostringstream out;
  out << "(";
#if PeanoDebug>0
  out << "debugX=" << _debugX;
#endif 
#if PeanoDebug>0
  out << ","; 
  out << "debugH=" << _debugH;
#endif 
  out << ","; 
  out << "SemaphoreNumber=" << _SemaphoreNumber;
  out << ","; 
  out << "TimeStamp=" << _TimeStamp;
  out << ","; 
  out << "TimeStepSize=" << _TimeStepSize;
  out << ","; 
  out << "HasUpdated=" << _HasUpdated;
  out << ")";
  return out.str();
}





#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   examples::exahype2::euler2D::celldata::euler2DCellLabel::getDebugX() const {
  return   _debugX;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   examples::exahype2::euler2D::celldata::euler2DCellLabel::getDebugX(int index) const {
  return   _debugX(index);
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setDebugX(int index, double value) {
  _debugX(index) = value;
}


#endif 


#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   examples::exahype2::euler2D::celldata::euler2DCellLabel::getDebugH() const {
  return   _debugH;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   examples::exahype2::euler2D::celldata::euler2DCellLabel::getDebugH(int index) const {
  return   _debugH(index);
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setDebugH(int index, double value) {
  _debugH(index) = value;
}


#endif 


int   examples::exahype2::euler2D::celldata::euler2DCellLabel::getSemaphoreNumber() const {
  return _SemaphoreNumber;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setSemaphoreNumber(int value) {
  _SemaphoreNumber = value;
}


double   examples::exahype2::euler2D::celldata::euler2DCellLabel::getTimeStamp() const {
  return _TimeStamp;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setTimeStamp(double value) {
  _TimeStamp = value;
}


double   examples::exahype2::euler2D::celldata::euler2DCellLabel::getTimeStepSize() const {
  return _TimeStepSize;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setTimeStepSize(double value) {
  _TimeStepSize = value;
}


bool   examples::exahype2::euler2D::celldata::euler2DCellLabel::getHasUpdated() const {
  return _HasUpdated;
}


void   examples::exahype2::euler2D::celldata::euler2DCellLabel::setHasUpdated(bool value) {
  _HasUpdated = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)      
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::Datatype;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::celldata::euler2DCellLabel::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


int examples::exahype2::euler2D::celldata::euler2DCellLabel::getSenderRank() const {
  return _senderDestinationRank;
}


void examples::exahype2::euler2D::celldata::euler2DCellLabel::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  examples::exahype2::euler2D::celldata::euler2DCellLabel  instances[2];

  int NumberOfAttributes = 0;
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;
  NumberOfAttributes++;

  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes]; 
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];
  
  int counter            = 0;
#if PeanoDebug>0
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
#endif
#if PeanoDebug>0
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = Dimensions;
  counter++;
#endif
  subtypes[counter] = MPI_INT;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
   
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  
  counter = 0;

#if PeanoDebug>0
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[counter] );
  counter++;
#endif

#if PeanoDebug>0
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[counter] );
  counter++;
#endif
  MPI_Get_address( &(instances[0]._SemaphoreNumber), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._TimeStamp), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._TimeStepSize), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._HasUpdated), &disp[counter] );
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


void examples::exahype2::euler2D::celldata::euler2DCellLabel::shutdownDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  MPI_Datatype type;
  type = getForkDatatype();
  MPI_Type_free( &type );
  type = getJoinDatatype();
  MPI_Type_free( &type );
  type = getBoundaryExchangeDatatype();
  MPI_Type_free( &type );
  type = getMultiscaleDataExchangeDatatype();
  MPI_Type_free( &type );
  type = getGlobalCommunciationDatatype();
  MPI_Type_free( &type );
  #else
  MPI_Datatype type = Datatype;
  MPI_Type_free( &type );
  #endif
}


void examples::exahype2::euler2D::celldata::euler2DCellLabel::send(const examples::exahype2::euler2D::celldata::euler2DCellLabel& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void examples::exahype2::euler2D::celldata::euler2DCellLabel::receive(examples::exahype2::euler2D::celldata::euler2DCellLabel& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void examples::exahype2::euler2D::celldata::euler2DCellLabel::send(
  const examples::exahype2::euler2D::celldata::euler2DCellLabel& buffer, 
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


void examples::exahype2::euler2D::celldata::euler2DCellLabel::receive(
  examples::exahype2::euler2D::celldata::euler2DCellLabel& buffer, 
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
void examples::exahype2::euler2D::celldata::euler2DCellLabel::sendAndPollDanglingMessages(const examples::exahype2::euler2D::celldata::euler2DCellLabel& message, int destination, int tag, MPI_Comm communicator ) {
  examples::exahype2::euler2D::celldata::euler2DCellLabel::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler2D::celldata::euler2DCellLabel", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler2D::celldata::euler2DCellLabel", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void examples::exahype2::euler2D::celldata::euler2DCellLabel::receiveAndPollDanglingMessages(examples::exahype2::euler2D::celldata::euler2DCellLabel& message, int source, int tag, MPI_Comm communicator ) {
  examples::exahype2::euler2D::celldata::euler2DCellLabel::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler2D::celldata::euler2DCellLabel", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler2D::celldata::euler2DCellLabel", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    
void examples::exahype2::euler2D::celldata::euler2DCellLabel::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const examples::exahype2::euler2D::celldata::euler2DCellLabel& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
      assertionMsg( false, "cells should never be subject of a boundary exchange" );
      break;
    case ::peano4::grid::TraversalObserver::SendReceiveContext::ForkDomain:
      *this = neighbour;
      break;
    default:
      assertionMsg( false, "not implemented yet" );
  }

}


bool examples::exahype2::euler2D::celldata::euler2DCellLabel::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DCellLabel::send(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DCellLabel::storePersistently( 
  const peano4::datamanagement::CellMarker& marker 
  
) const {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DCellLabel::loadPersistently( 
  const peano4::datamanagement::CellMarker& marker 
  
) {
  return true;
}


bool examples::exahype2::euler2D::celldata::euler2DCellLabel::compute( 
  const peano4::datamanagement::CellMarker& marker 
  
) const {
  return true;
}