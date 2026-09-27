// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManager.h"

#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

peano4::communication::CommunicationManager peano4::communication::CommunicationManager::_singleton;

peano4::communication::CommunicationManager&
peano4::communication::CommunicationManager::getInstance() {
  return _singleton;
}

/**
 * Determines the tag space every rank can address, creates the exchange communicator and
 * starts the progress engine with the transfer poller. Collective over all ranks.
 */
void
peano4::communication::CommunicationManager::init() {
  int*      tagUpperBound   = nullptr;
  int       attributeFound  = 0;
  const int attributeResult = MPI_Comm_get_attr(
    tarch::mpi::Rank::getInstance().getCommunicator(),
    MPI_TAG_UB,
    static_cast<void*>(&tagUpperBound),
    &attributeFound
  );
  if (attributeResult != MPI_SUCCESS or attributeFound == 0 or tagUpperBound == nullptr) {
    tarch::terminateWithError("Could not determine MPI_TAG_UB for the global communicator.");
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

  if (MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_exchangeCommunicator) != MPI_SUCCESS) {
    tarch::terminateWithError("Could not create the exchange communicator.");
  }

  auto& progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  progressManager.registerPoller([]() { return getInstance().pollInFlightTransfers(); });
  progressManager.start();
}

/**
 * Completes every posted transfer, stops the progress engine and releases the exchange
 * communicator. The leading barrier keeps a rank from freeing while a neighbour still
 * exchanges, and the trailing one keeps a rank from running ahead into the rest of the
 * shutdown while a neighbour still holds the communicator.
 */
void
peano4::communication::CommunicationManager::shutdown() {
  auto& progressManager = tarch::mpi::AbstractCommunicationManager::getInstance();
  progressManager.finishPendingWork();
  progressManager.stop();

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  if (_exchangeCommunicator != MPI_COMM_NULL) {
    MPI_Comm_free(&_exchangeCommunicator);
    _exchangeCommunicator = MPI_COMM_NULL;
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

MPI_Comm
peano4::communication::CommunicationManager::getCommunicator() const {
  assertion(_exchangeCommunicator != MPI_COMM_NULL);
  return _exchangeCommunicator;
}

/**
 * Returns the next unused tag of the exchange communicator. Every rank reserves its tags in
 * the same order, so the partners of an exchange hold the same tag without agreeing on it.
 */
int
peano4::communication::CommunicationManager::reserveTag() {
  assertion(_exchangeCommunicator != MPI_COMM_NULL);
  if (_nextTag > _tagUpperBound) {
    tarch::terminateWithError(
      "The exchange communicator has no tag left: Every tag up to " + std::to_string(_tagUpperBound) + " is reserved."
    );
  }
  return _nextTag++;
}

/**
 * Adds one posted transfer to the poller's list. The push is lock-free, so several threads post
 * concurrently: A poster replaces the list head and the poller takes the whole list in one
 * exchange.
 */
void
peano4::communication::CommunicationManager::registerInFlightTransfer(std::function<bool()> pollTransfer) {
  assertion(static_cast<bool>(pollTransfer));
  auto* const transfer = new InFlightTransfer{std::move(pollTransfer), nullptr};
  transfer->next       = _newInFlightTransfers.load(std::memory_order_relaxed);
  while (not _newInFlightTransfers
               .compare_exchange_weak(transfer->next, transfer, std::memory_order_release, std::memory_order_relaxed)) {
  }
}

/**
 * Polls every in-flight transfer once and drops the completed ones. Returns true when nothing
 * is left in flight.
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
