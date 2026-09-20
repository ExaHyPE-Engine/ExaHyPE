// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManager.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

peano4::communication::CommunicationManager peano4::communication::CommunicationManager::_singleton;

namespace {
  constexpr int AggregatedBoundaryMessagesPerContainer = 2;

  int getAggregatedBoundaryMessageTagOffset(
    const peano4::communication::CommunicationManager::AggregatedBoundaryMessage message
  ) {
    switch (message) {
    case peano4::communication::CommunicationManager::AggregatedBoundaryMessage::Payload:
      return 0;
    case peano4::communication::CommunicationManager::AggregatedBoundaryMessage::Migration:
      return 1;
    }
    tarch::terminateWithError("The aggregated boundary message has no tag offset.");
  }
} // namespace

peano4::communication::CommunicationManager& peano4::communication::CommunicationManager::getInstance() {
  return _singleton;
}

/**
 * Determines the tag space every rank can address and creates the exchange
 * communicator. Collective over all ranks.
 */
void peano4::communication::CommunicationManager::init() {
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

  const int localTagUpperBound   = *tagUpperBound;
  int       minimumTagUpperBound = localTagUpperBound;
  tarch::mpi::allreduce(
    &localTagUpperBound,
    &minimumTagUpperBound,
    1,
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  _maximumAggregatedContainerId =
    static_cast<int>((static_cast<std::int64_t>(minimumTagUpperBound) + 1) / AggregatedBoundaryMessagesPerContainer);

  if (
    MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_aggregatedBoundaryDataCommunicator) != MPI_SUCCESS
  ) {
    tarch::terminateWithError("Could not create the communicator for the aggregated boundary exchange.");
  }
}

/**
 * Releases the exchange communicator.
 *
 * Every rank frees the communicator, which is collective. The leading barrier
 * keeps a rank from freeing while a neighbour still exchanges tree data, and the
 * trailing one keeps a rank from running ahead into the rest of the shutdown
 * while a neighbour still holds the communicator.
 */
void peano4::communication::CommunicationManager::shutdown() {
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  if (_aggregatedBoundaryDataCommunicator != MPI_COMM_NULL) {
    MPI_Comm_free(&_aggregatedBoundaryDataCommunicator);
    _aggregatedBoundaryDataCommunicator = MPI_COMM_NULL;
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

/**
 * An aggregated exchange carries the boundary data of a rank pair in one
 * message per container, so its tag identifies the container and the message
 * part, and a container is addressed by a small number.
 */
peano4::communication::CommunicationManager::GridDataExchangeMetaInformation peano4::communication::
  CommunicationManager::getAggregatedBoundaryExchangeMetaInformation(
    const int                       containerId,
    const AggregatedBoundaryMessage message
  ) const {
  assertion(_aggregatedBoundaryDataCommunicator != MPI_COMM_NULL);
  if (containerId < 0 or containerId >= _maximumAggregatedContainerId) {
    tarch::terminateWithError(
      "Container id " + std::to_string(containerId) + " leaves the aggregated boundary tag space of "
      + std::to_string(_maximumAggregatedContainerId) + " containers."
    );
  }

  const int tag = containerId * AggregatedBoundaryMessagesPerContainer + getAggregatedBoundaryMessageTagOffset(message);
  return GridDataExchangeMetaInformation(tag, _aggregatedBoundaryDataCommunicator);
}

/**
 * Names the buffers of the sweep that is about to start.
 *
 * Raising the generation before any data moves marks every container's buffers
 * as belonging to an earlier sweep, so the phases that run ahead of the
 * boundary exchange do not mistake them for their own.
 */
void peano4::communication::CommunicationManager::beginSweep() { _generation++; }

int peano4::communication::CommunicationManager::getTopologyVersion() const { return _topologyVersion; }

const std::vector<int>& peano4::communication::CommunicationManager::getRemoteRanks() const { return _remoteRanks; }

int peano4::communication::CommunicationManager::getRemoteRankSlot(const int remoteRank) const {
  const auto entry = std::lower_bound(_remoteRanks.begin(), _remoteRanks.end(), remoteRank);
  assertion(entry != _remoteRanks.end() and *entry == remoteRank, remoteRank);
  return static_cast<int>(entry - _remoteRanks.begin());
}

/**
 * Converts a container size into the element count accepted by the stack
 * communication interface.
 */
int peano4::communication::CommunicationManager::toMessageElementCount(const std::size_t numberOfElements) {
  if (numberOfElements > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
    std::ostringstream message;
    message << "The aggregated boundary payload contains " << numberOfElements
            << " elements, but the message interface accepts at most " << std::numeric_limits<int>::max()
            << " elements.";
    tarch::terminateWithError(message.str());
  }
  return static_cast<int>(numberOfElements);
}

/**
 * Returns whether every registered transfer has completed.
 *
 * A zero count is meaningful only after all posts for the queried sweep have
 * been issued.
 */
bool peano4::communication::CommunicationManager::haveTransfersCompleted() const {
  for (std::size_t rankSlot = 0; rankSlot < _remoteRanks.size(); rankSlot++) {
    if (_outstandingTransfersPerRank[rankSlot].load(std::memory_order_acquire) > 0) {
      return false;
    }
  }
  return true;
}

/**
 * Hands one rank pair's posted messages to the manager's poller.
 *
 * The push is lock-free, so the containers of one sweep post concurrently. A
 * poster replaces the list head and the poller takes the whole list in one
 * exchange. The counter rises before the push, so a reader that sees the
 * counter at zero knows the poller has completed this transfer.
 */
void peano4::communication::CommunicationManager::registerInFlightTransfer(
  const int             remoteRank,
  std::function<bool()> pollTransfer,
  std::function<void()> markTransferComplete
) {
  const int rankSlot = getRemoteRankSlot(remoteRank);
  _outstandingTransfersPerRank[rankSlot].fetch_add(1, std::memory_order_relaxed);

  auto* const transfer =
    new InFlightTransfer{std::move(pollTransfer), std::move(markTransferComplete), rankSlot, nullptr};
  transfer->next = _newInFlightTransfers.load(std::memory_order_relaxed);
  while (not _newInFlightTransfers
               .compare_exchange_weak(transfer->next, transfer, std::memory_order_release, std::memory_order_relaxed)) {
  }
}

/**
 * Polls every in-flight transfer once. Completed transfers leave the polling
 * set and release their rank counter. Returns true when nothing is left in
 * flight.
 */
bool peano4::communication::CommunicationManager::pollInFlightTransfers() {
  auto* newTransfers = _newInFlightTransfers.exchange(nullptr, std::memory_order_acquire);
  while (newTransfers != nullptr) {
    auto* const next = newTransfers->next;
    _transfersBeingPolled.emplace_back(newTransfers);
    newTransfers = next;
  }

  std::erase_if(_transfersBeingPolled, [this](const std::unique_ptr<InFlightTransfer>& transfer) {
    if (transfer->poll()) {
      // The completion flag precedes the counter release, so an observer that
      // sees the counter reach zero finds every flag already set.
      transfer->markTransferComplete();
      _outstandingTransfersPerRank[transfer->rankSlot].fetch_sub(1, std::memory_order_release);
      return true;
    }
    return false;
  });

  return _transfersBeingPolled.empty() and _newInFlightTransfers.load(std::memory_order_relaxed) == nullptr;
}

/**
 * Installs the given sorted neighbour rank set.
 *
 * Only a changed rank set advances the topology version, because the
 * containers key their rank records on that set alone. The counters are
 * replaced with the set, so this runs only while no transfer is in flight.
 */
void peano4::communication::CommunicationManager::rebuildTopology(const std::vector<int>& sortedRemoteRanks) {
#if PEANO_ASSERTIONS
  // getRemoteRankSlot resolves a rank through a binary search, so a repeated
  // entry would map two neighbours onto one counter.
  const bool ranksAreSorted = std::is_sorted(sortedRemoteRanks.begin(), sortedRemoteRanks.end());
  const bool ranksAreUnique =
    std::adjacent_find(sortedRemoteRanks.begin(), sortedRemoteRanks.end()) == sortedRemoteRanks.end();
  assertion(ranksAreSorted);
  assertion(ranksAreUnique);
#endif

  if (_remoteRanks != sortedRemoteRanks) {
    assertion(haveTransfersCompleted(), "The topology may only be rebuilt while no transfer is in flight.");
    _remoteRanks                 = sortedRemoteRanks;
    _outstandingTransfersPerRank = std::make_unique<std::atomic<int>[]>(_remoteRanks.size());
    _topologyVersion++;
  }
}
