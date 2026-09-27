// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class HelperThreadTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::HelperThreadTest final : public tarch::tests::TestCase {
public:
  HelperThreadTest();
  HelperThreadTest(const HelperThreadTest&)            = delete;
  HelperThreadTest(HelperThreadTest&&)                 = delete;
  HelperThreadTest& operator=(const HelperThreadTest&) = delete;
  HelperThreadTest& operator=(HelperThreadTest&&)      = delete;
  ~HelperThreadTest() override                         = default;

  void run() override;

private:
  void testPollsUntilTheFunctionReportsCompletion();
  void testStopEndsAPollThatNeverCompletes();
  void testStopWithoutStartIsHarmless();
};
