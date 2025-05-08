// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class EnclaveBookkeepingTest;
}

class exahype2::tests::EnclaveBookkeepingTest final: public tarch::tests::TestCase {
private:
  void testFusePendingTasks();

public:
  EnclaveBookkeepingTest();
  ~EnclaveBookkeepingTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
