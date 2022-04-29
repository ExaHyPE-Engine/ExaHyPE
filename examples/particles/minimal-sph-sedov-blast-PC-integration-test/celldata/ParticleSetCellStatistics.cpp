#include "ParticleSetCellStatistics.h"



#include <sstream>
#include <algorithm>



examples::particles::celldata::ParticleSetCellStatistics::ParticleSetCellStatistics(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, int  __NumberOfParticles, bool  __ParentOfRefinedCell):
    _debugX(__debugX)
  , _debugH(__debugH)
  , _NumberOfParticles(__NumberOfParticles)
  , _ParentOfRefinedCell(__ParentOfRefinedCell)
  {}



std::string examples::particles::celldata::ParticleSetCellStatistics::toString() const {
  std::ostringstream out;
  out << "(";
  out << "debugX=" << _debugX;
  out << ","; 
  out << "debugH=" << _debugH;
  out << ","; 
  out << "NumberOfParticles=" << _NumberOfParticles;
  out << ","; 
  out << "ParentOfRefinedCell=" << _ParentOfRefinedCell;
  out << ")";
  return out.str();
}


tarch::la::Vector<Dimensions,double>   examples::particles::celldata::ParticleSetCellStatistics::getDebugX() const {
  return   _debugX;
}


void   examples::particles::celldata::ParticleSetCellStatistics::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   examples::particles::celldata::ParticleSetCellStatistics::getDebugX(int index) const {
  return   _debugX(index);
}


void   examples::particles::celldata::ParticleSetCellStatistics::setDebugX(int index, double value) {
  _debugX(index) = value;
}


tarch::la::Vector<Dimensions,double>   examples::particles::celldata::ParticleSetCellStatistics::getDebugH() const {
  return   _debugH;
}


void   examples::particles::celldata::ParticleSetCellStatistics::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   examples::particles::celldata::ParticleSetCellStatistics::getDebugH(int index) const {
  return   _debugH(index);
}


void   examples::particles::celldata::ParticleSetCellStatistics::setDebugH(int index, double value) {
  _debugH(index) = value;
}


int   examples::particles::celldata::ParticleSetCellStatistics::getNumberOfParticles() const {
  return _NumberOfParticles;
}


void   examples::particles::celldata::ParticleSetCellStatistics::setNumberOfParticles(int value) {
  _NumberOfParticles = value;
}


bool   examples::particles::celldata::ParticleSetCellStatistics::getParentOfRefinedCell() const {
  return _ParentOfRefinedCell;
}


void   examples::particles::celldata::ParticleSetCellStatistics::setParentOfRefinedCell(bool value) {
  _ParentOfRefinedCell = value;
}






#ifdef Parallel

MPI_Datatype examples::particles::celldata::ParticleSetCellStatistics::Datatype;

int examples::particles::celldata::ParticleSetCellStatistics::getSenderRank() const {
  return _senderDestinationRank;
}


void examples::particles::celldata::ParticleSetCellStatistics::initDatatype() {
  examples::particles::celldata::ParticleSetCellStatistics  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_INT, MPI_BYTE };
    
  int blocklen[] = { Dimensions, Dimensions, 1, 1 };

  const int NumberOfAttributes = 4;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._debugX.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._debugH.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._NumberOfParticles), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._ParentOfRefinedCell), &disp[currentAddress] );
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


void examples::particles::celldata::ParticleSetCellStatistics::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void examples::particles::celldata::ParticleSetCellStatistics::send(const examples::particles::celldata::ParticleSetCellStatistics& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void examples::particles::celldata::ParticleSetCellStatistics::receive(examples::particles::celldata::ParticleSetCellStatistics& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void examples::particles::celldata::ParticleSetCellStatistics::send(const examples::particles::celldata::ParticleSetCellStatistics& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void examples::particles::celldata::ParticleSetCellStatistics::receive(examples::particles::celldata::ParticleSetCellStatistics& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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

#ifdef Parallel
void examples::particles::celldata::ParticleSetCellStatistics::sendAndPollDanglingMessages(const examples::particles::celldata::ParticleSetCellStatistics& message, int destination, int tag, MPI_Comm communicator ) {
  examples::particles::celldata::ParticleSetCellStatistics::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::particles::celldata::ParticleSetCellStatistics", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::particles::celldata::ParticleSetCellStatistics", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void examples::particles::celldata::ParticleSetCellStatistics::receiveAndPollDanglingMessages(examples::particles::celldata::ParticleSetCellStatistics& message, int source, int tag, MPI_Comm communicator ) {
  examples::particles::celldata::ParticleSetCellStatistics::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "examples::particles::celldata::ParticleSetCellStatistics", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "examples::particles::celldata::ParticleSetCellStatistics", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    
void examples::particles::celldata::ParticleSetCellStatistics::merge(const examples::particles::celldata::ParticleSetCellStatistics& neighbour, const peano4::datamanagement::CellMarker& marker) {

}


bool examples::particles::celldata::ParticleSetCellStatistics::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::particles::celldata::ParticleSetCellStatistics::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::particles::celldata::ParticleSetCellStatistics::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool examples::particles::celldata::ParticleSetCellStatistics::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
