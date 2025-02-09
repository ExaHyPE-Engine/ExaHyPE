// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class EnclaveTaskNumberPoolTest;
}

class exahype2::tests::EnclaveTaskNumberPoolTest final: public tarch::tests::TestCase {
private:
  void testAccessToPool(
    int timeSteps,
    int taskLoopIterations,
    int poolRefillSize,
    int numberOfTaskNumbersToAcquire
  );

public:
  EnclaveTaskNumberPoolTest();
  ~EnclaveTaskNumberPoolTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
