#include "hydroPartSetCellStatistics.h"



#include <sstream>
#include <algorithm>



benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::hydroPartSetCellStatistics(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, int  __NumberOfParticles, bool  __ParentOfRefinedCell){
#if PeanoDebug>0
setDebugX( __debugX);
#endif 
#if PeanoDebug>0
setDebugH( __debugH);
#endif 
setNumberOfParticles( __NumberOfParticles);
setParentOfRefinedCell( __ParentOfRefinedCell);
}



std::string benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::toString() const {
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
  out << "NumberOfParticles=" << _NumberOfParticles;
  out << ","; 
  out << "ParentOfRefinedCell=" << _ParentOfRefinedCell;
  out << ")";
  return out.str();
}





#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getDebugX() const {
  return   _debugX;
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getDebugX(int index) const {
  return   _debugX(index);
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setDebugX(int index, double value) {
  _debugX(index) = value;
}


#endif 


#if PeanoDebug>0
tarch::la::Vector<Dimensions,double>   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getDebugH() const {
  return   _debugH;
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getDebugH(int index) const {
  return   _debugH(index);
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setDebugH(int index, double value) {
  _debugH(index) = value;
}


#endif 


int   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getNumberOfParticles() const {
  return _NumberOfParticles;
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setNumberOfParticles(int value) {
  _NumberOfParticles = value;
}


bool   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getParentOfRefinedCell() const {
  return _ParentOfRefinedCell;
}


void   benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::setParentOfRefinedCell(bool value) {
  _ParentOfRefinedCell = value;
}






#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)      
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::Datatype;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


int benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::getSenderRank() const {
  return _senderDestinationRank;
}


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics  instances[2];

  int NumberOfAttributes = 0;
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
#if PeanoDebug>0
  NumberOfAttributes++;
#endif 
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
  MPI_Get_address( &(instances[0]._NumberOfParticles), &disp[counter] );
  counter++;
  MPI_Get_address( &(instances[0]._ParentOfRefinedCell), &disp[counter] );
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


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::shutdownDatatype() {
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


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::send(const benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::receive(benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::send(
  const benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& buffer, 
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


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::receive(
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& buffer, 
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
void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::sendAndPollDanglingMessages(const benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& message, int destination, int tag, MPI_Comm communicator ) {
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::receiveAndPollDanglingMessages(benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& message, int source, int tag, MPI_Comm communicator ) {
  benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    
void benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics& neighbour, const peano4::datamanagement::CellMarker& marker) {
  
}


bool benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::send(
  const peano4::datamanagement::CellMarker& marker
  
) const {
  return true;
}


bool benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::storePersistently( 
  const peano4::datamanagement::CellMarker& marker 
  
) const {
  return false;
}


bool benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::loadPersistently( 
  const peano4::datamanagement::CellMarker& marker 
  
) {
  return false;
}


bool benchmarks::swift2::noh::celldata::hydroPartSetCellStatistics::compute( 
  const peano4::datamanagement::CellMarker& marker 
  
) const {
  return true;
}