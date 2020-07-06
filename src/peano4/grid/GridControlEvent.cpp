#include "GridControlEvent.h"



#include <sstream>
#include <algorithm>



peano4::grid::GridControlEvent::GridControlEvent(RefinementControl  __refinementControl, tarch::la::Vector<Dimensions,double>  __offset, tarch::la::Vector<Dimensions,double>  __width, tarch::la::Vector<Dimensions,double>  __h):
    _refinementControl(__refinementControl)
  , _offset(__offset)
  , _width(__width)
  , _h(__h)
  {}



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
  return _offset;
}


void   peano4::grid::GridControlEvent::setOffset(const tarch::la::Vector<Dimensions,double>& value) {
  _offset = value;
}


double   peano4::grid::GridControlEvent::getOffset(int index) const {
  return _offset(index);
}


void   peano4::grid::GridControlEvent::setOffset(int index, double value) {
  _offset(index) = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridControlEvent::getWidth() const {
  return _width;
}


void   peano4::grid::GridControlEvent::setWidth(const tarch::la::Vector<Dimensions,double>& value) {
  _width = value;
}


double   peano4::grid::GridControlEvent::getWidth(int index) const {
  return _width(index);
}


void   peano4::grid::GridControlEvent::setWidth(int index, double value) {
  _width(index) = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridControlEvent::getH() const {
  return _h;
}


void   peano4::grid::GridControlEvent::setH(const tarch::la::Vector<Dimensions,double>& value) {
  _h = value;
}


double   peano4::grid::GridControlEvent::getH(int index) const {
  return _h(index);
}


void   peano4::grid::GridControlEvent::setH(int index, double value) {
  _h(index) = value;
}






bool peano4::grid::GridControlEvent::receiveAndMerge() {
  return true;
}


bool peano4::grid::GridControlEvent::send() {
  return true;
}
    
    
#ifdef Parallel
void peano4::grid::GridControlEvent::sendAndPollDanglingMessages(const peano4::grid::GridControlEvent& message, int destination, int tag ) {
  peano4::grid::GridControlEvent::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridControlEvent", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::grid::GridControlEvent::receiveAndPollDanglingMessages(peano4::grid::GridControlEvent& message, int source, int tag ) {
  peano4::grid::GridControlEvent::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridControlEvent", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::grid::GridControlEvent::Datatype;

int peano4::grid::GridControlEvent::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::GridControlEvent::initDatatype() {
  peano4::grid::GridControlEvent  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_INT, MPI_DOUBLE, MPI_DOUBLE, MPI_DOUBLE };
    
  int blocklen[] = { 1, Dimensions, Dimensions, Dimensions };

  const int NumberOfAttributes = 4;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._refinementControl), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._offset.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._width.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[currentAddress] );
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


void peano4::grid::GridControlEvent::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::grid::GridControlEvent::send(const peano4::grid::GridControlEvent& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridControlEvent::receive(peano4::grid::GridControlEvent& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridControlEvent::send(const peano4::grid::GridControlEvent& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::grid::GridControlEvent::receive(peano4::grid::GridControlEvent& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
