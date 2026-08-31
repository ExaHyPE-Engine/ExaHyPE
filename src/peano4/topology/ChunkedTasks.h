// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/StdAfx.h"

namespace peano4::topology {
  constexpr int DefaultMinimumItemsPerChunk = 4096;

  int maximumNumberOfChunks();

  int numberOfChunks(int numberOfItems, int minimumItemsPerChunk = DefaultMinimumItemsPerChunk);

  void runChunkedTasks(
    int                                       numberOfItems,
    const std::function<void(int, int, int)>& processRange,
    int                                       minimumItemsPerChunk = DefaultMinimumItemsPerChunk
  );
} // namespace peano4::topology
