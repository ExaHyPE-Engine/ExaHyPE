// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <mpi.h>

namespace peano4::parallel {
  class ExchangeChannels;

  namespace tests {
    class ExchangeChannelsTest;
  } // namespace tests
} // namespace peano4::parallel

/**
 * Communicators and tags of the spacetree data exchange. The per-tree-pair
 * exchange derives its tag from the local sending and receiving slots and the
 * exchange mode, and shares duplicated communicators across groups of sending
 * trees sized by the tag space MPI grants. The aggregated boundary exchange
 * runs on a communicator of its own whose tags address a container and a
 * message part. Communicators are created collectively on demand.
 */
class peano4::parallel::ExchangeChannels final {
public:
  enum class ExchangeMode {
    HorizontalData,
    VerticalData,
    PeriodicMetadata,
    PeriodicData
  };

  /**
   * The messages that make up one aggregated boundary exchange between a pair
   * of ranks. Only a debug build sends the segment count, but the tag layout is
   * the same either way, so the two builds agree on what a tag means.
   */
  enum class AggregatedBoundaryMessage { Payload, SegmentTable, SegmentCount };

  using GridDataExchangeMetaInformation = std::pair<int, MPI_Comm>;

  ~ExchangeChannels()                                  = default;
  ExchangeChannels(const ExchangeChannels&)            = delete;
  ExchangeChannels(ExchangeChannels&&)                 = delete;
  ExchangeChannels& operator=(const ExchangeChannels&) = delete;
  ExchangeChannels& operator=(ExchangeChannels&&)      = delete;

  static ExchangeChannels& getInstance();

  void init();

  void shutdown();

  /**
   * Demand the data-exchange communicator of the group the local tree slot
   * belongs to. The next createPendingCommunicators() call services it.
   */
  void requestCommunicator(const int localTreeId);

  bool hasPendingCommunicatorRequests() const;

  void createPendingCommunicators();

  GridDataExchangeMetaInformation getGridDataExchangeMetaInformation(
    const int          sendingTreeId,
    const int          receivingTreeId,
    const ExchangeMode exchange
  ) const;

  GridDataExchangeMetaInformation
  getAggregatedBoundaryExchangeMetaInformation(const int containerId, const AggregatedBoundaryMessage message) const;

private:
  friend class peano4::parallel::tests::ExchangeChannelsTest;

  ExchangeChannels() = default;

  static ExchangeChannels _singleton;

  static int getNumberOfSendingTreesPerDataExchangeCommunicator(const int tagUpperBound);
  static int getDataExchangeCommunicatorIndex(const int localSendingTreeId, const int sendingTreesPerCommunicator);
  static int getDataExchangeTag(
    const int          localSendingTreeId,
    const int          localReceivingTreeId,
    const ExchangeMode exchange,
    const int          sendingTreesPerCommunicator
  );

  std::array<MPI_Comm, MAX_SPACETREES_PER_RANK> _dataExchangeCommunicators{};
  std::array<bool, MAX_SPACETREES_PER_RANK>     _needCommunicator{};
  int                                           _sendingTreesPerDataExchangeCommunicator{0};

  /**
   * Carries the aggregated boundary exchange. Its tags address a container and
   * a message part instead of a tree pair, so it needs a tag space of its own.
   */
  MPI_Comm _aggregatedBoundaryDataCommunicator{MPI_COMM_NULL};

  /** Exclusive upper bound for aggregated boundary container IDs. */
  int _maximumAggregatedContainerId{0};
};
