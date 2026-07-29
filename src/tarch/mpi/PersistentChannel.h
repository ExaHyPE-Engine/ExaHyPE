// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

#include <mpi.h>

namespace tarch::mpi {
  class PersistentChannel;
} // namespace tarch::mpi

/**
 * One direction of a repeated point-to-point byte transfer.
 *
 * A persistent request binds buffer, count, peer, tag and communicator once and
 * is then restarted for every repetition of the same transfer, which spares MPI
 * the per-message envelope setup. The channel compares each start against the
 * transfer it has bound: an unchanged transfer costs one MPI_Start, a changed
 * one re-creates the request first. A caller therefore states the transfer it
 * wants every time and needs no notion of whether anything changed.
 *
 * A start requires the previous transfer on this channel to be complete. The
 * caller polls getRequest() with tarch::mpi::test(); a completed persistent
 * request turns inactive rather than null, and testing an inactive request
 * reports completion, so a finished channel drops out of a polling loop on its
 * own. Channels are not entered into the pending-request diagnostics table;
 * the polling loop that completes them owns the timeout reporting.
 */
class tarch::mpi::PersistentChannel final {
public:
  PersistentChannel() = default;
  ~PersistentChannel();
  PersistentChannel(const PersistentChannel&)            = delete;
  PersistentChannel& operator=(const PersistentChannel&) = delete;
  PersistentChannel(PersistentChannel&& other) noexcept;
  PersistentChannel& operator=(PersistentChannel&& other) noexcept;

  void startSend(const void* buffer, const int byteCount, const int rank, const int tag, const MPI_Comm communicator);
  void startReceive(void* buffer, const int byteCount, const int rank, const int tag, const MPI_Comm communicator);

  INLINE_METHOD MPI_Request* getRequest();

private:
  void start(
    const void*    buffer,
    const int      byteCount,
    const int      rank,
    const int      tag,
    const MPI_Comm communicator,
    const bool     outgoing
  );
  void freeRequest();

  MPI_Request _request{MPI_REQUEST_NULL};
  const void* _buffer{nullptr};
  int         _byteCount{0};
  int         _rank{-1};
  int         _tag{-1};
  MPI_Comm    _communicator{MPI_COMM_NULL};
  bool        _outgoing{false};
};

#include "PersistentChannel.cpph"
