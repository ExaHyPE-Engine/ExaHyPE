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
/** Fills the chunk of one child from the chunk of its parent. */
template <typename T, typename Transfer>
struct peano4::communication::ProlongChunks final {
  const peano4::Prolongation*                             prolongations{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  Transfer                                                transfer{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/** Fills the chunk of one parent from the chunks of its children. */
template <typename T, typename Transfer>
struct peano4::communication::RestrictChunks final {
  const peano4::Restriction*                              restrictions{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  Transfer                                                transfer{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/** Writes the chunk of one sent cell into the send buffer. */
template <typename T, typename Transfer>
struct peano4::communication::GatherChunks final {
  const peano4::TransferredCell* cells{nullptr};
  T*                             send{nullptr};
  Transfer                       transfer{};
  int                            chunkSize{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};

/** Copies one received element into the slot of its cell. */
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
