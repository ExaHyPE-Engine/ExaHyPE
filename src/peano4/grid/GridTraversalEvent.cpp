#include "GridTraversalEvent.h"



#include <sstream>
#include <algorithm>



peano4::grid::GridTraversalEvent::GridTraversalEvent(tarch::la::Vector<Dimensions,double>  __x, tarch::la::Vector<Dimensions,double>  __h, std::bitset<TwoPowerD>  __isRefined, std::bitset<TwoPowerD>  __isVertexLocal, std::bitset<TwoTimesD>  __isFaceLocal, bool  __isCellLocal, std::bitset<TwoPowerD>  __isVertexInsideDomain, tarch::la::Vector<TwoPowerD,int>  __vertexDataFrom, tarch::la::Vector<TwoPowerD,int>  __vertexDataTo, tarch::la::Vector<TwoTimesD,int>  __faceDataFrom, tarch::la::Vector<TwoTimesD,int>  __faceDataTo, int  __cellData, tarch::la::Vector<Dimensions,int>  __relativePositionToFather, int  __invokingSpacetree, int  __invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing):
    _x(__x)
  , _h(__h)
  , _isRefined(__isRefined)
  , _isVertexLocal(__isVertexLocal)
  , _isFaceLocal(__isFaceLocal)
  , _isCellLocal(__isCellLocal)
  , _isVertexInsideDomain(__isVertexInsideDomain)
  , _vertexDataFrom(__vertexDataFrom)
  , _vertexDataTo(__vertexDataTo)
  , _faceDataFrom(__faceDataFrom)
  , _faceDataTo(__faceDataTo)
  , _cellData(__cellData)
  , _relativePositionToFather(__relativePositionToFather)
  , _invokingSpacetree(__invokingSpacetree)
  , _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(__invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing)
  {}



std::string peano4::grid::GridTraversalEvent::toString() const {
  std::ostringstream out;
  out << "(";
  out << "x=" << _x;
  out << ","; 
  out << "h=" << _h;
  out << ","; 
  out << "isRefined=" << _isRefined;
  out << ","; 
  out << "isVertexLocal=" << _isVertexLocal;
  out << ","; 
  out << "isFaceLocal=" << _isFaceLocal;
  out << ","; 
  out << "isCellLocal=" << _isCellLocal;
  out << ","; 
  out << "isVertexInsideDomain=" << _isVertexInsideDomain;
  out << ","; 
  out << "vertexDataFrom=" << _vertexDataFrom;
  out << ","; 
  out << "vertexDataTo=" << _vertexDataTo;
  out << ","; 
  out << "faceDataFrom=" << _faceDataFrom;
  out << ","; 
  out << "faceDataTo=" << _faceDataTo;
  out << ","; 
  out << "cellData=" << _cellData;
  out << ","; 
  out << "relativePositionToFather=" << _relativePositionToFather;
  out << ","; 
  out << "invokingSpacetree=" << _invokingSpacetree;
  out << ","; 
  out << "invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing=" << _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
  out << ")";
  return out.str();
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridTraversalEvent::getX() const {
  return _x;
}


void   peano4::grid::GridTraversalEvent::setX(const tarch::la::Vector<Dimensions,double>& value) {
  _x = value;
}


double   peano4::grid::GridTraversalEvent::getX(int index) const {
  return _x(index);
}


void   peano4::grid::GridTraversalEvent::setX(int index, double value) {
  _x(index) = value;
}


tarch::la::Vector<Dimensions,double>   peano4::grid::GridTraversalEvent::getH() const {
  return _h;
}


void   peano4::grid::GridTraversalEvent::setH(const tarch::la::Vector<Dimensions,double>& value) {
  _h = value;
}


double   peano4::grid::GridTraversalEvent::getH(int index) const {
  return _h(index);
}


void   peano4::grid::GridTraversalEvent::setH(int index, double value) {
  _h(index) = value;
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsRefined() const {
  return _isRefined;
}


void   peano4::grid::GridTraversalEvent::setIsRefined(const std::bitset<TwoPowerD>&  value) {
  _isRefined = value;
}


bool   peano4::grid::GridTraversalEvent::getIsRefined(int index) const {
  return _isRefined[index];
}


void   peano4::grid::GridTraversalEvent::setIsRefined(int index, bool value) {
  _isRefined[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsRefined(int index) {
  _isRefined.flip(index);
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsVertexLocal() const {
  return _isVertexLocal;
}


void   peano4::grid::GridTraversalEvent::setIsVertexLocal(const std::bitset<TwoPowerD>&  value) {
  _isVertexLocal = value;
}


bool   peano4::grid::GridTraversalEvent::getIsVertexLocal(int index) const {
  return _isVertexLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsVertexLocal(int index, bool value) {
  _isVertexLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsVertexLocal(int index) {
  _isVertexLocal.flip(index);
}


std::bitset<TwoTimesD>   peano4::grid::GridTraversalEvent::getIsFaceLocal() const {
  return _isFaceLocal;
}


void   peano4::grid::GridTraversalEvent::setIsFaceLocal(const std::bitset<TwoTimesD>&  value) {
  _isFaceLocal = value;
}


bool   peano4::grid::GridTraversalEvent::getIsFaceLocal(int index) const {
  return _isFaceLocal[index];
}


void   peano4::grid::GridTraversalEvent::setIsFaceLocal(int index, bool value) {
  _isFaceLocal[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsFaceLocal(int index) {
  _isFaceLocal.flip(index);
}


bool   peano4::grid::GridTraversalEvent::getIsCellLocal() const {
  return _isCellLocal;
}


void   peano4::grid::GridTraversalEvent::setIsCellLocal(bool value) {
  _isCellLocal = value;
}


std::bitset<TwoPowerD>   peano4::grid::GridTraversalEvent::getIsVertexInsideDomain() const {
  return _isVertexInsideDomain;
}


void   peano4::grid::GridTraversalEvent::setIsVertexInsideDomain(const std::bitset<TwoPowerD>&  value) {
  _isVertexInsideDomain = value;
}


bool   peano4::grid::GridTraversalEvent::getIsVertexInsideDomain(int index) const {
  return _isVertexInsideDomain[index];
}


void   peano4::grid::GridTraversalEvent::setIsVertexInsideDomain(int index, bool value) {
  _isVertexInsideDomain[index] = value;
}


void   peano4::grid::GridTraversalEvent::flipIsVertexInsideDomain(int index) {
  _isVertexInsideDomain.flip(index);
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridTraversalEvent::getVertexDataFrom() const {
  return _vertexDataFrom;
}


void   peano4::grid::GridTraversalEvent::setVertexDataFrom(const tarch::la::Vector<TwoPowerD,int>& value) {
  _vertexDataFrom = value;
}


int   peano4::grid::GridTraversalEvent::getVertexDataFrom(int index) const {
  return _vertexDataFrom(index);
}


void   peano4::grid::GridTraversalEvent::setVertexDataFrom(int index, int value) {
  _vertexDataFrom(index) = value;
}


tarch::la::Vector<TwoPowerD,int>   peano4::grid::GridTraversalEvent::getVertexDataTo() const {
  return _vertexDataTo;
}


void   peano4::grid::GridTraversalEvent::setVertexDataTo(const tarch::la::Vector<TwoPowerD,int>& value) {
  _vertexDataTo = value;
}


int   peano4::grid::GridTraversalEvent::getVertexDataTo(int index) const {
  return _vertexDataTo(index);
}


void   peano4::grid::GridTraversalEvent::setVertexDataTo(int index, int value) {
  _vertexDataTo(index) = value;
}


tarch::la::Vector<TwoTimesD,int>   peano4::grid::GridTraversalEvent::getFaceDataFrom() const {
  return _faceDataFrom;
}


void   peano4::grid::GridTraversalEvent::setFaceDataFrom(const tarch::la::Vector<TwoTimesD,int>& value) {
  _faceDataFrom = value;
}


int   peano4::grid::GridTraversalEvent::getFaceDataFrom(int index) const {
  return _faceDataFrom(index);
}


void   peano4::grid::GridTraversalEvent::setFaceDataFrom(int index, int value) {
  _faceDataFrom(index) = value;
}


tarch::la::Vector<TwoTimesD,int>   peano4::grid::GridTraversalEvent::getFaceDataTo() const {
  return _faceDataTo;
}


void   peano4::grid::GridTraversalEvent::setFaceDataTo(const tarch::la::Vector<TwoTimesD,int>& value) {
  _faceDataTo = value;
}


int   peano4::grid::GridTraversalEvent::getFaceDataTo(int index) const {
  return _faceDataTo(index);
}


void   peano4::grid::GridTraversalEvent::setFaceDataTo(int index, int value) {
  _faceDataTo(index) = value;
}


int   peano4::grid::GridTraversalEvent::getCellData() const {
  return _cellData;
}


void   peano4::grid::GridTraversalEvent::setCellData(int value) {
  _cellData = value;
}


tarch::la::Vector<Dimensions,int>   peano4::grid::GridTraversalEvent::getRelativePositionToFather() const {
  return _relativePositionToFather;
}


void   peano4::grid::GridTraversalEvent::setRelativePositionToFather(const tarch::la::Vector<Dimensions,int>& value) {
  _relativePositionToFather = value;
}


int   peano4::grid::GridTraversalEvent::getRelativePositionToFather(int index) const {
  return _relativePositionToFather(index);
}


void   peano4::grid::GridTraversalEvent::setRelativePositionToFather(int index, int value) {
  _relativePositionToFather(index) = value;
}


int   peano4::grid::GridTraversalEvent::getInvokingSpacetree() const {
  return _invokingSpacetree;
}


void   peano4::grid::GridTraversalEvent::setInvokingSpacetree(int value) {
  _invokingSpacetree = value;
}


int   peano4::grid::GridTraversalEvent::getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() const {
  return _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing;
}


void   peano4::grid::GridTraversalEvent::setInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing(int value) {
  _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing = value;
}






#ifdef Parallel
void peano4::grid::GridTraversalEvent::sendAndPollDanglingMessages(const peano4::grid::GridTraversalEvent& message, int destination, int tag ) {
  peano4::grid::GridTraversalEvent::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridTraversalEvent", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void peano4::grid::GridTraversalEvent::receiveAndPollDanglingMessages(peano4::grid::GridTraversalEvent& message, int source, int tag ) {
  peano4::grid::GridTraversalEvent::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::grid::GridTraversalEvent", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    
#ifdef Parallel

MPI_Datatype peano4::grid::GridTraversalEvent::Datatype;

int peano4::grid::GridTraversalEvent::getSenderRank() const {
  return _senderDestinationRank;
}


void peano4::grid::GridTraversalEvent::initDatatype() {
  peano4::grid::GridTraversalEvent  instances[2];
    
  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_BYTE, MPI_UNSIGNED_LONG, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT };
    
  int blocklen[] = { Dimensions, Dimensions, 1, 1, 1, 1, 1, TwoPowerD, TwoPowerD, TwoTimesD, TwoTimesD, 1, Dimensions, 1, 1 };

  const int NumberOfAttributes = 15;
    
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._x.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._h.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isRefined), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isVertexLocal), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isFaceLocal), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isCellLocal), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isVertexInsideDomain), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._vertexDataFrom.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._vertexDataTo.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._faceDataFrom.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._faceDataTo.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._cellData), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._relativePositionToFather.data()[0]), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._invokingSpacetree), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing), &disp[currentAddress] );
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


void peano4::grid::GridTraversalEvent::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::grid::GridTraversalEvent::send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::grid::GridTraversalEvent::receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::grid::GridTraversalEvent::send(const peano4::grid::GridTraversalEvent& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle; 
  int         flag = 0; 
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle ); 
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  while (!flag) { 
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE ); 
  }
}


void peano4::grid::GridTraversalEvent::receive(peano4::grid::GridTraversalEvent& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {   
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
