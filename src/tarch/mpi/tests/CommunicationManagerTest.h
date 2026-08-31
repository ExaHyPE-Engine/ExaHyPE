// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::mpi::tests {
  class CommunicationManagerTest;
} // namespace tarch::mpi::tests

class tarch::mpi::tests::CommunicationManagerTest final : public tarch::tests::TestCase {
public:
  CommunicationManagerTest();
  CommunicationManagerTest(const CommunicationManagerTest&)            = delete;
  CommunicationManagerTest(CommunicationManagerTest&&)                 = delete;
  CommunicationManagerTest& operator=(const CommunicationManagerTest&) = delete;
  CommunicationManagerTest& operator=(CommunicationManagerTest&&)      = delete;
  ~CommunicationManagerTest() override                                 = default;

  void run() override;

private:
  void testSerialProgressRunsEveryPollerOnce();
  void testSerialFinishPendingWorkRunsUntilIdle();
  void testThreadedManagerPollsWithoutProgressCalls();
  void testThreadedFinishPendingWorkWaitsForAnIdlePass();
  void testThreadedFinishPendingWorkAfterStopRunsInline();
  void testDedicatedThreadNeedsAPeerAndAConfiguration();
};
