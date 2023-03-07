#include "euler2DFaceLabel.h"



#include <sstream>
#include <algorithm>



examples::exahype2::euler2D::facedata::euler2DFaceLabel::euler2DFaceLabel(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, bool  __Boundary, std::bitset<2>  __Updated, tarch::la::Vector<2,double>  __UpdatedTimeStamp, tarch::la::Vector<2,double>  __NewTimeStamp, tarch::la::Vector<2,double>  __OldTimeStamp){
#if PeanoDebug>0
setDebugX( __debugX);
#endif 
#if PeanoDebug>0
setDebugH( __debugH);
#endif 
setBoundary( __Boundary);
setUpdated( __Updated);
setUpdatedTimeStamp( __UpdatedTimeStamp);
setNewTimeStamp( __NewTimeStamp);
setOldTimeStamp( __OldTimeStamp);
}



examples::exahype2::euler2D::facedata::euler2DFaceLabel::euler2DFaceLabel( const euler2DFaceLabel& copy ) {
#if PeanoDebug>0
  setDebugX( copy.getDebugX() );
#endif 
#if PeanoDebug>0
  setDebugH( copy.getDebugH() );
#endif 
  setBoundary( copy.getBoundary() );
  setUpdated( copy.getUpdated() );
  setUpdatedTimeStamp( copy.getUpdatedTimeStamp() );
  setNewTimeStamp( copy.getNewTimeStamp() );
  setOldTimeStamp( copy.getOldTimeStamp() );
}





std::string examples::exahype2::euler2D::facedata::euler2DFaceLabel::toString() const {
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
  out << "Boundary=" << _Boundary;
  out << ","; 
  out << "Updated=" << getUpdated();
  out << ","; 
  out << "UpdatedTimeStamp=" << _UpdatedTimeStamp;
  out << ","; 
  out << "NewTimeStamp=" << _NewTimeStamp;
  out << ","; 
  out << "OldTimeStamp=" << _OldTimeStamp;
  out << ")";
  return out.str();
}





#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getDebugX() const {
  return   _debugX;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getDebugX(int index) const {
  return   _debugX(index);
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setDebugX(int index, double value) {
  _debugX(index) = value;
}


#endif 


#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getDebugH() const {
  return   _debugH;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getDebugH(int index) const {
  return   _debugH(index);
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setDebugH(int index, double value) {
  _debugH(index) = value;
}


#endif 


bool   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getBoundary() const {
  return _Boundary;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setBoundary(bool value) {
  _Boundary = value;
}


std::bitset<2>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getUpdated() const {

  std::bitset<2> result;
  for (int i=0; i<2; i++) result[i] =   _Updated[i];
  return result;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setUpdated(const std::bitset<2>&  value) {

  for (int i=0; i<2; i++)   _Updated[i]=value[i];
}


bool   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getUpdated(int index) const {
  return   _Updated[index];
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setUpdated(int index, bool value) {
  _Updated[index] = value;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::flipUpdated(int index) {
  _Updated[index] = not   _Updated[index];
}


tarch::la::Vector<2,double>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getUpdatedTimeStamp() const {
  return   _UpdatedTimeStamp;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setUpdatedTimeStamp(const tarch::la::Vector<2,double>& value) {
  _UpdatedTimeStamp = value;
}


double   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getUpdatedTimeStamp(int index) const {
  return   _UpdatedTimeStamp(index);
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setUpdatedTimeStamp(int index, double value) {
  _UpdatedTimeStamp(index) = value;
}


tarch::la::Vector<2,double>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getNewTimeStamp() const {
  return   _NewTimeStamp;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setNewTimeStamp(const tarch::la::Vector<2,double>& value) {
  _NewTimeStamp = value;
}


double   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getNewTimeStamp(int index) const {
  return   _NewTimeStamp(index);
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setNewTimeStamp(int index, double value) {
  _NewTimeStamp(index) = value;
}


tarch::la::Vector<2,double>   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getOldTimeStamp() const {
  return   _OldTimeStamp;
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setOldTimeStamp(const tarch::la::Vector<2,double>& value) {
  _OldTimeStamp = value;
}


double   examples::exahype2::euler2D::facedata::euler2DFaceLabel::getOldTimeStamp(int index) const {
  return   _OldTimeStamp(index);
}


void   examples::exahype2::euler2D::facedata::euler2DFaceLabel::setOldTimeStamp(int index, double value) {
  _OldTimeStamp(index) = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)      
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::Datatype;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype examples::exahype2::euler2D::facedata::euler2DFaceLabel::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


int examples::exahype2::euler2D::facedata::euler2DFaceLabel::getSenderRank() const {
  return _senderDestinationRank;
}


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  examples::exahype2::euler2D::facedata::euler2DFaceLabel  instances[2];

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
  subtypes[counter] = MPI_BYTE;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_UNSIGNED_LONG;
  blocklen[counter] = 1;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 2;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 2;
  counter++;
  subtypes[counter] = MPI_DOUBLE;
  blocklen[counter] = 2;
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
  MPI_Get_address( &(instances[0]._Boundary), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._Updated), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._UpdatedTimeStamp.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._NewTimeStamp.data()[0]), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._OldTimeStamp.data()[0]), &disp[counter] );
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


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::shutdownDatatype() {
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


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::send(const examples::exahype2::euler2D::facedata::euler2DFaceLabel& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::receive(examples::exahype2::euler2D::facedata::euler2DFaceLabel& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::send(
  const examples::exahype2::euler2D::facedata::euler2DFaceLabel& buffer, 
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


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::receive(
  examples::exahype2::euler2D::facedata::euler2DFaceLabel& buffer, 
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
void examples::exahype2::euler2D::facedata::euler2DFaceLabel::sendAndPollDanglingMessages(const examples::exahype2::euler2D::facedata::euler2DFaceLabel& message, int destination, int tag, MPI_Comm communicator ) {
  examples::exahype2::euler2D::facedata::euler2DFaceLabel::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler2D::facedata::euler2DFaceLabel", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler2D::facedata::euler2DFaceLabel", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void examples::exahype2::euler2D::facedata::euler2DFaceLabel::receiveAndPollDanglingMessages(examples::exahype2::euler2D::facedata::euler2DFaceLabel& message, int source, int tag, MPI_Comm communicator ) {
  examples::exahype2::euler2D::facedata::euler2DFaceLabel::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::exahype2::euler2D::facedata::euler2DFaceLabel", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::exahype2::euler2D::facedata::euler2DFaceLabel", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    
void examples::exahype2::euler2D::facedata::euler2DFaceLabel::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const examples::exahype2::euler2D::facedata::euler2DFaceLabel& neighbour, const peano4::datamanagement::FaceMarker& marker) {
  
  switch (context) {
    case ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange:
    case ::peano4::grid::TraversalObserver::SendReceiveContext::PeriodicBoundaryDataSwap:
      {
        _Boundary = _Boundary or neighbour._Boundary;
  
        const int normal         = marker.getSelectedFaceNumber() % Dimensions;
        const int neighbourEntry = marker.outerNormal()(normal)<0.0 ? 0 : 1;
  
        _Updated[ neighbourEntry ]          = neighbour._Updated[ neighbourEntry ];
        _UpdatedTimeStamp( neighbourEntry ) = neighbour._UpdatedTimeStamp( neighbourEntry );
        _NewTimeStamp( neighbourEntry )     = neighbour._NewTimeStamp( neighbourEntry );
        _OldTimeStamp( neighbourEntry )     = neighbour._OldTimeStamp( neighbourEntry );
      }
      break;
    default:
      assertionMsg( false, "not implemented yet" );
  }

}


bool examples::exahype2::euler2D::facedata::euler2DFaceLabel::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::facedata::euler2DFaceLabel::send(
  const peano4::datamanagement::FaceMarker& marker
  
) const {
  return true;
}


bool examples::exahype2::euler2D::facedata::euler2DFaceLabel::loadPersistently( 
  const peano4::datamanagement::FaceMarker& marker 
  
) {
  return true;
}


bool examples::exahype2::euler2D::facedata::euler2DFaceLabel::storePersistently( 
  const peano4::datamanagement::FaceMarker& marker 
  
) const {
  return true;
}


bool examples::exahype2::euler2D::facedata::euler2DFaceLabel::compute( 
  const peano4::datamanagement::FaceMarker& marker 
  
) const {
  return true;
}