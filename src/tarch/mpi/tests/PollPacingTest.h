// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class PollPacingTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::PollPacingTest final : public tarch::tests::TestCase {
public:
  PollPacingTest();
  PollPacingTest(const PollPacingTest&)            = delete;
  PollPacingTest(PollPacingTest&&)                 = delete;
  PollPacingTest& operator=(const PollPacingTest&) = delete;
  PollPacingTest& operator=(PollPacingTest&&)      = delete;
  ~PollPacingTest() override                       = default;

  void run() override;

private:
  void testFirstPollReportsProgress();
  void testTurnsWidenWhileTheWaitLasts();
  void testStrideSaturatesSoReportingContinues();
  void testEveryWaitStartsTight();
};
