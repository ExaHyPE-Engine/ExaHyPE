// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/communication/CommunicationManager.h"
#include "peano4/data/ContainerRegistry.h"
#include "peano4/grid/StackContainer.h"

namespace peano4::data {
  enum class ManagedContainerKind { CellData, FaceData, VertexData };

  template <class StackType>
  class RegisteredStackContainer;
} // namespace peano4::data

/**
 * Registers a stack container on construction, so the sweep machinery reaches
 * it without any generated call. The kind selects the entity count a pool
 * operation sizes against. Only face and vertex containers take part in the
 * per-sweep boundary exchange, and a repartition moves cell data as well.
 */
template <class StackType>
class peano4::data::RegisteredStackContainer final : public peano4::grid::StackContainer<StackType>,
                                                     public peano4::data::ManagedContainer {
public:
  INLINE_METHOD RegisteredStackContainer(int containerId, peano4::data::ManagedContainerKind kind);
  RegisteredStackContainer(const RegisteredStackContainer&)            = delete;
  RegisteredStackContainer(RegisteredStackContainer&&)                 = delete;
  RegisteredStackContainer& operator=(const RegisteredStackContainer&) = delete;
  RegisteredStackContainer& operator=(RegisteredStackContainer&&)      = delete;
  INLINE_METHOD ~RegisteredStackContainer() override;

  INLINE_METHOD int getManagedContainerId() const override;

  INLINE_METHOD bool supportsSubtreeTaskReplication() const override;

  INLINE_METHOD void initialiseSerialSlot() override;
  INLINE_METHOD void resizePoolForTree(std::size_t cellCount, std::size_t faceCount, std::size_t vertexCount) override;
  INLINE_METHOD void migratePoolForTree(
    const std::vector<std::pair<int, int>>& cellSlotPairs,
    const std::vector<std::pair<int, int>>& faceSlotPairs,
    const std::vector<std::pair<int, int>>& vertexSlotPairs,
    std::size_t                             cellCount,
    std::size_t                             faceCount,
    std::size_t                             vertexCount
  ) override;
  INLINE_METHOD void replicateInterfaceSlots(
    const std::vector<std::pair<int, int>>& facePairs,
    const std::vector<std::pair<int, int>>& vertexPairs,
    int                                     chunkNumber,
    int                                     numberOfChunks
  ) override;
  INLINE_METHOD void packBoundarySlots(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists
  ) override;
  INLINE_METHOD void transferPoolSlotsForTree(
    const std::map<int, std::vector<int>>& sendCellSlots,
    const std::map<int, std::vector<int>>& receiveCellSlots,
    const std::map<int, std::vector<int>>& sendFaceSlots,
    const std::map<int, std::vector<int>>& receiveFaceSlots,
    const std::map<int, std::vector<int>>& sendVertexSlots,
    const std::map<int, std::vector<int>>& receiveVertexSlots
  ) override;
  INLINE_METHOD void finishBoundaryExchange(
    const std::vector<std::vector<int>>& facePackLists,
    const std::vector<std::vector<int>>& vertexPackLists,
    const std::vector<int>&              faceFrozenSlotBases,
    const std::vector<int>&              vertexFrozenSlotBases
  ) override;
  INLINE_METHOD void prepareLocalSlots(int numberOfSlots) override;

  INLINE_METHOD void clear();

private:
  const peano4::data::ManagedContainerKind _kind;

  /** Holds received repartition data until migration swaps the pool. */
  std::vector<std::pair<int, typename peano4::grid::StackContainer<StackType>::ElementType>> _pendingPoolTransfers{};
};

#include "peano4/data/RegisteredStackContainer.cpph"
