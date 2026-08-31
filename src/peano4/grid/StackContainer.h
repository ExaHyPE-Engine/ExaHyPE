// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/BoundaryDataAggregation.h"
#include "peano4/grid/StackTopology.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/ParallelFor.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  template <class StackType>
  class StackContainer;
} // namespace peano4::grid

/**
 * Storage of one data container on the rank's tree.
 *
 * Holds the flat entity pool a traversal addresses by pool slot, one observer
 * call block per subtree task, and the buffers of the aggregated boundary
 * exchange. The tree id every call carries selects the call block by
 * arithmetic: The rank's own id names slot zero, which the serial traversal
 * and the master both use, and a subtree task passes its own id one above it.
 */
template <class StackType>
class peano4::grid::StackContainer {
public:
  using Stack       = StackType;
  using ElementType = std::decay_t<decltype(std::declval<StackType&>().pop())>;

  StackContainer() = default;

  explicit StackContainer(const int containerId);

  StackContainer(const StackContainer&)            = delete;
  StackContainer(StackContainer&&)                 = delete;
  StackContainer& operator=(const StackContainer&) = delete;
  StackContainer& operator=(StackContainer&&)      = delete;
  ~StackContainer()                                = default;

  int getContainerId() const { return _containerId; }

  BoundaryDataAggregation<StackType>& getBoundaryDataAggregation() { return _boundaryDataAggregation; }

  static constexpr auto LocalStackCount = StackTopology::TotalNumberOfLocalExchangeStacks;

  using LocalStacksPerTree = std::array<Stack, static_cast<std::size_t>(LocalStackCount)>;

  Stack& getLocalExchangeStack(const int treeId, const int localStackId);

  INLINE_METHOD ElementType& getPoolSlot(const int poolIndex);

  INLINE_METHOD std::size_t getPoolSize() const;

  void resizePool(const std::size_t entityCount);

  void replicatePoolSlots(
    const std::vector<std::pair<int, int>>& sourceAndTargetSlots,
    int                                     chunkNumber,
    int                                     numberOfChunks
  );

  void migratePoolSlots(const std::vector<std::pair<int, int>>& sourceAndTargetSlots, const std::size_t entityCount);

  /**
   * Creates the serial slot and anchors the tree-id mapping on the rank.
   * Runs once at start-up, so no getter has to bootstrap storage lazily.
   */
  void initialiseSerialSlot();

  /**
   * Grows the local slot storage to the requested count. Slot growth is
   * serial: The engine grows before it spawns tasks, so concurrent slot
   * accesses never mutate the storage.
   */
  void growLocalSlots(const int numberOfSlots);

  std::size_t size() const;

  void clear();

private:
  std::size_t localSlotIndex(const int treeId) const;

  /**
   * Local stacks per slot: Slot zero serves the rank's serial traversal and
   * the spine, one further slot serves each subtree task. A deque never
   * relocates its entries when it grows, so references handed out earlier
   * stay valid.
   */
  std::deque<LocalStacksPerTree> _localSlotStacks{};

  /** Flat per-rank entity storage of the spacetree. */
  std::vector<ElementType> _pool{};

  /** Stores the tree id of slot zero, which initialiseSerialSlot() resolves at start-up. */
  int _firstLocalSlot{-1};

  /** Stores the rank-count stride between the rank's local slots. */
  int _localSlotStride{1};

  BoundaryDataAggregation<StackType> _boundaryDataAggregation;

  int _containerId{-1};
};

#include "peano4/grid/StackContainer.cpph"
