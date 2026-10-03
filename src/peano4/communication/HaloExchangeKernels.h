// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/accelerator.h"

namespace peano4 {
  struct ExchangedFace;
} // namespace peano4

namespace peano4::communication {
  template <typename T>
  struct GatherHaloHalves;

  template <typename T>
  struct ScatterHaloHalves;
} // namespace peano4::communication

TARGET_DECLARE
/**
 * Copies the half of every exchanged face that this rank owns into the send buffer.
 */
template <typename T>
struct peano4::communication::GatherHaloHalves final {
  typename tarch::accelerator::ChunkedMemoryPool<T>::ConstView pool{};
  const peano4::ExchangedFace*                                 entries{nullptr};
  T*                                                           send{nullptr};
  int                                                          halfSize{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int index) const;
};

/**
 * Copies the received half of every exchanged face into the side the neighbour owns.
 */
template <typename T>
struct peano4::communication::ScatterHaloHalves final {
  const peano4::ExchangedFace*                            entries{nullptr};
  const T*                                                received{nullptr};
  typename tarch::accelerator::ChunkedMemoryPool<T>::View pool{};
  int                                                     halfSize{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int index) const;
};
TARGET_DECLARE_END

#include "peano4/communication/HaloExchangeKernels.cpph"
