// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class AtomicTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::AtomicTest final : public tarch::tests::TestCase {
public:
  AtomicTest();
  AtomicTest(const AtomicTest&)            = delete;
  AtomicTest(AtomicTest&&)                 = delete;
  AtomicTest& operator=(const AtomicTest&) = delete;
  AtomicTest& operator=(AtomicTest&&)      = delete;
  ~AtomicTest() override                   = default;

  void run() override;

private:
  void testSerialOperations();
  void testConcurrentOperations();
  void testLoadStoreRoundTrip();
  void testConcurrentMaximumIsOrderIndependent();
};
