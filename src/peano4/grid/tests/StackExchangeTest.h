// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class StackExchangeTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::StackExchangeTest final : public tarch::tests::TestCase {
public:
  StackExchangeTest();

  void run() override;

private:
  void testSendAndReceiveToSelf();
  void testSendAndReceiveInARing();
};
