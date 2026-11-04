// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::tests {
  class MemoryUsageTest;
} // namespace tarch::tests

class tarch::tests::MemoryUsageTest final : public tarch::tests::TestCase {
public:
  MemoryUsageTest();
  MemoryUsageTest(const MemoryUsageTest& other)            = delete;
  MemoryUsageTest(MemoryUsageTest&& other)                 = delete;
  MemoryUsageTest& operator=(const MemoryUsageTest& other) = delete;
  MemoryUsageTest& operator=(MemoryUsageTest&& other)      = delete;
  ~MemoryUsageTest() override                              = default;

  void run() override;

private:
  void testAllocatedBytesCountTheCapacity();
  void testFormatPicksTheLargestReachedUnit();
  void testProcessAndNodeMemoryAreConsistent();
};
