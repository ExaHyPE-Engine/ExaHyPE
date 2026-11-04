// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"

namespace tarch::accelerator::unittests {
  template <typename T>
  [[nodiscard]] INLINE_METHOD std::vector<T> getChunksOf(const tarch::accelerator::ChunkedMemoryPool<T>& pool);

  template <typename T>
  INLINE_METHOD void setChunksOf(tarch::accelerator::ChunkedMemoryPool<T>& pool, const std::vector<T>& values);
} // namespace tarch::accelerator::unittests

#include "unittests/tarch/accelerator/ChunkedMemoryPoolTestData.cpph"
