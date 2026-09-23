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

  void run() override;

private:
  void testAllocateRawData();
  void testAllocateMemory();
  void testZeroedHeapMemory();
  void testLocationSpecificAllocators();
  void testLocationSpecificCopyFunctions();
};
