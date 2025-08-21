// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackMapTest.h"

#include "TestDataOnHeap.h"
#include "TestDataOnStack.h"
#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackMap.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "tarch/tests/TestMacros.h"

peano4::stacks::tests::StackMapTest::StackMapTest():
  TestCase("peano4::stacks::tests::StackMapTest") {}

void peano4::stacks::tests::StackMapTest::run() {
  testMethod(testGetForPush);
  testMethod(testGetForPop);
  testMethod(testEmpty);
  testMethod(testGarbageCollection);
  testMethod(testGetKeys);
}

void peano4::stacks::tests::StackMapTest::testGetForPush() {
  {
    StackMap<Stack<TestDataOnStack>> stackMap;
    int                              treeId = 0, stackId = 0;
    Stack<TestDataOnStack>*          pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(10));
    pushStack->push(TestDataOnStack(20));
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].value[0], 10);
    validateEquals((*pushStack)[1].value[0], 20);

    treeId = 0, stackId = 1;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(30));
    pushStack->push(TestDataOnStack(40));
    pushStack->push(TestDataOnStack(50));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].value[0], 30);
    validateEquals((*pushStack)[1].value[0], 40);
    validateEquals((*pushStack)[2].value[0], 50);

    treeId = 1, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnStack(60));
    pushStack->push(TestDataOnStack(70));
    pushStack->push(TestDataOnStack(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].value[0], 60);
    validateEquals((*pushStack)[1].value[0], 70);
    validateEquals((*pushStack)[2].value[0], 80);

    treeId = 0, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].value[0], 10);
    validateEquals((*pushStack)[1].value[0], 20);
  }

  {
    StackMap<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                              treeId = 0, stackId = 0;
    StackWithSerialisation<TestDataOnHeap>*          pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(10));
    pushStack->push(TestDataOnHeap(20));
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].value[0], 10);
    validateEquals((*pushStack)[1].value[0], 20);

    treeId = 0, stackId = 1;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(30));
    pushStack->push(TestDataOnHeap(40));
    pushStack->push(TestDataOnHeap(50));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].value[0], 30);
    validateEquals((*pushStack)[1].value[0], 40);
    validateEquals((*pushStack)[2].value[0], 50);

    treeId = 1, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validate(pushStack->empty());
    pushStack->push(TestDataOnHeap(60));
    pushStack->push(TestDataOnHeap(70));
    pushStack->push(TestDataOnHeap(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(3));
    validateEquals((*pushStack)[0].value[0], 60);
    validateEquals((*pushStack)[1].value[0], 70);
    validateEquals((*pushStack)[2].value[0], 80);

    treeId = 0, stackId = 0;
    pushStack = stackMap.getForPush(treeId, stackId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(2));
    validateEquals((*pushStack)[0].value[0], 10);
    validateEquals((*pushStack)[1].value[0], 20);
  }
}

void peano4::stacks::tests::StackMapTest::testGetForPop() {
  {
    StackMap<Stack<TestDataOnStack>> stackMap;
    int                              treeId = 0, stackId = 0;
    Stack<TestDataOnStack>*          pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnStack(30));
    pushStack->push(TestDataOnStack(40));
    Stack<TestDataOnStack>* popStack = stackMap.getForPop(treeId, stackId);
    validateEquals(popStack->size(), static_cast<std::size_t>(2));
    validateEquals((*popStack)[0].value[0], 30);
    validateEquals((*popStack)[1].value[0], 40);
  }

  {
    StackMap<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                              treeId = 0, stackId = 0;
    Stack<TestDataOnHeap>*                           pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnHeap(30));
    pushStack->push(TestDataOnHeap(40));
    Stack<TestDataOnHeap>* popStack = stackMap.getForPop(treeId, stackId);
    validateEquals(popStack->size(), static_cast<std::size_t>(2));
    validateEquals((*popStack)[0].value[0], 30);
    validateEquals((*popStack)[1].value[0], 40);
  }
}

void peano4::stacks::tests::StackMapTest::testEmpty() {
  {
    StackMap<Stack<TestDataOnStack>> stackMap;
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
    validate(!stackMap.empty(treeId, stackId));
  }

  {
    StackMap<StackWithSerialisation<TestDataOnHeap>> stackMap;
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
    validate(!stackMap.empty(treeId, stackId));
  }
}

void peano4::stacks::tests::StackMapTest::testGarbageCollection() {
  {
    StackMap<Stack<TestDataOnStack>> stackMap;
    int                              treeId = 2, stackId = 7;
    Stack<TestDataOnStack>*          pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnStack(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(1));
    pushStack->pop(); // Now empty
    validateEquals(pushStack->size(), static_cast<std::size_t>(0));
    // Run garbage collection for this spacetree.
    stackMap.garbageCollection(treeId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(0));
  }

  {
    StackMap<StackWithSerialisation<TestDataOnHeap>> stackMap;
    int                                              treeId = 2, stackId = 7;
    StackWithSerialisation<TestDataOnHeap>*          pushStack = stackMap.getForPush(treeId, stackId);
    pushStack->clear();
    pushStack->push(TestDataOnHeap(80));
    validateEquals(pushStack->size(), static_cast<std::size_t>(1));
    pushStack->pop(); // Now empty
    validateEquals(pushStack->size(), static_cast<std::size_t>(0));
    // Run garbage collection for this spacetree.
    stackMap.garbageCollection(treeId);
    validateEquals(pushStack->size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackMapTest::testGetKeys() {
  {
    StackMap<Stack<TestDataOnStack>> stackMap;
    // Create several stacks with distinct keys.
    stackMap.getForPush(0, 10);
    stackMap.getForPush(1, 20);
    stackMap.getForPush(2, 30);
    const auto& keys = stackMap.getKeys();
    validate(keys.count(StackKey(0, 10)) == 1);
    validate(keys.count(StackKey(1, 20)) == 1);
    validate(keys.count(StackKey(2, 30)) == 1);
  }

  {
    StackMap<StackWithSerialisation<TestDataOnHeap>> stackMap;
    // Create several stacks with distinct keys.
    stackMap.getForPush(0, 10);
    stackMap.getForPush(1, 20);
    stackMap.getForPush(2, 30);
    const auto& keys = stackMap.getKeys();
    validate(keys.count(StackKey(0, 10)) == 1);
    validate(keys.count(StackKey(1, 20)) == 1);
    validate(keys.count(StackKey(2, 30)) == 1);
  }
}
