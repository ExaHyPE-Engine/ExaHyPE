// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTest.h"

#include "TestDataOnHeap.h"
#include "TestDataOnStack.h"
#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "tarch/tests/TestMacros.h"

peano4::stacks::tests::StackTest::StackTest():
  TestCase("peano4::stacks::tests::StackTest") {

#ifdef WITH_MPI
  TestDataOnStack::initDatatype();
#endif
}

peano4::stacks::tests::StackTest::~StackTest() {
#ifdef WITH_MPI
  TestDataOnStack::shutdownDatatype();
#endif
}

void peano4::stacks::tests::StackTest::run() {
  testMethod(testPushData);
  testMethod(testEraseData);
  testMethod(testClearData);
  testMethod(testEmplaceData);
  testMethod(testPopData);
  testMethod(testClone);
  testMethod(testMPI);
}

void peano4::stacks::tests::StackTest::testPushData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(10);

    for (int n = 0; n < 5; n++) {
      stack.push(TestDataOnStack(n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(stack[n].value[i], static_cast<int>(n));
      }
    }

    for (int n = 5; n < 10; n++) {
      TestDataOnStack testData(n);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(stack[n].value[i], static_cast<int>(n));
      }
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(10);

    for (int n = 0; n < 5; n++) {
      stack.push(TestDataOnHeap(n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(stack[n].value[i], static_cast<int>(n));
      }
    }

    for (int n = 5; n < 10; n++) {
      TestDataOnHeap testData(n);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(stack[n].value[i], static_cast<int>(n));
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testEraseData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(4);

    stack.push(TestDataOnStack(100));
    stack.push(TestDataOnStack(200));
    stack.push(TestDataOnStack(300));
    stack.push(TestDataOnStack(400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    // Erase the element at index 1 (value 200).
    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    // Verify that none of the remaining elements equal 200.
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validate(stack[n].value[i] != 200);
      }
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(4);

    stack.push(TestDataOnHeap(100));
    stack.push(TestDataOnHeap(200));
    stack.push(TestDataOnHeap(300));
    stack.push(TestDataOnHeap(400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    // Erase the element at index 1 (value 200).
    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    // Verify that none of the remaining elements equal 200.
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validate(stack[n].value[i] != 200);
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testClearData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(2);
    stack.push(TestDataOnStack(111));
    stack.push(TestDataOnStack(222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(2);
    stack.push(TestDataOnHeap(111));
    stack.push(TestDataOnHeap(222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackTest::testEmplaceData() {
  {
    Stack<TestDataOnStack> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].value[0], 42);
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].value[0], 42);
  }
}

void peano4::stacks::tests::StackTest::testPopData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(2);
    stack.push(TestDataOnStack(500));
    stack.push(TestDataOnStack(600));

    TestDataOnStack popped = stack.pop();
    validateEquals(popped.value[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    TestDataOnStack popped2 = stack.pop();
    validateEquals(popped2.value[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(2);
    stack.push(TestDataOnHeap(500));
    stack.push(TestDataOnHeap(600));

    TestDataOnHeap popped = stack.pop();
    validateEquals(popped.value[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    TestDataOnHeap popped2 = stack.pop();
    validateEquals(popped2.value[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackTest::testClone() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(3);
    stack.push(TestDataOnStack(1));
    stack.push(TestDataOnStack(2));
    stack.push(TestDataOnStack(3));

    // Create a new (empty) stack and clone the data.
    Stack<TestDataOnStack> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    // Verify that both stacks have the same size and content.
    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t i = 0; i < stack.size(); i++) {
      validateEquals(clonedStack[i].value[0], stack[i].value[0]);
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(3);
    stack.push(TestDataOnHeap(1));
    stack.push(TestDataOnHeap(2));
    stack.push(TestDataOnHeap(3));

    // Create a new (empty) stack and clone the data.
    StackWithSerialisation<TestDataOnHeap> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    // Verify that both stacks have the same size and content.
    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t i = 0; i < stack.size(); i++) {
      validateEquals(clonedStack[i].value[0], stack[i].value[0]);
    }
  }
}

void peano4::stacks::tests::StackTest::testMPI() {
#ifdef WITH_MPI
  // Test non-blocking send and receive: after completion, the stack should be cleared.
  {
    Stack<TestDataOnStack> sender;
    Stack<TestDataOnStack> receiver;

    sender.resize(5);
    validateEquals(sender.size(), static_cast<std::size_t>(5));
    for (int n = 0; n < static_cast<int>(sender.size()); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        sender[n].value[i] = n * i;
      }
    }

    sender.startSend(grid::TraversalObserver::SendReceiveContext::BoundaryExchange, 0, 123, MPI_COMM_SELF);
    receiver.startReceive(grid::TraversalObserver::SendReceiveContext::BoundaryExchange, 0, 123, MPI_COMM_SELF, 5);
    while (not sender.tryToFinishSendOrReceive() or not receiver.tryToFinishSendOrReceive()) {
    };

    validateEquals(sender.size(), static_cast<std::size_t>(0));   // Sender stack should be cleared
    validateEquals(receiver.size(), static_cast<std::size_t>(5)); // Receiver stack should now have the data
    for (int n = 0; n < static_cast<int>(receiver.size()); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(receiver[n].value[i], n * i);
      }
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> sender;
    StackWithSerialisation<TestDataOnHeap> receiver;

    sender.resize(5);
    validateEquals(sender.size(), static_cast<std::size_t>(5));
    for (int n = 0; n < static_cast<int>(sender.size()); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        sender[n].value[i] = n * i;
      }
    }

    sender.startSend(
      grid::TraversalObserver::SendReceiveContext::BoundaryExchange,
      tarch::mpi::Rank::getInstance().getRank(),
      123,
      MPI_COMM_WORLD
    );
    receiver.startReceive(
      grid::TraversalObserver::SendReceiveContext::BoundaryExchange,
      tarch::mpi::Rank::getInstance().getRank(),
      123,
      MPI_COMM_WORLD,
      5
    );
    while (not sender.tryToFinishSendOrReceive() or not receiver.tryToFinishSendOrReceive()) {
    };

    validateEquals(sender.size(), static_cast<std::size_t>(0));   // Sender stack should be cleared
    validateEquals(receiver.size(), static_cast<std::size_t>(5)); // Receiver stack should now have the data
    for (int n = 0; n < static_cast<int>(receiver.size()); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(receiver[n].value[i], n * i);
      }
    }
  }
#endif
}
