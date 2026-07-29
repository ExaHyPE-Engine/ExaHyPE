// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::stacks::tests {
  class StackTest;
} // namespace peano4::stacks::tests

class peano4::stacks::tests::StackTest final : public tarch::tests::TestCase {
public:
  StackTest();
  ~StackTest() override final = default;

  void run() override final;

private:
  void testPushData();
  void testEraseData();
  void testClearData();
  void testEmplaceData();
  void testPopData();
  void testClone();
  void testReusedSlotAssignmentClearsEmptyState();
  void testPushBlockView();
  void testPopBlockView();
  void testSendAndReceiveToSelf();
  void testSendAndReceiveInARing();
};
