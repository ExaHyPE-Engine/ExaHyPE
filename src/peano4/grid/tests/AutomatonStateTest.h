// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class AutomatonStateTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::AutomatonStateTest final : public tarch::tests::TestCase {
public:
  AutomatonStateTest();
  ~AutomatonStateTest() override final = default;

  void run() override final;

private:
  void testFlags();
  void testRefineState();
  void testSendAndReceiveInARing();
};
