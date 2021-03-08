#include "{{ CLASSNAME }}.h"
#include "tarch/multicore/Lock.h"

{% macro fullyQualified(delim) -%}
{{NAMESPACE | join(delim)}}{{ delim }}{{CLASSNAME}}
{%- endmacro -%}

#include <sstream>
#include <algorithm>


{{ fullyQualified("::") }}::{{ CLASSNAME }}(tarch::la::Vector<Dimensions,double>  __debugX, tarch::la::Vector<Dimensions,double>  __debugH, int  __numTasksRemaining):
    _debugX(__debugX)
  , _debugH(__debugH)
  , _numTasksRemaining(__numTasksRemaining)
  {}



std::string {{ fullyQualified("::") }}::toString() const {
  std::ostringstream out;
  out << "(";
  out << "debugX=" << _debugX;
  out << ",";
  out << "debugH=" << _debugH;
  out << ",";
  out << "numTasksRemaining=" << _numTasksRemaining;
  out << ")";
  return out.str();
}


tarch::la::Vector<Dimensions,double>   {{ fullyQualified("::") }}::getDebugX() const {
  return _debugX;
}


void   {{ fullyQualified("::") }}::setDebugX(const tarch::la::Vector<Dimensions,double>& value) {
  _debugX = value;
}


double   {{ fullyQualified("::") }}::getDebugX(int index) const {
  return _debugX(index);
}


void   {{ fullyQualified("::") }}::setDebugX(int index, double value) {
  _debugX(index) = value;
}


tarch::la::Vector<Dimensions,double>   {{ fullyQualified("::") }}::getDebugH() const {
  return _debugH;
}


void   {{ fullyQualified("::") }}::setDebugH(const tarch::la::Vector<Dimensions,double>& value) {
  _debugH = value;
}


double   {{ fullyQualified("::") }}::getDebugH(int index) const {
  return _debugH(index);
}


void   {{ fullyQualified("::") }}::setDebugH(int index, double value) {
  _debugH(index) = value;
}


int   {{ fullyQualified("::") }}::getNumTasksRemaining() const {
  return _numTasksRemaining;
}

void   {{ fullyQualified("::") }}::setNumTasksRemaining(int value) {
  _numTasksRemaining = value;
}



int {{ fullyQualified("::") }}::addTask() {
  _numTasksRemaining++;
  return _numTasksRemaining;
}

int {{ fullyQualified("::") }}::removeTask() {
  assertion(_numTasksRemaining > 0);
  _numTasksRemaining--;
  return _numTasksRemaining;
}



#ifdef Parallel

MPI_Datatype {{ fullyQualified("::") }}::Datatype;

int {{ fullyQualified("::") }}::getSenderRank() const {
  return _senderDestinationRank;
}


void {{ fullyQualified("::") }}::initDatatype() {
  {{ fullyQualified("::") }}  instances[2];

  MPI_Datatype subtypes[] = { MPI_DOUBLE, MPI_DOUBLE, MPI_INT };

  int blocklen[] = { Dimensions, Dimensions, 1 };

  const int NumberOfAttributes = 3;

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
  MPI_Get_address( &(instances[0]._numTasksRemaining), &disp[currentAddress] );
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


void {{ fullyQualified("::") }}::shutdownDatatype() {
  MPI_Type_free( &Datatype );
}


void {{ fullyQualified("::") }}::send(const {{ fullyQualified("::") }}& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void {{ fullyQualified("::") }}::receive({{ fullyQualified("::") }}& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void {{ fullyQualified("::") }}::send(const {{ fullyQualified("::") }}& buffer, int destination, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  while (!flag) {
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  }
}


void {{ fullyQualified("::") }}::receive({{ fullyQualified("::") }}& buffer, int source, int tag, std::function<void()> waitFunctor, MPI_Comm communicator ) {
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
void {{ fullyQualified("::") }}::sendAndPollDanglingMessages(const {{ fullyQualified("::") }}& message, int destination, int tag ) {
  {{ fullyQualified("::") }}::send(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "{{ fullyQualified("::") }}", "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "{{ fullyQualified("::") }}", "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void {{ fullyQualified("::") }}::receiveAndPollDanglingMessages({{ fullyQualified("::") }}& message, int source, int tag ) {
  {{ fullyQualified("::") }}::receive(
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
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( "{{ fullyQualified("::") }}", "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "{{ fullyQualified("::") }}", "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif

void {{ fullyQualified("::") }}::merge(const {{ fullyQualified("::") }}& neighbour, const peano4::datamanagement::VertexMarker& marker) {
}


bool {{ fullyQualified("::") }}::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{ fullyQualified("::") }}::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{ fullyQualified("::") }}::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool {{ fullyQualified("::") }}::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}
