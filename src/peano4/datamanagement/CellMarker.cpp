#include "CellMarker.h"

#include "peano4/grid/TraversalObserver.h"
#include "peano4/parallel/SpacetreeSet.h"
#include "peano4/utils/Globals.h"


std::ostream& operator<<( std::ostream& out, const peano4::datamanagement::CellMarker& marker ) {
  out << marker.toString();
  return out;
}


peano4::datamanagement::CellMarker::CellMarker(
  const peano4::grid::GridTraversalEvent& event
):
  _centre(event.getX()),
  _h(event.getH()),
  _hasBeenRefined(event.getHasBeenRefined()!=0),
  _willBeRefined(event.getWillBeRefined()!=0),
  _isLocal(event.getIsCellLocal()),
  _areAllVerticesRefined( event.getWillBeRefined().all() and event.getHasBeenRefined().all() ),
  _isOneVertexHanging( false ),
  _areAllVerticesInsideDomain( event.getIsVertexAdjacentToParallelDomainBoundary().none() ),
  _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing( event.getInvokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing() )
{
  for (int i=0; i<TwoPowerD; i++) {
    _isOneVertexHanging |= event.getVertexDataTo(i)  ==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
    _isOneVertexHanging |= event.getVertexDataFrom(i)==peano4::grid::TraversalObserver::CreateOrDestroyHangingGridEntity;
  }
  _relativePositionOfCellWithinFatherCell = event.getRelativePositionToFather();
}


tarch::la::Vector<Dimensions,int>  peano4::datamanagement::CellMarker::getRelativePositionWithinFatherCell() const {
  return _relativePositionOfCellWithinFatherCell;
}


#if PeanoDebug>0
void peano4::datamanagement::CellMarker::setRelativePositionWithinFatherCell( int axis, int value ) {
  _relativePositionOfCellWithinFatherCell(axis) = value;
}
#endif


bool peano4::datamanagement::CellMarker::isContained( const tarch::la::Vector<Dimensions,double>& x ) const {
  bool result = true;
  for (int d=0; d<Dimensions; d++) {
    result &= x(d)>=_centre(d)-_h(d)/2.0;
    result &= x(d)<=_centre(d)+_h(d)/2.0;
  }
  return result;
}


bool peano4::datamanagement::CellMarker::overlaps( const tarch::la::Vector<Dimensions,double>& offset, const tarch::la::Vector<Dimensions,double>& size ) const {
  bool overlaps = true;

  for (int d=0; d<Dimensions; d++) {
    overlaps &= tarch::la::smallerEquals( _centre(d)-_h(d)/2.0, offset(d)+size(d) );
    overlaps &= tarch::la::greaterEquals( _centre(d)+_h(d)/2.0, offset(d) );
  }

  return overlaps;
}


bool peano4::datamanagement::CellMarker::willBeRefined() const {
  return _willBeRefined;
}


bool peano4::datamanagement::CellMarker::hasBeenRefined() const {
  return _hasBeenRefined;
}


bool peano4::datamanagement::CellMarker::isLocal() const {
  return _isLocal;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::x() const {
  return _centre;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::h() const {
  return _h;
}


tarch::la::Vector<Dimensions,double>  peano4::datamanagement::CellMarker::getOffset() const {
  return _centre-0.5*_h;
}


std::string peano4::datamanagement::CellMarker::toString() const {
  std::ostringstream msg;
  msg << "(x=" << _centre
      << ",h=" << _h
      << ",has-been-refined=" << _hasBeenRefined
      << ",will-be-refined=" << _willBeRefined
      << ",is-local=" << _isLocal
      << ",all-vertices-refined=" << _areAllVerticesRefined
      << ",one-vertex-hanging=" << _isOneVertexHanging
      << ",all-vertices-inside-domain=" << _areAllVerticesInsideDomain
      << ",no-lb=" << _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing
      << ",rel-pos=" << _relativePositionOfCellWithinFatherCell
      << ")";
  return msg.str();
}


bool peano4::datamanagement::CellMarker::isEnclaveCell() const {
  bool isRefined = _hasBeenRefined and _willBeRefined;
  return _invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing
     and _areAllVerticesInsideDomain
     and not _isOneVertexHanging
     and (not isRefined or _areAllVerticesRefined);
}


bool peano4::datamanagement::CellMarker::isSkeletonCell() const {
  return not isEnclaveCell();
}



#ifdef Parallel
MPI_Datatype peano4::datamanagement::CellMarker::Datatype;


void peano4::datamanagement::CellMarker::sendAndPollDanglingMessages(const peano4::datamanagement::CellMarker& message, int destination, int tag, MPI_Comm communicator  ) {
  peano4::datamanagement::CellMarker::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::datamanagement::CellMarker", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::datamanagement::CellMarker", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void peano4::datamanagement::CellMarker::receiveAndPollDanglingMessages(peano4::datamanagement::CellMarker& message, int source, int tag, MPI_Comm communicator ) {
  peano4::datamanagement::CellMarker::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "peano4::datamanagement::CellMarker", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "peano4::datamanagement::CellMarker", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}



void peano4::datamanagement::CellMarker::initDatatype() {
  peano4::grid::GridTraversalEvent dummyEvent;

  peano4::datamanagement::CellMarker  instances[] = { peano4::datamanagement::CellMarker(dummyEvent), peano4::datamanagement::CellMarker(dummyEvent) };

  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_BYTE, MPI_BYTE, MPI_BYTE, MPI_BYTE, MPI_BYTE, MPI_BYTE, MPI_BYTE, MPI_INT };

  int blocklen[] = { Dimensions, Dimensions, 1, 1, 1, 1, 1, 1, Dimensions };

  const int NumberOfAttributes = 9;

  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );
  MPI_Aint  disp[ NumberOfAttributes ];
  int       currentAddress = 0;
  MPI_Get_address( &(instances[0]._centre), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._h), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._hasBeenRefined), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._willBeRefined), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isLocal), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._areAllVerticesRefined), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._isOneVertexHanging), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._areAllVerticesInsideDomain), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._invokingSpacetreeIsNotInvolvedInAnyDynamicLoadBalancing), &disp[currentAddress] );
  currentAddress++;
  MPI_Get_address( &(instances[0]._relativePositionOfCellWithinFatherCell), &disp[currentAddress] );
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


void peano4::datamanagement::CellMarker::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void peano4::datamanagement::CellMarker::send(const peano4::datamanagement::CellMarker& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void peano4::datamanagement::CellMarker::receive(peano4::datamanagement::CellMarker& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  //buffer._senderDestinationRank = status.MPI_SOURCE;
}


void peano4::datamanagement::CellMarker::send(const peano4::datamanagement::CellMarker& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  while (!flag) {
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  }
}


void peano4::datamanagement::CellMarker::receive(peano4::datamanagement::CellMarker& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle );
  MPI_Test( &receiveRequestHandle, &flag, &status );
  while (!flag) {
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status );
  }
  //buffer._senderDestinationRank = status.MPI_SOURCE;
}

#endif
