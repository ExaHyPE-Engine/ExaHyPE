// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManager.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Handles.h"
#include "tarch/mpi/Rank.h"

peano4::communication::CommunicationManager peano4::communication::CommunicationManager::_singleton;

peano4::communication::CommunicationManager&
peano4::communication::CommunicationManager::getInstance() {
  return _singleton;
}

/**
 * Reduces the tag upper bound over all ranks, duplicates the global communicator and starts the
 * progress engine with the transfer poller. Every rank calls this function at the same time.
 */
void
peano4::communication::CommunicationManager::init() {
  int* tagUpperBound  = nullptr;
  int  attributeFound = 0;
  tarch::mpi::getAttribute(
    tarch::mpi::Rank::getInstance().getCommunicator(),
    MPI_TAG_UB,
    static_cast<void*>(&tagUpperBound),
    &attributeFound
  );
  if (attributeFound == 0 or tagUpperBound == nullptr) {
    tarch::terminateWithError("The MPI library cannot report its largest message tag.");
  }

  const int localTagUpperBound = *tagUpperBound;
  tarch::mpi::allreduce(
    &localTagUpperBound,
    &_tagUpperBound,
    1,
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  _nextTag = 0;

  tarch::mpi::duplicateCommunicator(tarch::mpi::Rank::getInstance().getCommunicator(), &_exchangeCommunicator);

  auto& progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  progressManager.registerPoller([]() { return getInstance().pollInFlightTransfers(); });
  progressManager.start();
}

/**
 * Completes every posted transfer, stops the progress engine and frees the exchange communicator.
 * The two barriers keep a rank from freeing the communicator or leaving the shutdown while a
 * neighbour still uses it.
 */
void
peano4::communication::CommunicationManager::shutdown() {
  auto& progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  progressManager.finishPendingWork();
  progressManager.stop();

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  if (_exchangeCommunicator != MPI_COMM_NULL) {
    tarch::mpi::freeCommunicator(&_exchangeCommunicator);
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

MPI_Comm
peano4::communication::CommunicationManager::getCommunicator() const {
  ASSERTION(_exchangeCommunicator != MPI_COMM_NULL);
  return _exchangeCommunicator;
}

/**
 * Returns the next unused tag of the exchange communicator. Every rank reserves its tags in the
 * same order. Both partners of an exchange get the same tag without communication.
 */
int
peano4::communication::CommunicationManager::reserveTag() {
  ASSERTION(_exchangeCommunicator != MPI_COMM_NULL);
  if (_nextTag > _tagUpperBound) {
    tarch::terminateWithError(
      "The MPI library provides message tags up to " + std::to_string(_tagUpperBound) + " only."
    );
  }
  return _nextTag++;
}

/**
 * Pushes one posted transfer onto the lock-free stack of new transfers. Several threads may push at
 * the same time. The poller takes the whole stack in one exchange.
 */
void
peano4::communication::CommunicationManager::registerInFlightTransfer(std::function<bool()> pollTransfer) {
  ASSERTION(static_cast<bool>(pollTransfer));
  auto* const transfer = new InFlightTransfer{std::move(pollTransfer), nullptr};
  transfer->next       = _newInFlightTransfers.load(std::memory_order_relaxed);
  while (not _newInFlightTransfers
               .compare_exchange_weak(transfer->next, transfer, std::memory_order_release, std::memory_order_relaxed)) {
  }
}

/**
 * Polls every in-flight transfer once and removes the completed ones. Returns whether every transfer
 * has completed.
 */
bool
peano4::communication::CommunicationManager::pollInFlightTransfers() {
  auto* newTransfers = _newInFlightTransfers.exchange(nullptr, std::memory_order_acquire);
  while (newTransfers != nullptr) {
    auto* const next = newTransfers->next;
    _transfersBeingPolled.emplace_back(newTransfers);
    newTransfers = next;
  }

  std::erase_if(_transfersBeingPolled, [](const std::unique_ptr<InFlightTransfer>& transfer) {
    return transfer->poll();
  });

  return _transfersBeingPolled.empty() and _newInFlightTransfers.load(std::memory_order_relaxed) == nullptr;
}
