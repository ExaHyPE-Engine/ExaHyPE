// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::accelerator::tests {
  class AtomicsTest;
} // namespace tarch::accelerator::tests

class tarch::accelerator::tests::AtomicsTest final : public tarch::tests::TestCase {
public:
  AtomicsTest();

  void run() override;

  // NVCC requires the enclosing function of an extended device lambda to be public.
  void testConcurrentMaximumIsOrderIndependent();

private:
  void testRaiseKeepsTheMaximum();
  void testLoadStoreRoundTrip();
};
