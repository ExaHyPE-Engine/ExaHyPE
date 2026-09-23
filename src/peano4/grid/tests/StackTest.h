// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class StackTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::StackTest final : public tarch::tests::TestCase {
public:
  StackTest();

  void run() override;

private:
  void testPush();
  void testPop();
  void testTop();
  void testResize();
  void testClear();
  void testPushBlock();
  void testReusedSlotDropsPreviousValue();
};
