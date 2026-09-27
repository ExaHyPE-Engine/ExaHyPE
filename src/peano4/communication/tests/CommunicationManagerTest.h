// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::communication::tests {
  class CommunicationManagerTest;
} // namespace peano4::communication::tests

/**
 * Checks that the manager hands out distinct tags every rank agrees on and completes the
 * transfers it polls.
 */
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
  void testTagsAreDistinctAndEveryRankAgrees();
  void testTheExchangeCommunicatorIsSeparate();
  void testAPolledTransferCompletes();
};
