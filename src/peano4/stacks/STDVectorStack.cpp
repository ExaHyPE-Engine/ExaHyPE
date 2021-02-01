#include "STDVectorStack.h"


template <>
void peano4::stacks::STDVectorStack<double>::startSend(int rank, int tag) {
  #ifdef Parallel
  assertion( _ioMode==IOMode::None );
  _ioMode = IOMode::MPISend;
  _ioTag  = tag;
  _ioRank = rank;

  assertion( _ioMPIRequest == nullptr );
  _ioMPIRequest = new MPI_Request;
  int result = MPI_Isend( _data.data(), _currentElement, MPI_DOUBLE, _ioRank, _ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _ioMPIRequest);
  if  (result!=MPI_SUCCESS) {
    std::ostringstream msg;
    logError( "startSend(int,int)", "was not able to send to node " << rank << " on tag " << tag
      << ": " << tarch::mpi::MPIReturnValueToString(result)
    );
  }
  #endif
}


template <>
void peano4::stacks::STDVectorStack<double>::startReceive(int rank, int tag, int numberOfElements) {
  #ifdef Parallel
  logTraceInWith3Arguments( "startReceive(int,int,int)", rank, tag, numberOfElements );
  assertion3( _ioMode==IOMode::None, rank, tag, numberOfElements );
  assertion3( numberOfElements>0, rank, tag, numberOfElements );
  _ioMode = IOMode::MPIReceive;
  _ioTag  = tag;
  _ioRank = rank;

  _data.resize(numberOfElements);
  _currentElement = numberOfElements;

  assertion3( _ioMPIRequest == nullptr, rank, tag, numberOfElements );
  _ioMPIRequest = new MPI_Request;
  assertion(_data.size()>0);
  int result = MPI_Irecv( _data.data(), _data.size(), MPI_DOUBLE, _ioRank, _ioTag, tarch::mpi::Rank::getInstance().getCommunicator(), _ioMPIRequest);
  if  (result!=MPI_SUCCESS) {
    logError( "startReceive(int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
      << ": " << tarch::mpi::MPIReturnValueToString(result)
    );
  }
  logTraceOut( "startReceive(int,int,int)" );
  #endif
}


std::string toString( peano4::stacks::IOMode mode ) {
  switch (mode) {
    case peano4::stacks::IOMode::MPIReceive:
      return "receive";
    case peano4::stacks::IOMode::MPISend:
      return "send";
    case peano4::stacks::IOMode::None:
      return "none";
  }
  return "<undef>";
}

