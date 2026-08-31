// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RegisteredStackContainerTest.h"

#include "peano4/data/ContainerRegistry.h"
#include "peano4/data/RegisteredStackContainer.h"
#include "peano4/stacks/Stack.h"
#include "tarch/mpi/Rank.h"

namespace {
  /**
   * One value that says which slot the element came from.
   */
  struct StampedValue {
    int value{0};
  };

  using StampedContainer = peano4::data::RegisteredStackContainer<peano4::stacks::Stack<StampedValue>>;

  /** Element type of a container whose copies must not diverge, such as a particle set. */
  struct UnreplicatedValue {
    static constexpr bool SupportsSubtreeTaskReplication = false;

    int value{0};
  };

  using UnreplicatedContainer = peano4::data::RegisteredStackContainer<peano4::stacks::Stack<UnreplicatedValue>>;

  /** Container ids above anything a generated repository hands out. */
  constexpr int FirstTestContainerId = 4300000;

  void stampPool(StampedContainer& container, const int firstValue) {
    for (int slot = 0; slot < static_cast<int>(container.getPoolSize()); slot++) {
      container.getPoolSlot(slot).value = firstValue + slot;
    }
  }
} // namespace

peano4::data::tests::RegisteredStackContainerTest::RegisteredStackContainerTest() :
  TestCase("peano4::data::tests::RegisteredStackContainerTest") {}

/**
 * One resize call carries the cell, face and vertex counts together, and each
 * container takes the one its kind names.
 */
void peano4::data::tests::RegisteredStackContainerTest::testEachKindSizesAgainstItsOwnEntityCount() {
  StampedContainer cellContainer(FirstTestContainerId, peano4::data::ManagedContainerKind::CellData);
  StampedContainer faceContainer(FirstTestContainerId + 1, peano4::data::ManagedContainerKind::FaceData);
  StampedContainer vertexContainer(FirstTestContainerId + 2, peano4::data::ManagedContainerKind::VertexData);

  constexpr std::size_t CellCount   = 7;
  constexpr std::size_t FaceCount   = 13;
  constexpr std::size_t VertexCount = 23;

  for (int round = 0; round < 8; round++) {
    cellContainer.resizePoolForTree(CellCount + round, FaceCount + round, VertexCount + round);
    faceContainer.resizePoolForTree(CellCount + round, FaceCount + round, VertexCount + round);
    vertexContainer.resizePoolForTree(CellCount + round, FaceCount + round, VertexCount + round);

    validateEquals(cellContainer.getPoolSize(), CellCount + round);
    validateEquals(faceContainer.getPoolSize(), FaceCount + round);
    validateEquals(vertexContainer.getPoolSize(), VertexCount + round);
  }
}

/**
 * The migration copies every surviving entity's data to the slot its pair
 * names.
 */
void peano4::data::tests::RegisteredStackContainerTest::testMigrationMovesTheDataToItsNewSlots() {
  StampedContainer container(FirstTestContainerId + 3, peano4::data::ManagedContainerKind::CellData);

  constexpr std::size_t NumberOfSlots = 6;
  container.resizePoolForTree(NumberOfSlots, 0, 0);
  stampPool(container, 100);

  // Reverses the slot order, so every pair moves and no target coincides with
  // its own source.
  std::vector<std::pair<int, int>> cellPairs;
  for (int slot = 0; slot < static_cast<int>(NumberOfSlots); slot++) {
    cellPairs.emplace_back(slot, static_cast<int>(NumberOfSlots) - 1 - slot);
  }

  container.migratePoolForTree(cellPairs, {}, {}, NumberOfSlots, 0, 0);

  validateEquals(container.getPoolSize(), NumberOfSlots);
  for (int slot = 0; slot < static_cast<int>(NumberOfSlots); slot++) {
    validateEquals(container.getPoolSlot(slot).value, 100 + static_cast<int>(NumberOfSlots) - 1 - slot);
  }
}

/**
 * replicateInterfaceSlots() copies entity data along the named slot pairs and
 * writes to no other slot.
 */
void peano4::data::tests::RegisteredStackContainerTest::testReplicationCopiesOnlyTheNamedSlots() {
  StampedContainer faceContainer(FirstTestContainerId + 4, peano4::data::ManagedContainerKind::FaceData);

  constexpr std::size_t NumberOfSlots = 5;
  faceContainer.resizePoolForTree(0, NumberOfSlots, 0);
  stampPool(faceContainer, 200);

  const std::vector<std::pair<int, int>> facePairs{{0, 3}};
  faceContainer.replicateInterfaceSlots(facePairs, {}, 0, 1);

  validateEquals(faceContainer.getPoolSlot(3).value, 200);
  for (const int untouchedSlot : {1, 2, 4}) {
    validateEquals(faceContainer.getPoolSlot(untouchedSlot).value, 200 + untouchedSlot);
  }

  // A vertex container ignores the face pairs of the same call, and the other
  // way round, so one call can serve both families.
  StampedContainer vertexContainer(FirstTestContainerId + 5, peano4::data::ManagedContainerKind::VertexData);
  vertexContainer.resizePoolForTree(0, 0, NumberOfSlots);
  stampPool(vertexContainer, 300);
  vertexContainer.replicateInterfaceSlots(facePairs, {}, 0, 1);
  for (int slot = 0; slot < static_cast<int>(NumberOfSlots); slot++) {
    validateEquals(vertexContainer.getPoolSlot(slot).value, 300 + slot);
  }
}

/**
 * The chunks of one replication call partition the pair list, so together they
 * copy every pair once and one chunk alone leaves the other chunks' targets
 * untouched.
 */
void peano4::data::tests::RegisteredStackContainerTest::testReplicationChunksPartitionThePairs() {
  StampedContainer faceContainer(FirstTestContainerId + 12, peano4::data::ManagedContainerKind::FaceData);

  constexpr std::size_t NumberOfSlots = 10;
  faceContainer.resizePoolForTree(0, NumberOfSlots, 0);
  stampPool(faceContainer, 400);

  const std::vector<std::pair<int, int>> facePairs{{0, 5}, {1, 6}, {2, 7}, {3, 8}, {4, 9}};
  constexpr int                          NumberOfChunks = 2;
  faceContainer.replicateInterfaceSlots(facePairs, {}, 0, NumberOfChunks);
  validateEquals(faceContainer.getPoolSlot(5).value, 400);
  validateEquals(faceContainer.getPoolSlot(6).value, 401);
  for (const int untouchedSlot : {7, 8, 9}) {
    validateEquals(faceContainer.getPoolSlot(untouchedSlot).value, 400 + untouchedSlot);
  }

  faceContainer.replicateInterfaceSlots(facePairs, {}, 1, NumberOfChunks);
  for (int pairNumber = 0; pairNumber < static_cast<int>(facePairs.size()); pairNumber++) {
    validateEquals(faceContainer.getPoolSlot(facePairs[pairNumber].second).value, 400 + pairNumber);
  }
}

/**
 * A cell container takes no part in the per-sweep boundary exchange.
 */
void peano4::data::tests::RegisteredStackContainerTest::testCellDataStaysOutOfTheBoundaryExchange() {
  StampedContainer cellContainer(FirstTestContainerId + 6, peano4::data::ManagedContainerKind::CellData);

  constexpr std::size_t NumberOfSlots = 4;
  cellContainer.resizePoolForTree(NumberOfSlots, 0, 0);
  stampPool(cellContainer, 400);

  // Pack lists that address slots this container does not own. A cell container
  // ignores both families.
  const std::vector<std::vector<int>> packLists{{0, 1}};
  cellContainer.packBoundarySlots(packLists, packLists);
  cellContainer.finishBoundaryExchange(packLists, packLists, {0}, {0});

  validateEquals(cellContainer.getPoolSize(), NumberOfSlots);
  for (int slot = 0; slot < static_cast<int>(NumberOfSlots); slot++) {
    validateEquals(cellContainer.getPoolSlot(slot).value, 400 + slot);
  }
}

/**
 * A container registers in its own constructor and leaves the registry on
 * destruction.
 */
void peano4::data::tests::RegisteredStackContainerTest::testTheContainerJoinsAndLeavesTheRegistry() {
  auto&      registry                = peano4::data::ContainerRegistry::getInstance();
  const auto replicationBeforeAnyone = registry.supportsSubtreeTaskReplication();

  {
    StampedContainer container(FirstTestContainerId + 7, peano4::data::ManagedContainerKind::VertexData);
    registry.resizePoolsForTree(0, 0, 9);
    validateEquals(container.getPoolSize(), std::size_t(9));
    validate(container.supportsSubtreeTaskReplication());
    validateEquals(container.getManagedContainerId(), FirstTestContainerId + 7);
  }

  validateEquals(registry.supportsSubtreeTaskReplication(), replicationBeforeAnyone);
  // A call that reaches a detached container faults, so this call must not reach one.
  registry.resizePoolsForTree(0, 0, 0);
}

/**
 * An element type declaring SupportsSubtreeTaskReplication false pins its rank to a
 * single subtree task, and one such container decides for the whole registry.
 */
void peano4::data::tests::RegisteredStackContainerTest::testAnElementTypeCanRefuseTaskReplication() {
  auto& registry = peano4::data::ContainerRegistry::getInstance();
  validate(registry.supportsSubtreeTaskReplication());

  {
    const UnreplicatedContainer container(FirstTestContainerId + 8, peano4::data::ManagedContainerKind::VertexData);
    validate(not container.supportsSubtreeTaskReplication());
    validate(not registry.supportsSubtreeTaskReplication());

    // A replicating container beside it does not restore the registry's answer.
    const StampedContainer replicatingContainer(
      FirstTestContainerId + 9,
      peano4::data::ManagedContainerKind::VertexData
    );
    validate(replicatingContainer.supportsSubtreeTaskReplication());
    validate(not registry.supportsSubtreeTaskReplication());
  }

  validate(registry.supportsSubtreeTaskReplication());
}

/**
 * A cleared container releases its pool and takes a fresh one afterwards.
 */
void peano4::data::tests::RegisteredStackContainerTest::testClearReleasesThePool() {
  StampedContainer container(FirstTestContainerId + 10, peano4::data::ManagedContainerKind::FaceData);

  constexpr std::size_t NumberOfSlots = 5;
  container.resizePoolForTree(0, NumberOfSlots, 0);
  container.prepareLocalSlots(3);
  stampPool(container, 500);

  container.clear();
  validateEquals(container.getPoolSize(), std::size_t(0));

  container.resizePoolForTree(0, NumberOfSlots, 0);
  validateEquals(container.getPoolSize(), NumberOfSlots);
  stampPool(container, 600);
  validateEquals(container.getPoolSlot(0).value, 600);
}

void peano4::data::tests::RegisteredStackContainerTest::testTaskSlotsFollowTheRankStride() {
  StampedContainer container(FirstTestContainerId + 11, peano4::data::ManagedContainerKind::VertexData);
  container.initialiseSerialSlot();
  container.prepareLocalSlots(3);

  const auto  rank          = tarch::mpi::Rank::getInstance().getRank();
  const auto  numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const auto& serialStack   = container.getLocalExchangeStack(rank, 0);
  const auto& firstTask     = container.getLocalExchangeStack(rank + numberOfRanks, 0);
  const auto& secondTask    = container.getLocalExchangeStack(rank + 2 * numberOfRanks, 0);
  validate(&serialStack != &firstTask);
  validate(&serialStack != &secondTask);
  validate(&firstTask != &secondTask);
  validate(&serialStack == &container.getLocalExchangeStack(rank, 0));

  container.clear();
}

void peano4::data::tests::RegisteredStackContainerTest::run() {
  testMethod(testEachKindSizesAgainstItsOwnEntityCount);
  testMethod(testMigrationMovesTheDataToItsNewSlots);
  testMethod(testReplicationCopiesOnlyTheNamedSlots);
  testMethod(testReplicationChunksPartitionThePairs);
  testMethod(testCellDataStaysOutOfTheBoundaryExchange);
  testMethod(testTheContainerJoinsAndLeavesTheRegistry);
  testMethod(testAnElementTypeCanRefuseTaskReplication);
  testMethod(testClearReleasesThePool);
  testMethod(testTaskSlotsFollowTheRankStride);
}
