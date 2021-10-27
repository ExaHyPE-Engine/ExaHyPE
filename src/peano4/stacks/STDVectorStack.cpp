#include "STDVectorStack.h"


template <>
void peano4::stacks::STDVectorStack<double>::startSend(int rank, int tag, MPI_Comm comm) {
  #ifdef Parallel
  assertion( _ioMode==IOMode::None );
  _ioMode = IOMode::MPISend;
  _ioTag  = tag;
  _ioRank = rank;

  assertion( _ioMPIRequest == nullptr );
  _ioMPIRequest = new MPI_Request;
  int result = MPI_Isend( _data.data(), _currentElement, MPI_DOUBLE, _ioRank, _ioTag, comm, _ioMPIRequest);
  if  (result!=MPI_SUCCESS) {
    std::ostringstream msg;
    logError( "startSend(int,int)", "was not able to send to node " << rank << " on tag " << tag
      << ": " << tarch::mpi::MPIReturnValueToString(result)
    );
  }
  #endif
}


template <>
void peano4::stacks::STDVectorStack<double>::startReceive(int rank, int tag, MPI_Comm comm, int numberOfElements) {
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
  int result = MPI_Irecv( _data.data(), _data.size(), MPI_DOUBLE, _ioRank, _ioTag, comm, _ioMPIRequest);
  if  (result!=MPI_SUCCESS) {
    logError( "startReceive(int,int)", "was not able to receive " << numberOfElements << " values from node " << rank << " on tag " << tag
      << ": " << tarch::mpi::MPIReturnValueToString(result)
    );
  }
  logTraceOut( "startReceive(int,int,int)" );
  #endif
}

