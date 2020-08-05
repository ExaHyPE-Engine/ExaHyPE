#include "AutomatonState.h"



#include <sstream>
#include <algorithm>



peano4::grid::AutomatonState::AutomatonState(int  __level, tarch::la::Vector<Dimensions,double>  __x, tarch::la::Vector<Dimensions,double>  __h, bool  __inverted, std::bitset<Dimensions>  __evenFlags, tarch::la::Vector<DimensionsTimesTwo,int>  __accessNumber):
    _level(__level)
  , _x(__x)
  , _h(__h)
  , _inverted(__inverted)
  , _evenFlags(__evenFlags)
  , _accessNumber(__accessNumber)
  {}



std::string peano4::grid::AutomatonState::toString() const {
  std::ostringstream out;
  out << "(";
  out << "level=" << _level;
  out << ","; 
  out << "x=" << _x;
  out << ","; 
  out << "h=" << _h;
  out << ","; 
  out << "inverted=" << _inverted;
  out << ","; 
  out << "evenFlags=" << _evenFlags;
  out << ","; 
  out << "accessNumber=" << _accessNumber;
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
  return _x;
}


void   peano4::grid::AutomatonState::setX(const tarch::la::Vector<Dimensions,double>& value) {
  _x = value;
}


double   peano4::grid::AutomatonState::getX(int index) const {
  return _x(index);
}


void   peano4::grid::AutomatonState::setX(int index, double value) {
  _x(index) = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::AutomatonState::getH() const {
  return _h;
}


void   peano4::grid::AutomatonState::setH(const tarch::la::Vector<Dimensions,double>& value) {
  _h = value;
}


double   peano4::grid::AutomatonState::getH(int index) const {
  return _h(index);
}


void   peano4::grid::AutomatonState::setH(int index, double value) {
  _h(index) = value;
}


bool   peano4::grid::AutomatonState::getInverted() const {
  return _inverted;
}


void   peano4::grid::AutomatonState::setInverted(bool value) {
  _inverted = value;
}


std::bitset<Dimensions>   peano4::grid::AutomatonState::getEvenFlags() const {
  return _evenFlags;
}


void   peano4::grid::AutomatonState::setEvenFlags(const std::bitset<Dimensions>&  value) {
  _evenFlags = value;
}


bool   peano4::grid::AutomatonState::getEvenFlags(int index) const {
  return _evenFlags[index];
}


void   peano4::grid::AutomatonState::setEvenFlags(int index, bool value) {
  _evenFlags[index] = value;
}


void   peano4::grid::AutomatonState::flipEvenFlags(int index) {
  _evenFlags.flip(index);
}


tarch::la::Vector<DimensionsTimesTwo,int>   peano4::grid::AutomatonState::getAccessNumber() const {
  return _accessNumber;
}


void   peano4::grid::AutomatonState::setAccessNumber(const tarch::la::Vector<DimensionsTimesTwo,int>& value) {
  _accessNumber = value;
}


int   peano4::grid::AutomatonState::getAccessNumber(int index) const {
  return _accessNumber(index);
}


void   peano4::grid::AutomatonState::setAccessNumber(int index, int value) {
  _accessNumber(index) = value;
}






#ifdef Parallel
void peano4::grid::AutomatonState::sendAndPollDanglingMessages(const peano4::grid::AutomatonState& message, int destination, int tag ) {
  peano4::grid::AutomatonState::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::AutomatonState", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::AutomatonState", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::grid::AutomatonState::receiveAndPollDanglingMessages(peano4::grid::AutomatonState& message, int source, int tag ) {
  peano4::grid::AutomatonState::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::AutomatonState", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::grid::AutomatonState::Datatype;

int peano4::grid::AutomatonState::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::AutomatonState::initDatatype() {
  peano4::grid::AutomatonState  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_BYTE, MPI_UNSIGNED_LONG, MPI_INT };
    
  int blocklen[] = { 1, Dimensions, Dimensions, 1, 1, DimensionsTimesTwo };

  const int NumberOfAttributes = 6;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._level), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._inverted), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._evenFlags), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._accessNumber.data()[0]), &disp[currentAddress] );
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


void peano4::grid::AutomatonState::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::grid::AutomatonState::send(const peano4::grid::AutomatonState& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::AutomatonState::receive(peano4::grid::AutomatonState& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::AutomatonState::send(const peano4::grid::AutomatonState& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::grid::AutomatonState::receive(peano4::grid::AutomatonState& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
