// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/mpi/AbstractCommunicationManager.h"
#include "tarch/mpi/Diagnostics.h"
#include "tarch/mpi/PollPacing.h"

#include <mpi.h>

namespace peano4::communication {
  class CommunicationManager;
} // namespace peano4::communication

/**
 * Exchanges aggregated boundary and migration data with the neighbour ranks.
 *
 * The manager owns the exchange communicator, whose tags address a container
 * and a message part, together with the rank-neighbour topology and the
 * in-flight transfers a sweep posts. init() creates the communicator and is
 * collective.
 */
class peano4::communication::CommunicationManager final {
public:
  /**
   * Classifies messages exchanged between one pair of ranks. Boundary payloads overlap a
   * sweep, while migration payloads complete inside a repartitioning step.
   */
  enum class AggregatedBoundaryMessage { Payload, Migration };

  using GridDataExchangeMetaInformation = std::pair<int, MPI_Comm>;

  CommunicationManager(const CommunicationManager&)            = delete;
  CommunicationManager(CommunicationManager&&)                 = delete;
  CommunicationManager& operator=(const CommunicationManager&) = delete;
  CommunicationManager& operator=(CommunicationManager&&)      = delete;
  ~CommunicationManager()                                      = default;

  static peano4::communication::CommunicationManager& getInstance();

  void init();

  void shutdown();

  [[nodiscard]] GridDataExchangeMetaInformation getAggregatedBoundaryExchangeMetaInformation(
    int                       containerId,
    AggregatedBoundaryMessage message
  ) const;

  void beginSweep();

  void rebuildTopology(const std::vector<int>& sortedRemoteRanks);

  [[nodiscard]] int getTopologyVersion() const;

  [[nodiscard]] const std::vector<int>& getRemoteRanks() const;

  [[nodiscard]] bool haveTransfersCompleted() const;

  bool pollInFlightTransfers();

  template <class Container>
  static void packPoolSlots(Container& stackContainer, const std::vector<std::vector<int>>& packLists);

  template <class Container>
  static void transferPoolSlots(
    Container&                                                    stackContainer,
    const std::map<int, std::vector<int>>&                        sendSlotLists,
    const std::map<int, std::vector<int>>&                        receiveSlotLists,
    std::vector<std::pair<int, typename Container::ElementType>>& pendingReceives
  );

  template <class Container>
  static void finishPoolExchange(
    Container&                           stackContainer,
    const std::vector<std::vector<int>>& packLists,
    const std::vector<int>&              frozenSlotBases
  );

private:
  CommunicationManager() = default;

  /** Tracks one rank pair's posted messages for one container. */
  struct InFlightTransfer final {
    std::function<bool()> poll{};
    std::function<void()> markTransferComplete{};
    int                   rankSlot{-1};
    InFlightTransfer*     next{nullptr};
  };

  static peano4::communication::CommunicationManager _singleton;

  void registerInFlightTransfer(
    int                   remoteRank,
    std::function<bool()> pollTransfer,
    std::function<void()> markTransferComplete
  );

  template <class Container>
  static void postPayloadExchange(Container& stackContainer, int remoteRank);

  [[nodiscard]] int getRemoteRankSlot(int remoteRank) const;

  static int toMessageElementCount(std::size_t numberOfElements);

  /**
   * Carries the aggregated boundary exchange. Its tags address a container and
   * a message part, so it needs a tag space of its own.
   */
  MPI_Comm _aggregatedBoundaryDataCommunicator{MPI_COMM_NULL};

  int _maximumAggregatedContainerId{0};

  /**
   * Stores sorted neighbour ranks every registered container exchanges with.
   */
  std::vector<int> _remoteRanks{};

  /**
   * Names the rank-neighbour topology. A container compares it
   * against the version its own rank records were built for, so each
   * container rebuilds them once per topology.
   */
  int _topologyVersion{0};

  /**
   * Names the sweep the buffers describe. A container compares it against the
   * sweep its own buffers were prepared for.
   */
  int _generation{0};

  /**
   * Counts registered transfers per remote rank, over all containers,
   * that the manager's poller has not completed yet. Zero means every message
   * this rank pair posted has arrived and may be consumed.
   */
  std::unique_ptr<std::atomic<int>[]> _outstandingTransfersPerRank{};

  /**
   * Collects freshly registered transfers pushed lock-free by the posting tasks.
   * The poller is the single consumer and takes the whole list into its own
   * polling set, so no lock sits on the polling loop.
   */
  std::atomic<InFlightTransfer*> _newInFlightTransfers{nullptr};

  /** Belongs exclusively to the polling thread, so its accesses need no synchronisation. */
  std::vector<std::unique_ptr<InFlightTransfer>> _transfersBeingPolled{};
};

#include "peano4/communication/CommunicationManager.cpph"
