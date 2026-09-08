// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CommunicationManagerTest.h"

#include "peano4/communication/CommunicationManager.h"
#include "peano4/data/RegisteredStackContainer.h"
#include "peano4/grid/GridVertex.h"
#include "peano4/stacks/Stack.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr int NumberOfProbedContainers = 16;

  /** Selects the largest container id covered by MPI's guaranteed tag range. */
  constexpr int TransferContainerId = 16383;

  constexpr int NumberOfTransferredSlots = 4;
  constexpr int FirstTransferredLevel    = 11;

  constexpr std::array<peano4::communication::CommunicationManager::AggregatedBoundaryMessage, 2> ProbedMessages{
    peano4::communication::CommunicationManager::AggregatedBoundaryMessage::Payload,
    peano4::communication::CommunicationManager::AggregatedBoundaryMessage::Migration
  };

  /**
   * Restores the rank set the manager held, so a test that rebuilds the
   * topology does not disturb a later one.
   */
  class TopologyGuard final {
  public:
    TopologyGuard() :
      _restoredRanks(peano4::communication::CommunicationManager::getInstance().getRemoteRanks()) {}
    TopologyGuard(const TopologyGuard&)            = delete;
    TopologyGuard(TopologyGuard&&)                 = delete;
    TopologyGuard& operator=(const TopologyGuard&) = delete;
    TopologyGuard& operator=(TopologyGuard&&)      = delete;
    ~TopologyGuard() { peano4::communication::CommunicationManager::getInstance().rebuildTopology(_restoredRanks); }

  private:
    const std::vector<int> _restoredRanks;
  };
} // namespace

peano4::communication::tests::CommunicationManagerTest::CommunicationManagerTest() :
  TestCase("peano4::communication::tests::CommunicationManagerTest") {}

/**
 * Every container and message part gets a tag of its own on one communicator.
 */
void peano4::communication::tests::CommunicationManagerTest::testTagsAreDistinctPerContainerAndMessage() {
  const auto&   manager = peano4::communication::CommunicationManager::getInstance();
  std::set<int> tags;
  MPI_Comm      firstCommunicator = MPI_COMM_NULL;

  for (int containerId = 0; containerId < NumberOfProbedContainers; containerId++) {
    for (const auto message : ProbedMessages) {
      const auto [tag, communicator] = manager.getAggregatedBoundaryExchangeMetaInformation(containerId, message);
      validate(tag >= 0);
      validate(tags.insert(tag).second);
      if (firstCommunicator == MPI_COMM_NULL) {
        firstCommunicator = communicator;
      }
      validate(communicator == firstCommunicator);
    }
  }

  validateEquals(static_cast<int>(tags.size()), NumberOfProbedContainers * static_cast<int>(ProbedMessages.size()));
}

/**
 * Repeated queries of one channel return the same tag.
 */
void peano4::communication::tests::CommunicationManagerTest::testTagsAreStableAcrossQueries() {
  const auto& manager = peano4::communication::CommunicationManager::getInstance();

  for (int round = 0; round < 64; round++) {
    for (int containerId = 0; containerId < NumberOfProbedContainers; containerId++) {
      for (const auto message : ProbedMessages) {
        const auto first  = manager.getAggregatedBoundaryExchangeMetaInformation(containerId, message);
        const auto second = manager.getAggregatedBoundaryExchangeMetaInformation(containerId, message);
        validateEquals(first.first, second.first);
        validate(first.second == second.second);
      }
    }
  }
}

/**
 * Every rank derives the same tag for a container and a message part.
 */
void peano4::communication::tests::CommunicationManagerTest::testEveryRankAgreesOnTheTags() {
  const auto&      manager = peano4::communication::CommunicationManager::getInstance();
  std::vector<int> localTags;
  for (int containerId = 0; containerId < NumberOfProbedContainers; containerId++) {
    for (const auto message : ProbedMessages) {
      localTags.push_back(manager.getAggregatedBoundaryExchangeMetaInformation(containerId, message).first);
    }
  }

  std::vector<int> minimumTags(localTags.size(), 0);
  std::vector<int> maximumTags(localTags.size(), 0);
  tarch::mpi::allreduce(
    localTags.data(),
    minimumTags.data(),
    static_cast<int>(localTags.size()),
    MPI_INT,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  tarch::mpi::allreduce(
    localTags.data(),
    maximumTags.data(),
    static_cast<int>(localTags.size()),
    MPI_INT,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );

  for (std::size_t entry = 0; entry < localTags.size(); entry++) {
    validateEquals(minimumTags[entry], maximumTags[entry]);
    validateEquals(localTags[entry], minimumTags[entry]);
  }
}

/**
 * An unchanged neighbour set keeps its topology version.
 */
void peano4::communication::tests::CommunicationManagerTest::testAnUnchangedRankSetKeepsTheVersion() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();

  const std::vector<int> ranks{1, 4, 9};
  manager.rebuildTopology(ranks);
  const auto settledVersion = manager.getTopologyVersion();

  for (int repetition = 0; repetition < 32; repetition++) {
    manager.rebuildTopology(ranks);
    validateEquals(manager.getTopologyVersion(), settledVersion);
  }
}

/**
 * Every changed neighbour set advances the topology version.
 */
void peano4::communication::tests::CommunicationManagerTest::testEveryChangedRankSetBumpsTheVersion() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();

  const std::vector<std::vector<int>> rankSets{{2}, {2, 3}, {3}, {}, {0, 1, 2, 3}, {0, 1, 3}};

  manager.rebuildTopology(rankSets.front());
  auto previousVersion = manager.getTopologyVersion();
  for (std::size_t setNumber = 1; setNumber < rankSets.size(); setNumber++) {
    manager.rebuildTopology(rankSets[setNumber]);
    validate(manager.getTopologyVersion() > previousVersion);
    previousVersion = manager.getTopologyVersion();
  }
}

/**
 * rebuildTopology() keeps the order and the entries of the rank set it is given.
 */
void peano4::communication::tests::CommunicationManagerTest::testTheRankSetIsHandedBackUnchanged() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();

  const std::vector<int> ranks{0, 5, 6, 11};
  manager.rebuildTopology(ranks);

  const auto& storedRanks = manager.getRemoteRanks();
  validateEquals(storedRanks.size(), ranks.size());
  for (std::size_t rankNumber = 0; rankNumber < ranks.size(); rankNumber++) {
    validateEquals(storedRanks[rankNumber], ranks[rankNumber]);
  }
}

/**
 * A topology with no posted message reports itself idle.
 */
void peano4::communication::tests::CommunicationManagerTest::testATopologyWithoutTransfersReportsItselfIdle() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();

  manager.rebuildTopology({});
  validate(manager.haveTransfersCompleted());

  manager.rebuildTopology({1, 2, 3});
  validate(manager.haveTransfersCompleted());
  validate(manager.pollInFlightTransfers());
}

/**
 * A sweep and the poller leave the rank set and the topology version unchanged.
 */
void peano4::communication::tests::CommunicationManagerTest::testSweepsAndPollingLeaveTheTopologyAlone() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();

  const std::vector<int> ranks{7, 8};
  manager.rebuildTopology(ranks);
  const auto settledVersion = manager.getTopologyVersion();

  for (int sweep = 0; sweep < 64; sweep++) {
    manager.beginSweep();
    validate(manager.pollInFlightTransfers());
    validate(manager.haveTransfersCompleted());
    validateEquals(manager.getTopologyVersion(), settledVersion);
    validateEquals(manager.getRemoteRanks().size(), ranks.size());
  }
}

/**
 * Moves pool slots to a rank and reads them back, which on one rank is the
 * rank itself.
 */
void peano4::communication::tests::CommunicationManagerTest::testATransferReachesTheReceivingSlots() {
  const TopologyGuard guard;
  auto&               manager = peano4::communication::CommunicationManager::getInstance();
  const int           ownRank = tarch::mpi::Rank::getInstance().getRank();

  peano4::data::RegisteredStackContainer<peano4::stacks::Stack<peano4::grid::GridVertex>> container(
    TransferContainerId,
    peano4::data::ManagedContainerKind::VertexData
  );
  container.resizePoolForTree(0, 0, NumberOfTransferredSlots);
  for (int slot = 0; slot < NumberOfTransferredSlots; slot++) {
    container.getPoolSlot(slot).level = FirstTransferredLevel + slot;
    container.getPoolSlot(slot).setState(peano4::grid::GridVertex::State::Refined);
  }

  manager.rebuildTopology({ownRank});

  // Slot i travels to the slot the receive list names at the same position, so
  // reversing the receive list has to reverse the levels.
  std::map<int, std::vector<int>> sendSlots;
  std::map<int, std::vector<int>> receiveSlots;
  for (int slot = 0; slot < NumberOfTransferredSlots; slot++) {
    sendSlots[ownRank].push_back(slot);
    receiveSlots[ownRank].push_back(NumberOfTransferredSlots - 1 - slot);
  }

  std::vector<std::pair<int, peano4::grid::GridVertex>> pendingReceives;
  peano4::communication::CommunicationManager::transferPoolSlots(container, sendSlots, receiveSlots, pendingReceives);
  validate(manager.haveTransfersCompleted());
  validateEquals(static_cast<int>(pendingReceives.size()), NumberOfTransferredSlots);

  for (const auto& [poolSlot, receivedVertex] : pendingReceives) {
    validate(poolSlot >= 0 and poolSlot < NumberOfTransferredSlots);
    validateEquals(receivedVertex.level, FirstTransferredLevel + NumberOfTransferredSlots - 1 - poolSlot);
    validateEquals(
      static_cast<int>(receivedVertex.getState()),
      static_cast<int>(peano4::grid::GridVertex::State::Refined)
    );
  }
}

void peano4::communication::tests::CommunicationManagerTest::run() {
  testMethod(testTagsAreDistinctPerContainerAndMessage);
  testMethod(testTagsAreStableAcrossQueries);
  testMethod(testEveryRankAgreesOnTheTags);
  testMethod(testAnUnchangedRankSetKeepsTheVersion);
  testMethod(testEveryChangedRankSetBumpsTheVersion);
  testMethod(testTheRankSetIsHandedBackUnchanged);
  testMethod(testATopologyWithoutTransfersReportsItselfIdle);
  testMethod(testSweepsAndPollingLeaveTheTopologyAlone);
  testMethod(testATransferReachesTheReceivingSlots);
}
