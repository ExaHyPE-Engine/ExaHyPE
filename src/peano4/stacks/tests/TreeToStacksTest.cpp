// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeToStacksTest.h"

#include "TestDataOnHeap.h"
#include "TestDataOnHeapWithContainer.h"
#include "TestDataOnStack.h"
#include "peano4/parallel/Node.h"
#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackWithCustomSerialisation.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "peano4/stacks/TreeToStacks.h"
#include "tarch/tests/TestMacros.h"

#include <algorithm>

peano4::stacks::tests::TreeToStacksTest::TreeToStacksTest():
  TestCase("peano4::stacks::tests::TreeToStacksTest") {}

void peano4::stacks::tests::TreeToStacksTest::run() {
  testMethod(testGetForPush);
  testMethod(testNoRecreateOnGetForPush);
  testMethod(testGetForPop);
  testMethod(testEmpty);
  testMethod(testSize);
  testMethod(testClearAll);
  testMethod(testClearTree);
  testMethod(testHoldsStack);
  testMethod(testGarbageCollection);
  testMethod(testGetKeys);
}

void peano4::stacks::tests::TreeToStacksTest::testGetForPush() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    int                                  treeId = 0, stackId = 0;
    Stack<TestDataOnStack>*              pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(10));
    pushStack->push(TestDataOnStack(20));
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].data[0], 10);
    validateEquals((*pushStack)[1].data[0], 20);

    treeId = 0, stackId = 1;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(30));
    pushStack->push(TestDataOnStack(40));
    pushStack->push(TestDataOnStack(50));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].data[0], 30);
    validateEquals((*pushStack)[1].data[0], 40);
    validateEquals((*pushStack)[2].data[0], 50);

    treeId = 1, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(60));
    pushStack->push(TestDataOnStack(70));
    pushStack->push(TestDataOnStack(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].data[0], 60);
    validateEquals((*pushStack)[1].data[0], 70);
    validateEquals((*pushStack)[2].data[0], 80);

    treeId = 0, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].data[0], 10);
    validateEquals((*pushStack)[1].data[0], 20);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                                  treeId = 0, stackId = 0;
    StackWithSerialisation<TestDataOnHeap>*              pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(10));
    pushStack->push(TestDataOnHeap(20));
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].data()[0], 10);
    validateEquals((*pushStack)[1].data()[0], 20);

    treeId = 0, stackId = 1;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(30));
    pushStack->push(TestDataOnHeap(40));
    pushStack->push(TestDataOnHeap(50));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].data()[0], 30);
    validateEquals((*pushStack)[1].data()[0], 40);
    validateEquals((*pushStack)[2].data()[0], 50);

    treeId = 1, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(60));
    pushStack->push(TestDataOnHeap(70));
    pushStack->push(TestDataOnHeap(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].data()[0], 60);
    validateEquals((*pushStack)[1].data()[0], 70);
    validateEquals((*pushStack)[2].data()[0], 80);

    treeId = 0, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].data()[0], 10);
    validateEquals((*pushStack)[1].data()[0], 20);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    int                                                                     treeId = 0, stackId = 0;
    StackWithCustomSerialisation<TestDataOnHeapWithContainer>* pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeapWithContainer(1, 10));
    pushStack->push(TestDataOnHeapWithContainer(1, 20));
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].at(0), 10);
    validateEquals((*pushStack)[1].at(0), 20);

    treeId = 0, stackId = 1;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeapWithContainer(1, 30));
    pushStack->push(TestDataOnHeapWithContainer(1, 40));
    pushStack->push(TestDataOnHeapWithContainer(1, 50));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].at(0), 30);
    validateEquals((*pushStack)[1].at(0), 40);
    validateEquals((*pushStack)[2].at(0), 50);

    treeId = 1, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeapWithContainer(1, 60));
    pushStack->push(TestDataOnHeapWithContainer(1, 70));
    pushStack->push(TestDataOnHeapWithContainer(1, 80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].at(0), 60);
    validateEquals((*pushStack)[1].at(0), 70);
    validateEquals((*pushStack)[2].at(0), 80);

    treeId = 0, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].at(0), 10);
    validateEquals((*pushStack)[1].at(0), 20);
  }
}

void peano4::stacks::tests::TreeToStacksTest::testNoRecreateOnGetForPush() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    int                                  tree = 2, stk = 5;
    auto*                                sA = stackMap.getForPush(tree, stk);
    validate(sA->empty());
    sA->push(TestDataOnStack(42));
    validateEquals(sA->size(), static_cast<std::size_t>(1));

    // Call getForPush again -> should not recreate/clear
    auto* sB = stackMap.getForPush(tree, stk);
    validate(sA == sB); // Same address (same object)
    validateEquals(sB->size(), static_cast<std::size_t>(1));
    validateEquals((*sB)[0].data[0], 42);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                                  tree = 3, stk = 1;
    auto*                                                sA = stackMap.getForPush(tree, stk);
    sA->push(TestDataOnHeap(77));
    validateEquals(sA->size(), static_cast<std::size_t>(1));

    auto* sB = stackMap.getForPush(tree, stk);
    validate(sA == sB);
    validateEquals((*sB)[0].data()[0], 77);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    int                                                                     tree = 3, stk = 1;
    auto*                                                                   sA = stackMap.getForPush(tree, stk);
    sA->push(TestDataOnHeapWithContainer(1, 77));
    validateEquals(sA->size(), static_cast<std::size_t>(1));

    auto* sB = stackMap.getForPush(tree, stk);
    validate(sA == sB);
    validateEquals((*sB)[0].at(0), 77);
  }
}

void peano4::stacks::tests::TreeToStacksTest::testGetForPop() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    int                                  treeId = 0, stackId = 0;
    Stack<TestDataOnStack>*              pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnStack(30));
    pushStack->push(TestDataOnStack(40));
    Stack<TestDataOnStack>* popStack = stackMap.getForPop(treeId, stackId);
    validateEquals(popStack->size(), static_cast<std::size_t>(2));
    validateEquals((*popStack)[0].data[0], 30);
    validateEquals((*popStack)[1].data[0], 40);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                                  treeId = 0, stackId = 0;
    Stack<TestDataOnHeap>*                               pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnHeap(30));
    pushStack->push(TestDataOnHeap(40));
    Stack<TestDataOnHeap>* popStack = stackMap.getForPop(treeId, stackId);
    validateEquals(popStack->size(), static_cast<std::size_t>(2));
    validateEquals((*popStack)[0].data()[0], 30);
    validateEquals((*popStack)[1].data()[0], 40);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    int                                                                     treeId = 0, stackId = 0;
    Stack<TestDataOnHeapWithContainer>* pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnHeapWithContainer(1, 30));
    pushStack->push(TestDataOnHeapWithContainer(1, 40));
    Stack<TestDataOnHeapWithContainer>* popStack = stackMap.getForPop(treeId, stackId);
    validateEquals(popStack->size(), static_cast<std::size_t>(2));
    validateEquals((*popStack)[0].at(0), 30);
    validateEquals((*popStack)[1].at(0), 40);
  }
}

void peano4::stacks::tests::TreeToStacksTest::testEmpty() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    // For tree id 0, stack id 99 should be considered empty.
    int treeId = 0, stackId = 99;
    validate(stackMap.empty(treeId, stackId));
    // Create the stack.
    stackMap.getForPush(treeId, stackId);
    // Still empty because nothing has been pushed.
    validate(stackMap.empty(treeId, stackId));
    // Push an element.
    Stack<TestDataOnStack>* pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->push(TestDataOnStack(50));
    validate(not stackMap.empty(treeId, stackId));
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    // For tree id 0, stack id 99 should be considered empty.
    int treeId = 0, stackId = 99;
    validate(stackMap.empty(treeId, stackId));
    // Create the stack.
    stackMap.getForPush(treeId, stackId);
    // Still empty because nothing has been pushed.
    validate(stackMap.empty(treeId, stackId));
    // Push an element.
    StackWithSerialisation<TestDataOnHeap>* pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->push(TestDataOnHeap(50));
    validate(not stackMap.empty(treeId, stackId));
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    // For tree id 0, stack id 99 should be considered empty.
    int treeId = 0, stackId = 99;
    validate(stackMap.empty(treeId, stackId));
    // Create the stack.
    stackMap.getForPush(treeId, stackId);
    // Still empty because nothing has been pushed.
    validate(stackMap.empty(treeId, stackId));
    // Push an element.
    StackWithCustomSerialisation<TestDataOnHeapWithContainer>* pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->push(TestDataOnHeapWithContainer(1, 50));
    validate(not stackMap.empty(treeId, stackId));
  }
}

void peano4::stacks::tests::TreeToStacksTest::testSize() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;

    validateEquals(stackMap.size(), 0u);

    stackMap.getForPush(0, 1)->push(TestDataOnStack(11));
    stackMap.getForPush(0, 2)->push(TestDataOnStack(21));
    stackMap.getForPush(1, 3)->push(TestDataOnStack(31));
    stackMap.getForPush(1, 3)->push(TestDataOnStack(32));

    // Total = 4
    validateEquals(stackMap.size(), 4u);

    stackMap.clear(0);

    // Left only 2 in tree 1
    validateEquals(stackMap.size(), 2u);

    stackMap.clear();

    validateEquals(stackMap.size(), 0u);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;

    validateEquals(stackMap.size(), 0u);

    stackMap.getForPush(0, 1)->push(TestDataOnHeap(11));
    stackMap.getForPush(0, 2)->push(TestDataOnHeap(21));
    stackMap.getForPush(1, 3)->push(TestDataOnHeap(31));
    stackMap.getForPush(1, 3)->push(TestDataOnHeap(32));

    // Total = 4
    validateEquals(stackMap.size(), 4u);

    stackMap.clear(0);

    // Left only 2 in tree 1
    validateEquals(stackMap.size(), 2u);

    stackMap.clear();

    validateEquals(stackMap.size(), 0u);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;

    validateEquals(stackMap.size(), 0u);

    stackMap.getForPush(0, 1)->push(TestDataOnHeapWithContainer(1, 11));
    stackMap.getForPush(0, 2)->push(TestDataOnHeapWithContainer(1, 21));
    stackMap.getForPush(1, 3)->push(TestDataOnHeapWithContainer(1, 31));
    stackMap.getForPush(1, 3)->push(TestDataOnHeapWithContainer(1, 32));

    // Total = 4
    validateEquals(stackMap.size(), 4u);

    stackMap.clear(0);

    // Left only 2 in tree 1
    validateEquals(stackMap.size(), 2u);

    stackMap.clear();

    validateEquals(stackMap.size(), 0u);
  }
}

void peano4::stacks::tests::TreeToStacksTest::testClearAll() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    // Create a few stacks and push data
    auto* s00 = stackMap.getForPush(0, 0);
    auto* s01 = stackMap.getForPush(0, 1);
    auto* s10 = stackMap.getForPush(1, 0);
    s00->push(TestDataOnStack(1));
    s01->push(TestDataOnStack(2));
    s10->push(TestDataOnStack(3));
    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));
    validate(stackMap.holdsStack(1, 0));

    // Clear all stacks logically (entries remain)
    stackMap.clear();

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));
    validate(stackMap.holdsStack(1, 0));
    validate(s00->empty());
    validate(s01->empty());
    validate(s10->empty());
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    auto*                                                s00 = stackMap.getForPush(0, 0);
    auto*                                                s01 = stackMap.getForPush(0, 1);
    s00->push(TestDataOnHeap(11));
    s01->push(TestDataOnHeap(22));
    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));

    stackMap.clear();

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));
    validate(s00->empty());
    validate(s01->empty());
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    auto*                                                                   s00 = stackMap.getForPush(0, 0);
    auto*                                                                   s01 = stackMap.getForPush(0, 1);
    s00->push(TestDataOnHeapWithContainer(1, 11));
    s01->push(TestDataOnHeapWithContainer(1, 22));
    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));

    stackMap.clear();

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));
    validate(s00->empty());
    validate(s01->empty());
  }
}

void peano4::stacks::tests::TreeToStacksTest::testClearTree() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    auto*                                s00 = stackMap.getForPush(0, 0);
    auto*                                s01 = stackMap.getForPush(0, 1);
    auto*                                s10 = stackMap.getForPush(1, 0);
    s00->push(TestDataOnStack(1));
    s01->push(TestDataOnStack(2));
    s10->push(TestDataOnStack(3));

    // Clear only tree 0; tree 1 should be untouched
    stackMap.clear(0);

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(0, 1));
    validate(stackMap.holdsStack(1, 0));

    validate(s00->empty());
    validate(s01->empty());
    validate(not s10->empty());
    validateEquals((*s10)[0].data[0], 3);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    auto*                                                s00 = stackMap.getForPush(0, 0);
    auto*                                                s10 = stackMap.getForPush(1, 0);
    s00->push(TestDataOnHeap(5));
    s10->push(TestDataOnHeap(6));

    stackMap.clear(0);

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(1, 0));
    validate(s00->empty());
    validate(not s10->empty());
    validateEquals((*s10)[0].data()[0], 6);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    auto*                                                                   s00 = stackMap.getForPush(0, 0);
    auto*                                                                   s10 = stackMap.getForPush(1, 0);
    s00->push(TestDataOnHeapWithContainer(1, 5));
    s10->push(TestDataOnHeapWithContainer(1, 6));

    stackMap.clear(0);

    validate(stackMap.holdsStack(0, 0));
    validate(stackMap.holdsStack(1, 0));
    validate(s00->empty());
    validate(not s10->empty());
    validateEquals((*s10)[0].at(0), 6);
  }
}

void peano4::stacks::tests::TreeToStacksTest::testHoldsStack() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    validate(not stackMap.holdsStack(3, 7));
    auto* s = stackMap.getForPush(3, 7);
    validate(stackMap.holdsStack(3, 7));
    validate(s->empty());
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    validate(not stackMap.holdsStack(4, 9));
    (void)stackMap.getForPush(4, 9);
    validate(stackMap.holdsStack(4, 9));
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    validate(not stackMap.holdsStack(4, 9));
    (void)stackMap.getForPush(4, 9);
    validate(stackMap.holdsStack(4, 9));
  }
}

void peano4::stacks::tests::TreeToStacksTest::testGarbageCollection() {
  const int treeId  = 2;
  const int emptyId = 70; // Non-storage stack assumed by tests
  validate(not parallel::Node::isStorageStackNumber(emptyId));
  const int liveId = 8;

  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    // Prepare one empty and one non-empty stack in the same tree
    auto* sEmpty = stackMap.getForPush(treeId, emptyId);
    auto* sLive  = stackMap.getForPush(treeId, liveId);
    sEmpty->push(TestDataOnStack(80));
    sEmpty->pop(); // Now empty
    sLive->push(TestDataOnStack(11));

    validate(stackMap.holdsStack(treeId, emptyId));
    validate(stackMap.holdsStack(treeId, liveId));

    // Run GC -> should erase empty non-storage stack, keep the non-empty one
    stackMap.garbageCollection(treeId);

    // The empty non-storage stack must be gone (entry erased)
    validate(not stackMap.holdsStack(treeId, emptyId)); // Erased entry
    validate(stackMap.empty(treeId, emptyId));          // empty() stays true when entry is missing
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, emptyId}) == 0);
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, liveId}) == 1);

    // The non-empty stack must remain intact
    auto* sLiveAfter = stackMap.getForPop(treeId, liveId);
    validateEquals(sLiveAfter->size(), static_cast<std::size_t>(1));
    validateEquals((*sLiveAfter)[0].data[0], 11);

    // Accessing the erased stack recreates it on demand
    auto* sEmptyRecreated = stackMap.getForPush(treeId, emptyId);
    validate(stackMap.holdsStack(treeId, emptyId));
    validate(sEmptyRecreated->empty());
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    auto*                                                sEmpty = stackMap.getForPush(treeId, emptyId);
    auto*                                                sLive  = stackMap.getForPush(treeId, liveId);
    sEmpty->push(TestDataOnHeap(80));
    sEmpty->pop(); // Now empty
    sLive->push(TestDataOnHeap(21));

    validate(stackMap.holdsStack(treeId, emptyId));
    validate(stackMap.holdsStack(treeId, liveId));

    stackMap.garbageCollection(treeId);

    // Empty non-storage erased
    validate(not stackMap.holdsStack(treeId, emptyId));
    validate(stackMap.empty(treeId, emptyId));
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, emptyId}) == 0);
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, liveId}) == 1);

    // Non-empty preserved
    auto* sLiveAfter = stackMap.getForPop(treeId, liveId);
    validateEquals(sLiveAfter->size(), static_cast<std::size_t>(1));
    validateEquals((*sLiveAfter)[0].data()[0], 21);

    // Recreate on demand
    auto* sEmptyRecreated = stackMap.getForPush(treeId, emptyId);
    validate(stackMap.holdsStack(treeId, emptyId));
    validate(sEmptyRecreated->empty());
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    auto* sEmpty = stackMap.getForPush(treeId, emptyId);
    auto* sLive  = stackMap.getForPush(treeId, liveId);
    sEmpty->push(TestDataOnHeapWithContainer(1, 80));
    sEmpty->pop(); // Now empty
    sLive->push(TestDataOnHeapWithContainer(1, 21));

    validate(stackMap.holdsStack(treeId, emptyId));
    validate(stackMap.holdsStack(treeId, liveId));

    stackMap.garbageCollection(treeId);

    // Empty non-storage erased
    validate(not stackMap.holdsStack(treeId, emptyId));
    validate(stackMap.empty(treeId, emptyId));
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, emptyId}) == 0);
    validate(std::count(keys.begin(), keys.end(), StackKey{treeId, liveId}) == 1);

    // Non-empty preserved
    auto* sLiveAfter = stackMap.getForPop(treeId, liveId);
    validateEquals(sLiveAfter->size(), static_cast<std::size_t>(1));
    validateEquals((*sLiveAfter)[0].at(0), 21);

    // Recreate on demand
    auto* sEmptyRecreated = stackMap.getForPush(treeId, emptyId);
    validate(stackMap.holdsStack(treeId, emptyId));
    validate(sEmptyRecreated->empty());
  }
}

void peano4::stacks::tests::TreeToStacksTest::testGetKeys() {
  {
    TreeToStacks<Stack<TestDataOnStack>> stackMap;
    // Create several stacks with distinct keys.
    stackMap.getForPush(0, 10);
    stackMap.getForPush(1, 20);
    stackMap.getForPush(2, 30);
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{0, 10}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{1, 20}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{2, 30}) == 1);
  }

  {
    TreeToStacks<StackWithSerialisation<TestDataOnHeap>> stackMap;
    // Create several stacks with distinct keys.
    stackMap.getForPush(0, 10);
    stackMap.getForPush(1, 20);
    stackMap.getForPush(2, 30);
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{0, 10}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{1, 20}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{2, 30}) == 1);
  }

  {
    TreeToStacks<StackWithCustomSerialisation<TestDataOnHeapWithContainer>> stackMap;
    // Create several stacks with distinct keys.
    stackMap.getForPush(0, 10);
    stackMap.getForPush(1, 20);
    stackMap.getForPush(2, 30);
    const auto keys = stackMap.getKeys();
    validate(std::count(keys.begin(), keys.end(), StackKey{0, 10}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{1, 20}) == 1);
    validate(std::count(keys.begin(), keys.end(), StackKey{2, 30}) == 1);
  }
}
