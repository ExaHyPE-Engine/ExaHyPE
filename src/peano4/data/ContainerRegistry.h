// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::data {
  class ManagedContainer;
  class ContainerRegistry;
} // namespace peano4::data

/**
 * Exchange interface of one registered data container.
 */
class peano4::data::ManagedContainer {
public:
  ManagedContainer()                                   = default;
  ManagedContainer(const ManagedContainer&)            = default;
  ManagedContainer(ManagedContainer&&)                 = default;
  ManagedContainer& operator=(const ManagedContainer&) = default;
  ManagedContainer& operator=(ManagedContainer&&)      = default;
  virtual ~ManagedContainer()                          = default;

  /** Returns the small number that addresses this container in the exchange tags. */
  virtual int getManagedContainerId() const = 0;

  /** Returns whether the element type may be replicated into one copy per subtree task. */
  virtual bool supportsSubtreeTaskReplication() const = 0;

  /** Anchors the slot the serial traversal and the master write, once the rank is known. */
  virtual void initialiseSerialSlot() = 0;

  /** Sizes the pool to the entity count of this container's kind. */
  virtual void resizePoolForTree(std::size_t cellCount, std::size_t faceCount, std::size_t vertexCount) = 0;

  /** Moves every surviving entity's data to the slot it holds in the new structure. */
  virtual void migratePoolForTree(
    const std::vector<std::pair<int, int>>& cellSlotPairs,
    const std::vector<std::pair<int, int>>& faceSlotPairs,
    const std::vector<std::pair<int, int>>& vertexSlotPairs,
    std::size_t                             cellCount,
    std::size_t                             faceCount,
    std::size_t                             vertexCount
  ) = 0;

  /**
   * Copies each named source slot of one chunk onto its target slot. The
   * pair list of this container's kind splits into numberOfChunks ranges of
   * equal size, and the call copies the range chunkNumber.
   */
  virtual void replicateInterfaceSlots(
    const std::vector<std::pair<int, int>>& facePairs,
    const std::vector<std::pair<int, int>>& vertexPairs,
    int                                     chunkNumber,
    int                                     numberOfChunks
  ) = 0;

  /** Copies the rank-boundary slots into the outgoing buffers and posts them. */
  virtual void packBoundarySlots(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists
  ) = 0;

  /** Moves the data of repartitioned entities to their new owner rank. */
  virtual void transferPoolSlotsForTree(
    const std::map<int, std::vector<int>>& sendCellSlots,
    const std::map<int, std::vector<int>>& receiveCellSlots,
    const std::map<int, std::vector<int>>& sendFaceSlots,
    const std::map<int, std::vector<int>>& receiveFaceSlots,
    const std::map<int, std::vector<int>>& sendVertexSlots,
    const std::map<int, std::vector<int>>& receiveVertexSlots
  ) = 0;

  /** Waits for the neighbours' messages and unpacks them into the frozen slots. */
  virtual void finishBoundaryExchange(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists,
    const std::vector<int>&              faceFrozenSlotBases,
    const std::vector<int>&              vertexFrozenSlotBases
  ) = 0;

  /** Grows the per-task call blocks to the number of walk units the sweep spawns. */
  virtual void prepareLocalSlots(int numberOfSlots) = 0;
};

/**
 * Registry of every data container of the rank. Containers register in their
 * constructors, and the sweep machinery drives every exchange over the
 * registered set. The exchange operations visit the containers in container-id
 * order, so all ranks derive matching message tags; the local pool operations
 * carry no such order and run as concurrent tasks.
 */
class peano4::data::ContainerRegistry final {
public:
  ContainerRegistry(const ContainerRegistry&)            = delete;
  ContainerRegistry(ContainerRegistry&&)                 = delete;
  ContainerRegistry& operator=(const ContainerRegistry&) = delete;
  ContainerRegistry& operator=(ContainerRegistry&&)      = delete;
  ~ContainerRegistry()                                   = default;

  static peano4::data::ContainerRegistry& getInstance();

  void attachContainer(peano4::data::ManagedContainer* container);
  void detachContainer(const peano4::data::ManagedContainer* container);

  bool supportsSubtreeTaskReplication() const;

  void initialiseSerialSlots();
  void resizePoolsForTree(std::size_t cellCount, std::size_t faceCount, std::size_t vertexCount);
  void migratePoolsForTree(
    const std::vector<std::pair<int, int>>& cellSlotPairs,
    const std::vector<std::pair<int, int>>& faceSlotPairs,
    const std::vector<std::pair<int, int>>& vertexSlotPairs,
    std::size_t                             cellCount,
    std::size_t                             faceCount,
    std::size_t                             vertexCount
  );
  void replicateInterfaceSlots(
    const std::vector<std::pair<int, int>>& facePairs,
    const std::vector<std::pair<int, int>>& vertexPairs
  );
  void packBoundarySlots(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists
  );
  void transferPoolsForTree(
    const std::map<int, std::vector<int>>& sendCellSlots,
    const std::map<int, std::vector<int>>& receiveCellSlots,
    const std::map<int, std::vector<int>>& sendFaceSlots,
    const std::map<int, std::vector<int>>& receiveFaceSlots,
    const std::map<int, std::vector<int>>& sendVertexSlots,
    const std::map<int, std::vector<int>>& receiveVertexSlots
  );
  void finishBoundaryExchange(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists,
    const std::vector<int>&              faceFrozenSlotBases,
    const std::vector<int>&              vertexFrozenSlotBases
  );
  void prepareLocalSlots(int numberOfSlots);

private:
  ContainerRegistry() = default;

  /** Registered containers, ascending by container id. */
  std::vector<peano4::data::ManagedContainer*> _containers{};
};
