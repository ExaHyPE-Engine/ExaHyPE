// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackContainerTest.h"

#include "peano4/grid/PeriodicTopology.h"
#include "peano4/grid/StackContainer.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/stacks/Stack.h"

namespace {
  peano4::grid::PeriodicStamp faceStampAt(int transverseIndex, peano4::grid::PeriodicSide side) {
    constexpr int                      Level = 2;
    tarch::la::Vector<DIMENSIONS, int> cellIndex(transverseIndex);
    cellIndex(0) = side == peano4::grid::PeriodicSide::Low ? 0 : peano4::grid::cellsPerAxis(Level) - 1;
    return peano4::grid::makeFaceStamp(Level, cellIndex, side == peano4::grid::PeriodicSide::Low ? 0 : DIMENSIONS);
  }
} // namespace

peano4::grid::tests::StackContainerTest::StackContainerTest() :
  TestCase("peano4::grid::tests::StackContainerTest") {}

void peano4::grid::tests::StackContainerTest::run() {
  testMethod(testEmptyContainerHasZeroSize);
  testMethod(testSizeCountsElementInLocalAndParallelStacks);
  testMethod(testSetLocalTreesPreservesExistingTreeDataWhenAddingNewTrees);
  testMethod(testSetLocalTreesDropsRemovedTrees);
  testMethod(testMultipleParallelSlotsIndependentAndCounted);
  testMethod(testStampMatchedPop);
  testMethod(testCrossRankPeriodicStaging);
  testMethod(testLazyTreeRegistration);
}

void peano4::grid::tests::StackContainerTest::testEmptyContainerHasZeroSize() {
  StackContainer<stacks::Stack<int>> c;

  c.setLocalTrees(std::set<int>{1});
  c.resizeParallelExchangeStacks(1, 1);

  validateEquals(c.size(), 0);

  auto& local0 = c.getLocalExchangeStack(1, 0);
  validateEquals(local0.size(), 0);
  validateEquals(c.size(), 0);

  // Touch a parallel slot/stack without pushing data: still zero.
  auto& p0 = c.getParallelExchangeStack(1, 0, StackTopology::ParallelExchangeStack::HorizontalOutput);
  validateEquals(p0.size(), 0);
  validateEquals(c.size(), 0);
}

void peano4::grid::tests::StackContainerTest::testSizeCountsElementInLocalAndParallelStacks() {
  StackContainer<stacks::Stack<int>> c;

  c.setLocalTrees(std::set<int>{1});
  c.resizeParallelExchangeStacks(1, 1);

  c.getLocalExchangeStack(1, 0).push(7);
  validateEquals(c.size(), 1);

  c.getLocalExchangeStack(1, 1).push(8);
  validateEquals(c.size(), 2);

  c.getParallelExchangeStack(1, 0, StackTopology::ParallelExchangeStack::HorizontalOutput).push(9);
  validateEquals(c.size(), 3);

  // Different parallel stack (same slot) is independent.
  validateEquals(c.getParallelExchangeStack(1, 0, StackTopology::ParallelExchangeStack::HorizontalInput).size(), 0);
  validateEquals(c.size(), 3);
}

void peano4::grid::tests::StackContainerTest::testSetLocalTreesPreservesExistingTreeDataWhenAddingNewTrees() {
  StackContainer<stacks::Stack<int>> c;

  c.setLocalTrees(std::set<int>{5});
  c.resizeParallelExchangeStacks(5, 1);

  c.getLocalExchangeStack(5, 0).push(1);
  c.getParallelExchangeStack(5, 0, StackTopology::ParallelExchangeStack::VerticalOutput).push(2);
  validateEquals(c.size(), 2);

  // Add a new tree; existing tree data must be preserved.
  c.setLocalTrees(std::set<int>{2, 5});

  validateEquals(c.getLocalExchangeStack(5, 0).size(), 1);
  validateEquals(c.getParallelExchangeStack(5, 0, StackTopology::ParallelExchangeStack::VerticalOutput).size(), 1);

  // New tree is empty.
  validateEquals(c.getLocalExchangeStack(2, 0).size(), 0);
  validateEquals(c.size(), 2);
}

void peano4::grid::tests::StackContainerTest::testSetLocalTreesDropsRemovedTrees() {
  StackContainer<stacks::Stack<int>> c;

  c.setLocalTrees(std::set<int>{2, 5});
  c.resizeParallelExchangeStacks(2, 2);

  c.getLocalExchangeStack(5, 0).push(11);
  c.getParallelExchangeStack(2, 1, StackTopology::ParallelExchangeStack::HorizontalOutput).push(22);
  validateEquals(c.size(), 2);

  // Remove tree 5: its data must no longer contribute.
  c.setLocalTrees(std::set<int>{2});
  validateEquals(c.size(), 1);
  validateEquals(c.getParallelExchangeStack(2, 1, StackTopology::ParallelExchangeStack::HorizontalOutput).size(), 1);
}

void peano4::grid::tests::StackContainerTest::testMultipleParallelSlotsIndependentAndCounted() {
  StackContainer<stacks::Stack<int>> c;

  c.setLocalTrees(std::set<int>{1});
  c.resizeParallelExchangeStacks(1, 3);

  c.getParallelExchangeStack(1, 0, StackTopology::ParallelExchangeStack::HorizontalOutput).push(1);
  validateEquals(c.size(), 1);

  c.getParallelExchangeStack(1, 2, StackTopology::ParallelExchangeStack::HorizontalOutput).push(2);
  c.getParallelExchangeStack(1, 2, StackTopology::ParallelExchangeStack::HorizontalOutput).push(3);
  c.getParallelExchangeStack(1, 2, StackTopology::ParallelExchangeStack::HorizontalOutput).push(4);

  validateEquals(c.getParallelExchangeStack(1, 0, StackTopology::ParallelExchangeStack::HorizontalOutput).size(), 1);
  validateEquals(c.getParallelExchangeStack(1, 2, StackTopology::ParallelExchangeStack::HorizontalOutput).size(), 3);
  validateEquals(c.size(), 4);
}

void peano4::grid::tests::StackContainerTest::testStampMatchedPop() {
  StackContainer<stacks::Stack<int>> c;
  c.setLocalTrees(std::set<int>{1});

  const int stackNumber =
    StackTopology::PeriodicBoundaryExchangeStackOffset + StackTopology::TotalNumberOfPeriodicBoundaryExchangeStacks / 2;

  // Deposit two low-side entries of different columns, in an order the reader
  // must not depend on.
  c.getLocalExchangeStack(1, stackNumber).push(11);
  c.pushLocalExchangeStamp(1, stackNumber, faceStampAt(3, PeriodicSide::Low));
  c.getLocalExchangeStack(1, stackNumber).push(22);
  c.pushLocalExchangeStamp(1, stackNumber, faceStampAt(5, PeriodicSide::Low));

  // A reader on the high side of column 3 gets exactly the column-3 entry.
  const auto matched = c.popLocalExchangeStackEntryMatching(1, stackNumber, faceStampAt(3, PeriodicSide::High));
  validate(matched.has_value());
  validateEquals(matched.value(), 11);
  validateEquals(c.getLocalExchangeStack(1, stackNumber).size(), 1);
  validateEquals(static_cast<int>(c.getLocalExchangeStamps(1, stackNumber).size()), 1);

  // A reader of the same side as the remaining entry must not match it.
  validate(not c.popLocalExchangeStackEntryMatching(1, stackNumber, faceStampAt(5, PeriodicSide::Low)).has_value());

  // A reader of an absent column finds nothing.
  validate(not c.popLocalExchangeStackEntryMatching(1, stackNumber, faceStampAt(7, PeriodicSide::High)).has_value());

  // The remaining entry pairs with its partner side.
  const auto remaining = c.popLocalExchangeStackEntryMatching(1, stackNumber, faceStampAt(5, PeriodicSide::High));
  validate(remaining.has_value());
  validateEquals(remaining.value(), 22);
  validateEquals(c.getLocalExchangeStack(1, stackNumber).size(), 0);
  validate(c.getLocalExchangeStamps(1, stackNumber).empty());
}

void peano4::grid::tests::StackContainerTest::testCrossRankPeriodicStaging() {
  StackContainer<stacks::Stack<int>> c;
  c.setLocalTrees(std::set<int>{1});

  validate(c.getCrossRankPeriodicStaging().empty());
  c.stageCrossRankPeriodicEntry(1, 4, faceStampAt(2, PeriodicSide::Low), 17, 33);
  validateEquals(static_cast<int>(c.getCrossRankPeriodicStaging().size()), 1);
  validateEquals(c.getCrossRankPeriodicStaging().front().sendTree, 1);
  validateEquals(c.getCrossRankPeriodicStaging().front().recvTree, 4);
  validateEquals(c.getCrossRankPeriodicStaging().front().depositStack, 17);
  validateEquals(c.getCrossRankPeriodicStaging().front().data, 33);
  c.clearCrossRankPeriodicStaging();
  validate(c.getCrossRankPeriodicStaging().empty());
}

void peano4::grid::tests::StackContainerTest::testLazyTreeRegistration() {
  StackContainer<stacks::Stack<int>> c;
  c.setLocalTrees(std::set<int>{1});

  // Accessing an unregistered tree allocates empty storage on demand.
  c.getLocalExchangeStack(9, 0).push(42);
  validateEquals(c.size(), 1);

  // A later setLocalTrees() that includes the tree keeps its data.
  c.setLocalTrees(std::set<int>{1, 9});
  validateEquals(c.getLocalExchangeStack(9, 0).size(), 1);
  validateEquals(c.getLocalExchangeStack(9, 0).pop(), 42);
}
