// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ContainerRegistry.h"

#include "tarch/Assertions.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/tasking/Runtime.h"

namespace {
  /** Chunks per thread of one replication call; several let the runtime balance chunks of unequal cost. */
  constexpr int ReplicationChunksPerThread = 4;

  /**
   * Runs one operation per registered container concurrently. The containers
   * own disjoint pools, so no two loop iterations touch the same storage.
   */
  template <class Operation>
  void runContainerTasks(const std::vector<peano4::data::ManagedContainer*>& containers, const Operation& operation) {
    tarch::accelerator::parallelForOnHost(
      tarch::accelerator::Range(static_cast<int>(containers.size())),
      [&](const int containerNumber) { operation(containers[containerNumber]); }
    );
  }
} // namespace

peano4::data::ContainerRegistry& peano4::data::ContainerRegistry::getInstance() {
  static peano4::data::ContainerRegistry instance;
  return instance;
}

/**
 * Registers a container, keeping the set ordered by container id. Registration
 * is single-threaded, so no synchronisation is involved.
 */
void peano4::data::ContainerRegistry::attachContainer(peano4::data::ManagedContainer* container) {
  assertion(container != nullptr);
  const auto insertionPoint =
    std::find_if(_containers.begin(), _containers.end(), [container](const peano4::data::ManagedContainer* registered) {
      return registered->getManagedContainerId() >= container->getManagedContainerId();
    });
  [[maybe_unused]] const bool containerIdIsUnused = insertionPoint == _containers.end()
    or (*insertionPoint)->getManagedContainerId() != container->getManagedContainerId();
  assertion(containerIdIsUnused, container->getManagedContainerId());
  _containers.insert(insertionPoint, container);
}

void peano4::data::ContainerRegistry::detachContainer(const peano4::data::ManagedContainer* const container) {
  assertion(container != nullptr);
  const auto entry = std::find(_containers.begin(), _containers.end(), container);

  [[maybe_unused]] const bool containerIsRegistered = entry != _containers.end();
  assertion(containerIsRegistered);
  _containers.erase(entry);
}

/**
 * Returns whether every registered container's data may be replicated into one
 * copy per subtree task. A data type whose copies must not diverge restricts
 * the rank to a single task.
 */
bool peano4::data::ContainerRegistry::supportsSubtreeTaskReplication() const {
  return std::all_of(_containers.begin(), _containers.end(), [](const peano4::data::ManagedContainer* const container) {
    return container->supportsSubtreeTaskReplication();
  });
}

bool peano4::data::ContainerRegistry::arePayloadPoolsLive() const { return _payloadPoolsLive; }

/**
 * Marks the pooled payload buffers live and binds every container's pooled
 * storage. Values a record holds before its slot is bound move into the
 * bound chunk.
 */
void peano4::data::ContainerRegistry::allocatePayloadPools() {
  _payloadPoolsLive = true;
  runContainerTasks(_containers, [](peano4::data::ManagedContainer* const container) { container->bindPool(); });
}

/**
 * Returns the pool buffer bytes the registered containers' slots select.
 */
std::size_t peano4::data::ContainerRegistry::getPooledPayloadBytes() const {
  std::size_t bytes = 0;
  for (const auto* const container : _containers) {
    bytes += container->getPooledPayloadBytes();
  }
  return bytes;
}

void peano4::data::ContainerRegistry::resizePoolsForTree(
  const std::size_t cellCount,
  const std::size_t faceCount,
  const std::size_t vertexCount
) {
  runContainerTasks(_containers, [&](peano4::data::ManagedContainer* const container) {
    container->resizePoolForTree(cellCount, faceCount, vertexCount);
  });
}

void peano4::data::ContainerRegistry::migratePoolsForTree(
  const std::vector<std::pair<int, int>>& cellSlotPairs,
  const std::vector<std::pair<int, int>>& faceSlotPairs,
  const std::vector<std::pair<int, int>>& vertexSlotPairs,
  const std::size_t                       cellCount,
  const std::size_t                       faceCount,
  const std::size_t                       vertexCount
) {
  runContainerTasks(_containers, [&](peano4::data::ManagedContainer* const container) {
    container->migratePoolForTree(cellSlotPairs, faceSlotPairs, vertexSlotPairs, cellCount, faceCount, vertexCount);
  });
}

/**
 * Copies the interface pairs of every container. The pair list of every
 * container splits into chunks, and one loop over the chunks of all
 * containers runs them, so the call forks and joins once.
 */
void peano4::data::ContainerRegistry::replicateInterfaceSlots(
  const std::vector<std::pair<int, int>>& facePairs,
  const std::vector<std::pair<int, int>>& vertexPairs
) {
  const auto numberOfContainers = static_cast<int>(_containers.size());
  if (numberOfContainers == 0) {
    return;
  }
  const int numberOfThreads = std::max(1, tarch::tasking::Runtime::getInstance().getNumberOfThreads());
  const int chunksPerContainer =
    std::max(1, (ReplicationChunksPerThread * numberOfThreads + numberOfContainers - 1) / numberOfContainers);
  tarch::accelerator::parallelForOnHost(
    tarch::accelerator::Range(numberOfContainers * chunksPerContainer),
    [&](const int itemNumber) {
      _containers[itemNumber / chunksPerContainer]
        ->replicateInterfaceSlots(facePairs, vertexPairs, itemNumber % chunksPerContainer, chunksPerContainer);
    }
  );
}

void peano4::data::ContainerRegistry::packBoundarySlots(
  const std::vector<std::vector<int>>& facePackLists,
  const std::vector<std::vector<int>>& vertexPackLists
) {
  for (auto* const container : _containers) {
    container->packBoundarySlots(facePackLists, vertexPackLists);
  }
}

void peano4::data::ContainerRegistry::transferPoolsForTree(
  const std::map<int, std::vector<int>>& sendCellSlots,
  const std::map<int, std::vector<int>>& receiveCellSlots,
  const std::map<int, std::vector<int>>& sendFaceSlots,
  const std::map<int, std::vector<int>>& receiveFaceSlots,
  const std::map<int, std::vector<int>>& sendVertexSlots,
  const std::map<int, std::vector<int>>& receiveVertexSlots
) {
  for (auto* const container : _containers) {
    container->transferPoolSlotsForTree(
      sendCellSlots,
      receiveCellSlots,
      sendFaceSlots,
      receiveFaceSlots,
      sendVertexSlots,
      receiveVertexSlots
    );
  }
}

void peano4::data::ContainerRegistry::finishBoundaryExchange(
  const std::vector<std::vector<int>>& facePackLists,
  const std::vector<std::vector<int>>& vertexPackLists,
  const std::vector<int>&              faceFrozenSlotBases,
  const std::vector<int>&              vertexFrozenSlotBases
) {
  for (auto* const container : _containers) {
    container->finishBoundaryExchange(facePackLists, vertexPackLists, faceFrozenSlotBases, vertexFrozenSlotBases);
  }
}
