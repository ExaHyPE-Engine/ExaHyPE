// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch {
  namespace accelerator {
    namespace tests {
      class MemoryAllocationTest;
    } // namespace tests
  } // namespace accelerator
} // namespace tarch

class tarch::accelerator::tests::MemoryAllocationTest final: public tarch::tests::TestCase {
private:
  void testAllocateRawData();
  void testAllocateMemory();
  void testLocationSpecificAllocators();
  void testLocationSpecificCopyFunctions();

public:
  MemoryAllocationTest();
  ~MemoryAllocationTest() override = default;

  void run() override;
};
