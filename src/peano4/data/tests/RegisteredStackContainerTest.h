// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::data::tests {
  class RegisteredStackContainerTest;
} // namespace peano4::data::tests

class peano4::data::tests::RegisteredStackContainerTest final : public tarch::tests::TestCase {
public:
  RegisteredStackContainerTest();
  RegisteredStackContainerTest(const RegisteredStackContainerTest&)            = delete;
  RegisteredStackContainerTest(RegisteredStackContainerTest&&)                 = delete;
  RegisteredStackContainerTest& operator=(const RegisteredStackContainerTest&) = delete;
  RegisteredStackContainerTest& operator=(RegisteredStackContainerTest&&)      = delete;
  ~RegisteredStackContainerTest() override                                     = default;

  void run() override;

private:
  void testEachKindSizesAgainstItsOwnEntityCount();
  void testMigrationMovesTheDataToItsNewSlots();
  void testReplicationCopiesOnlyTheNamedSlots();
  void testReplicationChunksPartitionThePairs();
  void testCellDataStaysOutOfTheBoundaryExchange();
  void testTheContainerJoinsAndLeavesTheRegistry();
  void testAnElementTypeCanRefuseTaskReplication();
  void testClearReleasesThePool();
  void testTaskSlotsFollowTheRankStride();
};
