// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::tests {
  class HaloExchangeTest;
} // namespace peano4::tests

/**
 * Checks that an exchange fills the remote half of every exchanged face with the half the neighbour
 * rank wrote, leaves every other face unchanged and works again after a rebuild.
 */
class peano4::tests::HaloExchangeTest final : public tarch::tests::TestCase {
public:
  HaloExchangeTest();
  HaloExchangeTest(const HaloExchangeTest&)            = delete;
  HaloExchangeTest(HaloExchangeTest&&)                 = delete;
  HaloExchangeTest& operator=(const HaloExchangeTest&) = delete;
  HaloExchangeTest& operator=(HaloExchangeTest&&)      = delete;
  ~HaloExchangeTest() override                         = default;

  void run() override;

private:
  void testExchangeFillsTheRemoteHalves();
  void testExchangesHoldDistinctTags();
  void testReleaseFreesTheBuffersAndKeepsTheTag();
};
