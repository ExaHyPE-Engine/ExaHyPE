// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ExchangeChannels.h"

#include "peano4/parallel/TreeIdMapping.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

peano4::parallel::ExchangeChannels peano4::parallel::ExchangeChannels::_singleton;

namespace {
  constexpr int TagsPerReceivingTree = 4;

  /**
   * One aggregated exchange ships the payload, the segment table describing it
   * and, in a debug build, the segment count that validates the table length.
   */
  constexpr int AggregatedBoundaryMessagesPerContainer = 3;

  int getExchangeModeTagOffset(const peano4::parallel::ExchangeChannels::ExchangeMode exchange) {
    switch (exchange) {
    case peano4::parallel::ExchangeChannels::ExchangeMode::HorizontalData:
      return 0;
    case peano4::parallel::ExchangeChannels::ExchangeMode::VerticalData:
      return 1;
    case peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicMetadata:
      return 2;
    case peano4::parallel::ExchangeChannels::ExchangeMode::PeriodicData:
      return 3;
    }
    assertion(false);
    return -1;
  }

  int getAggregatedBoundaryMessageTagOffset(
    const peano4::parallel::ExchangeChannels::AggregatedBoundaryMessage message
  ) {
    switch (message) {
    case peano4::parallel::ExchangeChannels::AggregatedBoundaryMessage::Payload:
      return 0;
    case peano4::parallel::ExchangeChannels::AggregatedBoundaryMessage::SegmentTable:
      return 1;
    case peano4::parallel::ExchangeChannels::AggregatedBoundaryMessage::SegmentCount:
      return 2;
    }
    assertion(false);
    return -1;
  }
} // namespace

peano4::parallel::ExchangeChannels& peano4::parallel::ExchangeChannels::getInstance() { return _singleton; }

/**
 * Compute how many sending-tree slots can share one communicator. The tag
 * stores the sender offset within its group, the receiving-tree slot, and the
 * exchange mode. A zero result means that one sender cannot be represented.
 */
int peano4::parallel::ExchangeChannels::getNumberOfSendingTreesPerDataExchangeCommunicator(const int tagUpperBound) {
  constexpr std::int64_t TagsPerSendingTree = static_cast<std::int64_t>(TagsPerReceivingTree) * MAX_SPACETREES_PER_RANK;
  if (tagUpperBound < 0) {
    return 0;
  }

  const std::int64_t supportedSendingTrees = (static_cast<std::int64_t>(tagUpperBound) + 1) / TagsPerSendingTree;
  return static_cast<int>(std::min<std::int64_t>(MAX_SPACETREES_PER_RANK, supportedSendingTrees));
}

int peano4::parallel::ExchangeChannels::getDataExchangeCommunicatorIndex(
  const int localSendingTreeId,
  const int sendingTreesPerCommunicator
) {
  assertion(localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK, localSendingTreeId);
  assertion(sendingTreesPerCommunicator > 0, sendingTreesPerCommunicator);
  return localSendingTreeId / sendingTreesPerCommunicator;
}

int peano4::parallel::ExchangeChannels::getDataExchangeTag(
  const int                                              localSendingTreeId,
  const int                                              localReceivingTreeId,
  const peano4::parallel::ExchangeChannels::ExchangeMode exchange,
  const int                                              sendingTreesPerCommunicator
) {
  assertion(localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK, localSendingTreeId);
  assertion(localReceivingTreeId >= 0 and localReceivingTreeId < MAX_SPACETREES_PER_RANK, localReceivingTreeId);
  assertion(sendingTreesPerCommunicator > 0, sendingTreesPerCommunicator);

  const int          localSendingTreeOffset = localSendingTreeId % sendingTreesPerCommunicator;
  const std::int64_t tag                    = TagsPerReceivingTree
      * (static_cast<std::int64_t>(localSendingTreeOffset) * MAX_SPACETREES_PER_RANK + localReceivingTreeId)
    + getExchangeModeTagOffset(exchange);
  assertion(tag <= std::numeric_limits<int>::max(), tag);
  return static_cast<int>(tag);
}

/**
 * Discover the tag space every rank can address, size the sending-tree groups
 * accordingly, and create the communicator of the aggregated boundary
 * exchange. Collective over all ranks.
 */
void peano4::parallel::ExchangeChannels::init() {
  int*      tagUpperBound  = nullptr;
  int       attributeFound = 0;
  const int attributeResult =
    MPI_Comm_get_attr(tarch::mpi::Rank::getInstance().getCommunicator(), MPI_TAG_UB, &tagUpperBound, &attributeFound);
  if (attributeResult != MPI_SUCCESS or not attributeFound or tagUpperBound == nullptr) {
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
  _sendingTreesPerDataExchangeCommunicator = getNumberOfSendingTreesPerDataExchangeCommunicator(minimumTagUpperBound);
  if (_sendingTreesPerDataExchangeCommunicator <= 0) {
    tarch::terminateWithError(
      "MPI_TAG_UB " + std::to_string(minimumTagUpperBound) + " cannot encode one sending tree and "
      + std::to_string(MAX_SPACETREES_PER_RANK) + " receiving trees."
    );
  }
  _dataExchangeCommunicators.fill(MPI_COMM_NULL);
  _needCommunicator.fill(false);

  _maximumAggregatedContainerId = static_cast<int>(
    (static_cast<std::int64_t>(minimumTagUpperBound) + 1) / AggregatedBoundaryMessagesPerContainer
  );
  const int duplicationResult =
    MPI_Comm_dup(tarch::mpi::Rank::getInstance().getCommunicator(), &_aggregatedBoundaryDataCommunicator);
  if (duplicationResult != MPI_SUCCESS) {
    tarch::terminateWithError("Could not create the communicator for the aggregated boundary exchange.");
  }
}

/**
 * Release the tree data-exchange communicators
 *
 * Every rank frees the same set of communicators, which is a collective operation, so
 * the leading barrier makes sure no rank is still exchanging tree data when the first
 * one starts freeing. The trailing barrier keeps a rank that has finished from running
 * ahead into the rest of the shutdown while a neighbour still holds a communicator.
 */
void peano4::parallel::ExchangeChannels::shutdown() {
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  for (auto& communicator : _dataExchangeCommunicators) {
    if (communicator != MPI_COMM_NULL) {
      MPI_Comm_free(&communicator);
      communicator = MPI_COMM_NULL;
    }
  }
  _needCommunicator.fill(false);

  if (_aggregatedBoundaryDataCommunicator != MPI_COMM_NULL) {
    MPI_Comm_free(&_aggregatedBoundaryDataCommunicator);
    _aggregatedBoundaryDataCommunicator = MPI_COMM_NULL;
  }

  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}

void peano4::parallel::ExchangeChannels::requestCommunicator(const int localTreeId) {
  assertion(localTreeId >= 0 and localTreeId < MAX_SPACETREES_PER_RANK, localTreeId);
  _needCommunicator[localTreeId] = true;
}

bool peano4::parallel::ExchangeChannels::hasPendingCommunicatorRequests() const {
  return std::any_of(_needCommunicator.begin(), _needCommunicator.end(), [](const bool pending) { return pending; });
}

/**
 * Create communicator groups in the same order on every rank. Registrations
 * serviced while the allreduce progresses remain pending for the next call.
 */
void peano4::parallel::ExchangeChannels::createPendingCommunicators() {
  constexpr int                      NumberOfTreeSlots = MAX_SPACETREES_PER_RANK;
  std::array<int, NumberOfTreeSlots> localPendingTreeSlots{};
  std::array<int, NumberOfTreeSlots> globalPendingTreeSlots{};
  for (int localTreeId = 0; localTreeId < NumberOfTreeSlots; ++localTreeId) {
    localPendingTreeSlots[localTreeId] = _needCommunicator[localTreeId] ? 1 : 0;
  }

  tarch::mpi::allreduce(
    localPendingTreeSlots.data(),
    globalPendingTreeSlots.data(),
    NumberOfTreeSlots,
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  for (int localTreeId = 0; localTreeId < NumberOfTreeSlots; ++localTreeId) {
    if (globalPendingTreeSlots[localTreeId]) {
      const int communicatorIndex =
        getDataExchangeCommunicatorIndex(localTreeId, _sendingTreesPerDataExchangeCommunicator);
      if (_dataExchangeCommunicators[communicatorIndex] == MPI_COMM_NULL) {
        const int duplicateResult = MPI_Comm_dup(
          tarch::mpi::Rank::getInstance().getCommunicator(),
          &_dataExchangeCommunicators[communicatorIndex]
        );
        if (duplicateResult != MPI_SUCCESS) {
          tarch::terminateWithError("Could not create an MPI communicator for tree data exchange.");
        }
      }
    }
    if (localPendingTreeSlots[localTreeId]) {
      _needCommunicator[localTreeId] = false;
    }
  }
}

peano4::parallel::ExchangeChannels::GridDataExchangeMetaInformation
peano4::parallel::ExchangeChannels::getGridDataExchangeMetaInformation(
  const int          sendingTreeId,
  const int          receivingTreeId,
  const ExchangeMode exchange
) const {
  const int localSendingTreeId   = peano4::parallel::TreeIdMapping::getLocalTreeId(sendingTreeId);
  const int localReceivingTreeId = peano4::parallel::TreeIdMapping::getLocalTreeId(receivingTreeId);

  assertion(
    localSendingTreeId >= 0 and localSendingTreeId < MAX_SPACETREES_PER_RANK,
    sendingTreeId,
    MAX_SPACETREES_PER_RANK,
    peano4::parallel::TreeIdMapping::getRank(sendingTreeId),
    localSendingTreeId
  );
  assertion(
    localReceivingTreeId >= 0 and localReceivingTreeId < MAX_SPACETREES_PER_RANK,
    receivingTreeId,
    MAX_SPACETREES_PER_RANK,
    peano4::parallel::TreeIdMapping::getRank(receivingTreeId),
    localReceivingTreeId
  );

  const int communicatorIndex =
    getDataExchangeCommunicatorIndex(localSendingTreeId, _sendingTreesPerDataExchangeCommunicator);
  const int tag =
    getDataExchangeTag(localSendingTreeId, localReceivingTreeId, exchange, _sendingTreesPerDataExchangeCommunicator);

  return GridDataExchangeMetaInformation(tag, _dataExchangeCommunicators[communicatorIndex]);
}

/**
 * An aggregated exchange carries the boundary data of every tree pair between
 * two ranks in one message, so its tag identifies the container and the message
 * part and never the trees. The exchange runs on a communicator of its own,
 * which keeps this flat tag space clear of the per-tree-pair tags and lets a
 * container be addressed by a small number.
 */
peano4::parallel::ExchangeChannels::GridDataExchangeMetaInformation
peano4::parallel::ExchangeChannels::getAggregatedBoundaryExchangeMetaInformation(
  const int                       containerId,
  const AggregatedBoundaryMessage message
) const {
  assertion(_aggregatedBoundaryDataCommunicator != MPI_COMM_NULL);
  assertion(
    containerId >= 0 and containerId < _maximumAggregatedContainerId,
    containerId,
    _maximumAggregatedContainerId
  );

  const int tag = containerId * AggregatedBoundaryMessagesPerContainer + getAggregatedBoundaryMessageTagOffset(message);
  return GridDataExchangeMetaInformation(tag, _aggregatedBoundaryDataCommunicator);
}
