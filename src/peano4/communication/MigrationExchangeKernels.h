// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/accelerator.h"

namespace peano4 {
  struct Prolongation;
  struct Restriction;
  struct TransferredCell;
} // namespace peano4

namespace peano4::communication {
  template <typename Transfer>
  struct PrepareProlongations;

  template <typename Transfer>
  struct PrepareRestrictions;

  template <typename Transfer>
  struct PrepareSentCells;

  template <typename T, typename Transfer>
  struct ProlongChunks;

  template <typename T, typename Transfer>
  struct RestrictChunks;

  template <typename T, typename Transfer>
  struct GatherChunks;

  template <typename T>
  struct ScatterChunks;
} // namespace peano4::communication

TARGET_DECLARE
/**
 * Writes the values that the transfer prepares from the parents of the prolongations. Work item w
 * writes the item w % Transfer::PreparationItemsPerChunk of the prolongation
 * e = w / Transfer::PreparationItemsPerChunk at prepared + e * Transfer::PreparedValuesPerChunk.
 */
template <typename Transfer>
struct peano4::communication::PrepareProlongations final {
  const peano4::Prolongation* prolongations{nullptr};
  Transfer                    transfer{};
  double*                     prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Writes the values that the transfer prepares from the children of the restrictions. Work item w
 * writes the item w % Transfer::PreparationItemsPerChunk of the restriction
 * e = w / Transfer::PreparationItemsPerChunk at prepared + e * Transfer::PreparedValuesPerChunk.
 */
template <typename Transfer>
struct peano4::communication::PrepareRestrictions final {
  const peano4::Restriction* restrictions{nullptr};
  Transfer                   transfer{};
  double*                    prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Writes the values that the transfer prepares from the parent or the children of the sent cells.
 * Work item w writes the item w % Transfer::PreparationItemsPerChunk of the sent cell
 * e = w / Transfer::PreparationItemsPerChunk at prepared + e * Transfer::PreparedValuesPerChunk.
 * Only the work items of sent cells write values.
 */
template <typename Transfer>
struct peano4::communication::PrepareSentCells final {
  const peano4::TransferredCell* cells{nullptr};
  Transfer                       transfer{};
  double*                        prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Fills the chunks of the children from the chunks of their parents. Work item w fills the item
 * w % Transfer::WorkItemsPerChunk of the child of the prolongation
 * e = w / Transfer::WorkItemsPerChunk. The transfer reads the prepared values of e at
 * prepared + e * Transfer::PreparedValuesPerChunk.
 */
template <typename T, typename Transfer>
struct peano4::communication::ProlongChunks final {
  const peano4::Prolongation*                             prolongations{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  Transfer                                                transfer{};
  const double*                                           prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Fills the chunks of the parents from the chunks of their children. Work item w fills the item
 * w % Transfer::WorkItemsPerChunk of the parent of the restriction
 * e = w / Transfer::WorkItemsPerChunk. The transfer reads the prepared values of e at
 * prepared + e * Transfer::PreparedValuesPerChunk.
 */
template <typename T, typename Transfer>
struct peano4::communication::RestrictChunks final {
  const peano4::Restriction*                              restrictions{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  Transfer                                                transfer{};
  const double*                                           prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Writes the chunks of the sent cells into the send buffer. Work item w writes the item
 * w % Transfer::WorkItemsPerChunk of the sent cell e = w / Transfer::WorkItemsPerChunk. The
 * transfer reads the prepared values of e at prepared + e * Transfer::PreparedValuesPerChunk.
 */
template <typename T, typename Transfer>
struct peano4::communication::GatherChunks final {
  const peano4::TransferredCell* cells{nullptr};
  T*                             send{nullptr};
  Transfer                       transfer{};
  int                            chunkSize{0};
  const double*                  prepared{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int workItem) const;
};

/**
 * Copies one received element into the slot of its cell.
 */
template <typename T>
struct peano4::communication::ScatterChunks final {
  const int*                                              slots{nullptr};
  const T*                                                received{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  int                                                     chunkSize{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int index) const;
};
TARGET_DECLARE_END

#include "peano4/communication/MigrationExchangeKernels.cpph"
