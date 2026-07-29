// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::parallel::tests {
  class ExchangeChannelsTest;
} // namespace peano4::parallel::tests

class peano4::parallel::tests::ExchangeChannelsTest final : public tarch::tests::TestCase {
public:
  ExchangeChannelsTest();
  void run() override;

private:
  void testTagCalculation();
  void testDataExchangeCommunicatorCreation();
};
