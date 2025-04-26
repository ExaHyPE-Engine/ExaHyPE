// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::multicore::taskfusion::tests {
  class TaskFusionTest;
} // namespace tarch::multicore::taskfusion::tests

class tarch::multicore::taskfusion::tests::TaskFusionTest final: public tarch::tests::TestCase {
private:
  void testTranslateFusableTaskIntoTaskSequence();

public:
  TaskFusionTest();
  ~TaskFusionTest() override final = default;

  /**
   * This routine is triggered by the TestCaseCollection
   */
  void run() override final;
};
