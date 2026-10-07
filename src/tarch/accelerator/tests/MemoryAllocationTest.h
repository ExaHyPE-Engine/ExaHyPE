// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class MemoryAllocationTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::MemoryAllocationTest final : public tarch::tests::TestCase {
public:
  MemoryAllocationTest();
  MemoryAllocationTest(const MemoryAllocationTest& other)            = delete;
  MemoryAllocationTest(MemoryAllocationTest&& other)                 = delete;
  MemoryAllocationTest& operator=(const MemoryAllocationTest& other) = delete;
  MemoryAllocationTest& operator=(MemoryAllocationTest&& other)      = delete;
  ~MemoryAllocationTest() override                                   = default;

  void run() override;

private:
  void testAllocateRawData();
  void testAllocateMemory();
  void testLocationSpecificAllocators();
  void testLocationSpecificCopyFunctions();
  void testTheCurrentDeviceHoldsForEveryHostThread();
  void testOnlyTheDeviceLocationOfAGPUBuildIsGPUMemory();
};
