// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::communication::tests {
  class HaloExchangeKernelsTest;
} // namespace peano4::communication::tests

/** Checks that the gather copies the own half of every listed face and the scatter fills the other half. */
class peano4::communication::tests::HaloExchangeKernelsTest final : public tarch::tests::TestCase {
public:
  HaloExchangeKernelsTest();
  HaloExchangeKernelsTest(const HaloExchangeKernelsTest&)            = delete;
  HaloExchangeKernelsTest(HaloExchangeKernelsTest&&)                 = delete;
  HaloExchangeKernelsTest& operator=(const HaloExchangeKernelsTest&) = delete;
  HaloExchangeKernelsTest& operator=(HaloExchangeKernelsTest&&)      = delete;
  ~HaloExchangeKernelsTest() override                                = default;

  void run() override;

private:
  void testGatherCopiesTheOwnHalfOfEveryListedFace();
  void testScatterFillsTheHalfTheNeighbourOwns();
};
