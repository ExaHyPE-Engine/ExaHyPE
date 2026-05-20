// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackContainerTest.h"

#include "peano4/grid/StackContainer.h"
#include "peano4/grid/StackTopology.h"
#include "peano4/stacks/Stack.h"

peano4::grid::tests::StackContainerTest::StackContainerTest():
  TestCase("peano4::grid::tests::StackContainerTest") {}

void peano4::grid::tests::StackContainerTest::run() {
  testMethod(testEmptyContainerHasZeroSize);
  testMethod(testSizeCountsElementInLocalAndParallelStacks);
  testMethod(testSetLocalTreesPreservesExistingTreeDataWhenAddingNewTrees);
  testMethod(testSetLocalTreesDropsRemovedTrees);
  testMethod(testMultipleParallelSlotsIndependentAndCounted);
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
