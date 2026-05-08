// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace accelerator {
    namespace tests {
      class MemoryAllocationCacheTest;
    } // namespace tests
  } // namespace accelerator
} // namespace tarch

class tarch::accelerator::tests::MemoryAllocationCacheTest final: public tarch::tests::TestCase {
private:
  void testReuseHeapAllocations();
  void testReuseDeviceAllocations();
  void testReusePinnedAllocations();

public:
  MemoryAllocationCacheTest();
  ~MemoryAllocationCacheTest() override = default;

  void run() override;
};
