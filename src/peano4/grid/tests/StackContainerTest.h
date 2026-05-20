// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace peano4::grid::tests {
  class StackContainerTest;
} // namespace peano4::grid::tests

class peano4::grid::tests::StackContainerTest final: public tarch::tests::TestCase {
private:
  void testEmptyContainerHasZeroSize();
  void testSizeCountsElementInLocalAndParallelStacks();
  void testSetLocalTreesPreservesExistingTreeDataWhenAddingNewTrees();
  void testSetLocalTreesDropsRemovedTrees();
  void testMultipleParallelSlotsIndependentAndCounted();

public:
  StackContainerTest();
  ~StackContainerTest() override final = default;

  void run() override final;
};
