// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/grid/BoundaryDataAggregation.h"
#include "tarch/Assertions.h"
#include "tarch/Termination.h"
#include "tarch/accelerator/ParallelFor.h"

namespace peano4::grid {
  template <class StackType>
  class StackContainer;
} // namespace peano4::grid

/**
 * Stores one rank's entity slots and aggregated boundary data.
 *
 * The last slot is a sentinel for positions without a rank-local entity.
 */
template <class StackType>
class peano4::grid::StackContainer {
public:
  using Stack       = StackType;
  using ElementType = typename StackType::ElementType;

  explicit StackContainer(int containerId);
  StackContainer(const StackContainer&)            = delete;
  StackContainer(StackContainer&&)                 = delete;
  StackContainer& operator=(const StackContainer&) = delete;
  StackContainer& operator=(StackContainer&&)      = delete;
  ~StackContainer()                                = default;

  INLINE_METHOD int getContainerId() const;

  INLINE_METHOD BoundaryDataAggregation<StackType>& getBoundaryDataAggregation();

  INLINE_METHOD ElementType& getPoolSlot(int poolIndex);
  INLINE_METHOD ElementType* getPoolData();
  INLINE_METHOD int          getSentinelSlot() const;
  INLINE_METHOD std::size_t getPoolSize() const;

  void resizePool(std::size_t entityCount, bool bindPooledData);

  void bindPool();

  void replicatePoolSlots(
    const std::vector<std::pair<int, int>>& sourceAndTargetSlots,
    int                                     chunkNumber,
    int                                     numberOfChunks
  );

  void migratePoolSlots(
    const std::vector<std::pair<int, int>>& sourceAndTargetSlots,
    std::size_t                             entityCount,
    bool                                    bindPooledData
  );

  void clear();

private:
  void bindPooledStorage();
  void resetSentinel();

  //! Stores the rank-local entity slots followed by the sentinel.
  std::vector<ElementType> _pool{};

  BoundaryDataAggregation<StackType> _boundaryDataAggregation{};

  int _containerId;
};

#include "peano4/grid/StackContainer.cpph"
