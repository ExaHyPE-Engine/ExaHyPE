// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTest.h"

#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackWithCustomSerialisation.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "peano4/stacks/tests/TestDataOnHeap.h"
#include "peano4/stacks/tests/TestDataOnHeapWithContainer.h"
#include "peano4/stacks/tests/TestDataOnStack.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tests/TestMacros.h"

namespace {
  class NullableSlotData {
  private:
    int* _value{nullptr};

  public:
    NullableSlotData() = default;

    explicit NullableSlotData(int value) :
      _value(new int(value)) {}

    ~NullableSlotData() { delete _value; }

    NullableSlotData(const NullableSlotData& other) :
      _value(other._value == nullptr ? nullptr : new int(*other._value)) {}

    NullableSlotData(NullableSlotData&& other) noexcept :
      _value(std::exchange(other._value, nullptr)) {}

    NullableSlotData& operator=(const NullableSlotData& other) {
      if (this == &other) {
        return *this;
      }

      if (other._value == nullptr) {
        delete _value;
        _value = nullptr;
        return *this;
      }

      if (_value == nullptr) {
        _value = new int;
      }
      *_value = *other._value;
      return *this;
    }

    NullableSlotData& operator=(NullableSlotData&& other) noexcept {
      if (this != &other) {
        delete _value;
        _value = std::exchange(other._value, nullptr);
      }
      return *this;
    }

    bool empty() const { return _value == nullptr; }

    int value() const {
      assertion(_value != nullptr);
      return *_value;
    }
  };

  template <class StackType>
  void completeTransfer(StackType& sender, StackType& receiver) {
    bool senderFinished   = false;
    bool receiverFinished = false;
    while (not senderFinished or not receiverFinished) {
      senderFinished   = sender.tryToFinishSendOrReceive();
      receiverFinished = receiver.tryToFinishSendOrReceive();
    }
  }
} // namespace

peano4::stacks::tests::StackTest::StackTest() :
  TestCase("peano4::stacks::tests::StackTest") {}

void peano4::stacks::tests::StackTest::run() {
  testMethod(testPushData);
  testMethod(testEraseData);
  testMethod(testClearData);
  testMethod(testEmplaceData);
  testMethod(testPopData);
  testMethod(testClone);
  testMethod(testReusedSlotAssignmentClearsEmptyState);
  testMethod(testPushBlockView);
  testMethod(testPopBlockView);
  testMethod(testSendAndReceiveToSelf);
  testMethod(testSendAndReceiveInARing);
}

void peano4::stacks::tests::StackTest::testPushData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(10);

    // TODO: Test with copy constructor as well

    for (int n = 0; n < 5; n++) {
      stack.push(TestDataOnStack(n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(stack[n].data[i], static_cast<int>(n));
      }
    }

    for (int n = 5; n < 10; n++) {
      TestDataOnStack testData(n);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(stack[n].data[i], static_cast<int>(n));
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
        validateEquals(stack[n].data()[i], static_cast<int>(n));
      }
    }

    for (int n = 5; n < 10; n++) {
      TestDataOnHeap testData(n);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(stack[n].data()[i], static_cast<int>(n));
      }
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(10);

    for (int n = 0; n < 5; n++) {
      stack.push(TestDataOnHeapWithContainer(32, n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));

    for (int n = 5; n < 10; n++) {
      TestDataOnHeapWithContainer testData(32, n);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < 32; i++) {
        validateEquals(stack[n].at(i), static_cast<int>(n));
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
        validate(stack[n].data[i] != 200);
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
        validate(stack[n].data()[i] != 200);
      }
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(4);

    stack.push(TestDataOnHeapWithContainer(32, 100));
    stack.push(TestDataOnHeapWithContainer(32, 200));
    stack.push(TestDataOnHeapWithContainer(32, 300));
    stack.push(TestDataOnHeapWithContainer(32, 400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < 32; i++) {
        validate(stack[n].at(i) != 200);
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

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(2);
    stack.push(TestDataOnHeapWithContainer(32, 111));
    stack.push(TestDataOnHeapWithContainer(32, 222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackTest::testEmplaceData() {
  {
    Stack<TestDataOnStack> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].data[0], 42);
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].data()[0], 42);
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.emplace(32, 42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].at(0), 42);
  }
}

void peano4::stacks::tests::StackTest::testPopData() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(2);
    stack.push(TestDataOnStack(500));
    stack.push(TestDataOnStack(600));

    const TestDataOnStack popped = stack.pop();
    validateEquals(popped.data[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const TestDataOnStack popped2 = stack.pop();
    validateEquals(popped2.data[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(2);
    stack.push(TestDataOnHeap(500));
    stack.push(TestDataOnHeap(600));

    const TestDataOnHeap popped = stack.pop();
    validateEquals(popped.data()[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const TestDataOnHeap popped2 = stack.pop();
    validateEquals(popped2.data()[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(2);
    stack.push(TestDataOnHeapWithContainer(32, 500));
    stack.push(TestDataOnHeapWithContainer(32, 600));

    const TestDataOnHeapWithContainer popped = stack.pop();
    validateEquals(popped.at(0), 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const TestDataOnHeapWithContainer popped2 = stack.pop();
    validateEquals(popped2.at(0), 500);
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
      validateEquals(clonedStack[i].data[0], stack[i].data[0]);
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
      validateEquals(clonedStack[i].data()[0], stack[i].data()[0]);
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(3);
    stack.push(TestDataOnHeapWithContainer(32, 1));
    stack.push(TestDataOnHeapWithContainer(32, 2));
    stack.push(TestDataOnHeapWithContainer(32, 3));

    // Create a new (empty) stack and clone the data.
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    // Verify that both stacks have the same size and content.
    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t n = 0; n < stack.size(); n++) {
      for (int i = 0; i < 32; i++) {
        validateEquals(clonedStack[n].at(i), stack[n].at(i));
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testReusedSlotAssignmentClearsEmptyState() {
  {
    Stack<NullableSlotData> stack;
    stack.push(NullableSlotData(7));
    validateEquals(stack[0].value(), 7);

    stack.shallowClear();

    const NullableSlotData empty;
    stack.push(empty);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    Stack<NullableSlotData> stack;
    auto                    initial = stack.pushBlock(1);
    initial.set(0, NullableSlotData(9));
    validateEquals(stack[0].value(), 9);

    stack.shallowClear();

    auto reused = stack.pushBlock(1);
    reused.set(0, NullableSlotData());

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    Stack<NullableSlotData> stack;
    auto                    initial = stack.pushBlock(1);
    initial.set(0, NullableSlotData(10));
    validateEquals(stack[0].value(), 10);

    stack.shallowClear();

    auto reused = stack.pushBlock(1);
    reused.emplace_default(0);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.push(TestDataOnHeap(11));
    validateEquals(stack[0].data()[0], 11);

    stack.shallowClear();

    TestDataOnHeap       movedFrom(12);
    const TestDataOnHeap owner(std::move(movedFrom));
    stack.push(movedFrom);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].data() == nullptr);
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.push(TestDataOnHeapWithContainer(1, 13));
    validateEquals(stack[0].at(0), 13);

    stack.shallowClear();

    const TestDataOnHeapWithContainer empty;
    stack.push(empty);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].size(), 0);
  }
}

void peano4::stacks::tests::StackTest::testPushBlockView() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(16);

    // Reserve a block of 4 and fill via rvalues/moves.
    auto blockView = stack.pushBlock(4);
    validateEquals(blockView.size(), static_cast<std::size_t>(4));

    // Use temporaries (rvalues) => move-assignment
    blockView.set(0, TestDataOnStack(101));
    blockView.set(1, TestDataOnStack(102));

    // Use lvalue moved into place => move-assignment
    TestDataOnStack tmp(103);
    blockView.set(2, std::move(tmp));

    // Another rvalue
    blockView.set(3, TestDataOnStack(104));

    // The block increased logical size of stack
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    // Verify contents landed correctly
    validateEquals(stack[0].data[0], 101);
    validateEquals(stack[1].data[0], 102);
    validateEquals(stack[2].data[0], 103);
    validateEquals(stack[3].data[0], 104);

    // --- In-place construction (no temporaries) ---
    blockView.emplace(0, 201);
    blockView.emplace(1, 202);
    blockView.emplace(2, 203);
    blockView.emplace(3, 204);

    // Verify contents updated via in-place reconstruction
    validateEquals(stack[0].data[0], 201);
    validateEquals(stack[1].data[0], 202);
    validateEquals(stack[2].data[0], 203);
    validateEquals(stack[3].data[0], 204);
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(16);

    auto blockView = stack.pushBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));

    // Mix rvalues and moved lvalues
    blockView.set(0, TestDataOnHeap(201)); // Rvalue => move-assign
    TestDataOnHeap a(202);
    blockView.set(1, std::move(a));        // Lvalue moved => move-assign
    blockView.set(2, TestDataOnHeap(203)); // Rvalue

    validateEquals(stack.size(), static_cast<std::size_t>(3));

    validateEquals(stack[0].data()[0], 201);
    validateEquals(stack[1].data()[0], 202);
    validateEquals(stack[2].data()[0], 203);

    // --- In-place construction (no temporaries) ---
    blockView.emplace(0, 301);
    blockView.emplace(1, 302);
    blockView.emplace(2, 303);

    validateEquals(stack[0].data()[0], 301);
    validateEquals(stack[1].data()[0], 302);
    validateEquals(stack[2].data()[0], 303);
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(16);

    auto blockView = stack.pushBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));

    // Mix rvalues and moved lvalues
    blockView.set(0, TestDataOnHeapWithContainer(32, 201)); // Rvalue => move-assign
    TestDataOnHeapWithContainer a(32, 202);
    blockView.set(1, std::move(a));                         // Lvalue moved => move-assign
    blockView.set(2, TestDataOnHeapWithContainer(32, 203)); // Rvalue

    validateEquals(stack.size(), static_cast<std::size_t>(3));

    validateEquals(stack[0].at(5), 201);
    validateEquals(stack[1].at(5), 202);
    validateEquals(stack[2].at(5), 203);

    // --- In-place construction via emplace ---
    blockView.emplace(0, 32, 301);
    blockView.emplace(1, 32, 302);
    blockView.emplace(2, 32, 303);

    validateEquals(stack[0].at(5), 301);
    validateEquals(stack[1].at(5), 302);
    validateEquals(stack[2].at(5), 303);
  }
}

void peano4::stacks::tests::StackTest::testPopBlockView() {
  {
    Stack<TestDataOnStack> stack;
    stack.reserve(16);

    // Fill with 0..7
    for (int n = 0; n < 8; ++n) {
      stack.push(TestDataOnStack(n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(8));

    // Pop a block of 3 (should reference the last 3 elements: 5,6,7)
    auto blockView = stack.popBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));
    validateEquals(stack.size(), static_cast<std::size_t>(5)); // Logical shrink only

    validateEquals(blockView.get(0).data[0], 5);
    validateEquals(blockView.get(1).data[0], 6);
    validateEquals(blockView.get(2).data[0], 7);

    // Modify the popped view; it should not affect the (now smaller) stack.
    blockView.set(0, TestDataOnStack(999));
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    // Stack[4] was 4 before pop; still 4
    validateEquals(stack[4].data[0], 4);

    // --- In-place construction on popped view ---
    blockView.emplace(1, 1001);
    blockView.emplace(2, 1002);
    validateEquals(blockView.get(0).data[0], 999);
    validateEquals(blockView.get(1).data[0], 1001);
    validateEquals(blockView.get(2).data[0], 1002);

    // Stack head remains 0..4
    for (int n = 0; n < static_cast<int>(stack.size()); ++n) {
      validateEquals(stack[n].data[0], n);
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> stack;
    stack.reserve(16);

    for (int n = 0; n < 6; ++n) {
      stack.push(TestDataOnHeap(n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(6));

    auto blockView = stack.popBlock(2); // Should reference 4,5
    validateEquals(blockView.size(), static_cast<std::size_t>(2));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    validateEquals(blockView.get(0).data()[0], 4);
    validateEquals(blockView.get(1).data()[0], 5);

    // Mutate the popped view (move-assign).
    TestDataOnHeap moved(777);
    blockView.set(1, std::move(moved));
    validateEquals(blockView.get(1).data()[0], 777);

    // Stack still holds the first four elements 0..3
    for (int n = 0; n < static_cast<int>(stack.size()); ++n) {
      validateEquals(stack[n].data()[0], n);
    }

    // --- In-place construction on popped view ---
    blockView.emplace(0, 888);
    blockView.emplace(1, 889);
    validateEquals(blockView.get(0).data()[0], 888);
    validateEquals(blockView.get(1).data()[0], 889);

    // Stack still holds the first four elements 0..3
    for (int n = 0; n < static_cast<int>(stack.size()); ++n) {
      validateEquals(stack[n].data()[0], n);
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> stack;
    stack.reserve(16);

    for (int n = 0; n < 6; ++n) {
      stack.push(TestDataOnHeapWithContainer(32, n));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(6));

    auto blockView = stack.popBlock(2); // Should reference 4,5
    validateEquals(blockView.size(), static_cast<std::size_t>(2));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    validateEquals(blockView.get(0).at(5), 4);
    validateEquals(blockView.get(1).at(5), 5);

    // Mutate the popped view (move-assign).
    TestDataOnHeapWithContainer moved(32, 777);
    blockView.set(1, std::move(moved));
    validateEquals(blockView.get(1).at(5), 777);

    // Stack still holds the first four elements 0..3
    for (int n = 0; n < static_cast<int>(stack.size()); ++n) {
      validateEquals(stack[n].at(5), n);
    }

    // --- In-place construction on popped view ---
    blockView.emplace(0, 32, 888);
    blockView.emplace(1, 32, 889);
    validateEquals(blockView.get(0).at(5), 888);
    validateEquals(blockView.get(1).at(5), 889);

    // Stack still holds the first four elements 0..3
    for (int n = 0; n < static_cast<int>(stack.size()); ++n) {
      validateEquals(stack[n].at(5), n);
    }
  }

  // Edge case: pop the whole stack
  {
    Stack<TestDataOnStack> stack;
    for (int n = 0; n < 3; ++n) {
      stack.push(TestDataOnStack(10 + n));
    }
    auto blockView = stack.popBlock(3);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
    validateEquals(blockView.size(), static_cast<std::size_t>(3));
    validateEquals(blockView.get(0).data[0], 10);
    validateEquals(blockView.get(1).data[0], 11);
    validateEquals(blockView.get(2).data[0], 12);

    // --- In-place construction when stack is now empty (view still valid) ---
    blockView.emplace(0, 1234);
    blockView.emplace(2, 1236);
    validateEquals(blockView.get(0).data[0], 1234);
    validateEquals(blockView.get(1).data[0], 11);
    validateEquals(blockView.get(2).data[0], 1236);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
    validateEquals(blockView.size(), static_cast<std::size_t>(3));
  }
}

void peano4::stacks::tests::StackTest::testSendAndReceiveToSelf() {
  constexpr int NumberOfElements   = 5;
  constexpr int NumberOfContainers = 32;
  constexpr int Tag                = 123;

  const auto&    rank         = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator = rank.getCommunicator();
  const int      localRank    = rank.getRank();

  {
    Stack<TestDataOnStack> sender;
    Stack<TestDataOnStack> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        sender[n].data[i] = n * i;
      }
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(receiver[n].data[i], n * i);
      }
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> sender;
    StackWithSerialisation<TestDataOnHeap> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        sender[n].data()[i] = n * i;
      }
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(receiver[n].data()[i], n * i);
      }
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> sender;
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      sender[n] = TestDataOnHeapWithContainer(NumberOfContainers, n * 10);
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < NumberOfContainers; i++) {
        validateEquals(receiver[n].at(i), n * 10);
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testSendAndReceiveInARing() {
  constexpr int NumberOfElements   = 5;
  constexpr int NumberOfContainers = 32;
  constexpr int Tag                = 4321;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();
  const int      successor     = (localRank + 1) % numberOfRanks;
  const int      predecessor   = (localRank - 1 + numberOfRanks) % numberOfRanks;

  {
    Stack<TestDataOnStack> sender;
    Stack<TestDataOnStack> receiver;

    sender.resize(NumberOfElements);
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        sender[n].data[i] = localRank * 1000 + n * 10 + i;
      }
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnStack::Cardinality; i++) {
        validateEquals(receiver[n].data[i], predecessor * 1000 + n * 10 + i);
      }
    }
  }

  {
    StackWithSerialisation<TestDataOnHeap> sender;
    StackWithSerialisation<TestDataOnHeap> receiver;

    sender.resize(NumberOfElements);
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        sender[n].data()[i] = localRank * 1000 + n * 10 + i;
      }
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < TestDataOnHeap::Cardinality; i++) {
        validateEquals(receiver[n].data()[i], predecessor * 1000 + n * 10 + i);
      }
    }
  }

  {
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> sender;
    StackWithCustomSerialisation<TestDataOnHeapWithContainer> receiver;

    sender.resize(NumberOfElements);
    for (int n = 0; n < NumberOfElements; n++) {
      sender[n] = TestDataOnHeapWithContainer(NumberOfContainers, localRank * 1000 + n * 10);
    }

    receiver.startReceive(SendReceiveContext::BoundaryExchange, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(SendReceiveContext::BoundaryExchange, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int n = 0; n < NumberOfElements; n++) {
      for (int i = 0; i < NumberOfContainers; i++) {
        validateEquals(receiver[n].at(i), predecessor * 1000 + n * 10);
      }
    }
  }
}
