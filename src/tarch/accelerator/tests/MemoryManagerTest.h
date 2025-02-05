// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class MemoryManagerTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::MemoryManagerTest:
  public tarch::tests::TestCase {
private:
  void testMemoryAllocationsForOneDevice();
  void testMemoryAllocationsForMultipleDevices();

public:
  MemoryManagerTest();
  virtual ~MemoryManagerTest() override = default;

  virtual void run() override;
};
