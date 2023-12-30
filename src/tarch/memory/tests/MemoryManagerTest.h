// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::memory::tests {
  class MemoryManagerTest;
} // namespace tarch::memory::tests

class tarch::memory::tests::MemoryManagerTest: public tarch::tests::TestCase {
private:
  void testMemoryAllocationsForOneDevice();
  void testMemoryAllocationsForMultipleDevices();

public:
  MemoryManagerTest();
  virtual ~MemoryManagerTest() override = default;

  virtual void run() override;
};
