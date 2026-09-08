// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace runner::tests {
  class RunControlTest;
} // namespace runner::tests

class runner::tests::RunControlTest final : public tarch::tests::TestCase {
public:
  RunControlTest();
  RunControlTest(const RunControlTest&)            = delete;
  RunControlTest(RunControlTest&&)                 = delete;
  RunControlTest& operator=(const RunControlTest&) = delete;
  RunControlTest& operator=(RunControlTest&&)      = delete;
  ~RunControlTest() override                       = default;

  void run() override;

private:
  void testProgramStepBroadcast();
  void testTerminationBroadcast();
};
