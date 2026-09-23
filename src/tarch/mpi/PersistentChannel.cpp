// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "PersistentChannel.h"

#include "tarch/Termination.h"
#include "tarch/mpi/Diagnostics.h"

tarch::mpi::PersistentChannel::~PersistentChannel() { freeRequest(); }

tarch::mpi::PersistentChannel::PersistentChannel(tarch::mpi::PersistentChannel&& other) noexcept :
  _request(other._request),
  _buffer(other._buffer),
  _byteCount(other._byteCount),
  _rank(other._rank),
  _tag(other._tag),
  _communicator(other._communicator),
  _outgoing(other._outgoing) {
  other._request = MPI_REQUEST_NULL;
  other._buffer  = nullptr;
}

tarch::mpi::PersistentChannel& tarch::mpi::PersistentChannel::operator=(
  tarch::mpi::PersistentChannel&& other
) noexcept {
  if (this != &other) {
    freeRequest();
    _request       = other._request;
    _buffer        = other._buffer;
    _byteCount     = other._byteCount;
    _rank          = other._rank;
    _tag           = other._tag;
    _communicator  = other._communicator;
    _outgoing      = other._outgoing;
    other._request = MPI_REQUEST_NULL;
    other._buffer  = nullptr;
  }
  return *this;
}

void tarch::mpi::PersistentChannel::startSend(
  const void* buffer,
  const int   byteCount,
  const int   rank,
  const int   tag,
  MPI_Comm    communicator
) {
  start(buffer, byteCount, rank, tag, communicator, true);
}

void tarch::mpi::PersistentChannel::startReceive(
  void*     buffer,
  const int byteCount,
  const int rank,
  const int tag,
  MPI_Comm  communicator
) {
  start(buffer, byteCount, rank, tag, communicator, false);
}

void tarch::mpi::PersistentChannel::start(
  const void* buffer,
  const int   byteCount,
  const int   rank,
  const int   tag,
  MPI_Comm    communicator,
  const bool  outgoing
) {
  const bool transferUnchanged = _request != MPI_REQUEST_NULL and buffer == _buffer and byteCount == _byteCount
    and rank == _rank and tag == _tag and communicator == _communicator and outgoing == _outgoing;
  if (not transferUnchanged) {
    freeRequest();
    const int result = outgoing
      ? MPI_Send_init(buffer, byteCount, MPI_BYTE, rank, tag, communicator, &_request)
      : MPI_Recv_init(const_cast<void*>(buffer), byteCount, MPI_BYTE, rank, tag, communicator, &_request);
    if (result != MPI_SUCCESS) {
      tarch::terminateWithError(
        std::string(outgoing ? "MPI_Send_init" : "MPI_Recv_init")
          + " failed: " + tarch::mpi::mpiReturnValueToString(result),
        result
      );
    }
    _buffer       = buffer;
    _byteCount    = byteCount;
    _rank         = rank;
    _tag          = tag;
    _communicator = communicator;
    _outgoing     = outgoing;
  }

  const int result = MPI_Start(&_request);
  if (result != MPI_SUCCESS) {
    tarch::terminateWithError(std::string("MPI_Start failed: ") + tarch::mpi::mpiReturnValueToString(result), result);
  }
}

void tarch::mpi::PersistentChannel::freeRequest() {
  if (_request == MPI_REQUEST_NULL) {
    return;
  }

  // A channel owned by an object with static storage duration is destroyed
  // after MPI_Finalize, where freeing the handle is illegal; the library has
  // already released everything at that point.
  int finalised = 0;
  MPI_Finalized(&finalised);
  if (finalised == 0) {
    MPI_Request_free(&_request);
  }
  _request = MPI_REQUEST_NULL;
}
