// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::communication::tests {
  class CommunicationManagerTest;
} // namespace peano4::communication::tests

class peano4::communication::tests::CommunicationManagerTest final : public tarch::tests::TestCase {
public:
  CommunicationManagerTest();
  CommunicationManagerTest(const CommunicationManagerTest&)            = delete;
  CommunicationManagerTest(CommunicationManagerTest&&)                 = delete;
  CommunicationManagerTest& operator=(const CommunicationManagerTest&) = delete;
  CommunicationManagerTest& operator=(CommunicationManagerTest&&)      = delete;
  ~CommunicationManagerTest() override                                 = default;

  void run() override;

private:
  void testTagsAreDistinctPerContainerAndMessage();
  void testTagsAreStableAcrossQueries();
  void testEveryRankAgreesOnTheTags();
  void testAnUnchangedRankSetKeepsTheVersion();
  void testEveryChangedRankSetBumpsTheVersion();
  void testTheRankSetIsHandedBackUnchanged();
  void testATopologyWithoutTransfersReportsItselfIdle();
  void testSweepsAndPollingLeaveTheTopologyAlone();
  void testATransferReachesTheReceivingSlots();
};
