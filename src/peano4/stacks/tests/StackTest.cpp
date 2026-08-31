// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTest.h"

#include "peano4/stacks/SendReceiveContext.h"
#include "peano4/stacks/Stack.h"
#include "peano4/stacks/StackWithCustomSerialisation.h"
#include "peano4/stacks/StackWithSerialisation.h"
#include "peano4/stacks/tests/TestDataOnHeap.h"
#include "peano4/stacks/tests/TestDataOnHeapWithContainer.h"
#include "peano4/stacks/tests/TestDataOnStack.h"
#include "tarch/Assertions.h"
#include "tarch/mpi/Rank.h"
#include "tarch/tasking/Task.h"
#include "tarch/tasking/TaskGraph.h"
#include "tarch/tests/TestMacros.h"

#include <mpi.h>

namespace {
  class NullableSlotData {
  public:
    NullableSlotData() = default;

    explicit NullableSlotData(const int value) :
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

  private:
    int* _value{nullptr};
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

  template <class StackType>
  class InitSendReceiveTask final : public tarch::tasking::Task {
  public:
    InitSendReceiveTask(
      StackType* const sendStack,
      StackType* const receiveStack,
      const int        sendRank,
      const int        receiveRank,
      const int        tag,
      const int        numberOfElements
    ) :
      _sendStack(sendStack),
      _receiveStack(receiveStack),
      _sendRank(sendRank),
      _receiveRank(receiveRank),
      _tag(tag),
      _numberOfElements(numberOfElements) {}
    InitSendReceiveTask(const InitSendReceiveTask&)            = delete;
    InitSendReceiveTask(InitSendReceiveTask&&)                 = delete;
    InitSendReceiveTask& operator=(const InitSendReceiveTask&) = delete;
    InitSendReceiveTask& operator=(InitSendReceiveTask&&)      = delete;
    ~InitSendReceiveTask() override                            = default;

    void run() override {
      _sendStack->startSend(peano4::stacks::SendReceiveContext::InterRank, _sendRank, _tag, MPI_COMM_WORLD);
      _receiveStack->startReceive(
        peano4::stacks::SendReceiveContext::InterRank,
        _receiveRank,
        _tag,
        MPI_COMM_WORLD,
        _numberOfElements
      );
    }

  private:
    StackType* const _sendStack;
    StackType* const _receiveStack;
    const int        _sendRank;
    const int        _receiveRank;
    const int        _tag;
    const int        _numberOfElements;
  };

  template <class StackType>
  class ProgressSendReceiveTask final : public tarch::tasking::Task {
  public:
    ProgressSendReceiveTask(std::vector<StackType*> sendStacks, std::vector<StackType*> receiveStacks) :
      _sendStacks(std::move(sendStacks)),
      _receiveStacks(std::move(receiveStacks)) {}
    ProgressSendReceiveTask(const ProgressSendReceiveTask&)            = delete;
    ProgressSendReceiveTask(ProgressSendReceiveTask&&)                 = delete;
    ProgressSendReceiveTask& operator=(const ProgressSendReceiveTask&) = delete;
    ProgressSendReceiveTask& operator=(ProgressSendReceiveTask&&)      = delete;
    ~ProgressSendReceiveTask() override                                = default;

    void run() override {
      bool allStacksFinished = false;
      while (not allStacksFinished) {
        allStacksFinished = true;
        for (std::size_t pairIndex = 0; pairIndex < _sendStacks.size(); pairIndex++) {
          allStacksFinished &= _sendStacks[pairIndex]->tryToFinishSendOrReceive();
          allStacksFinished &= _receiveStacks[pairIndex]->tryToFinishSendOrReceive();
        }
      }
    }

  private:
    const std::vector<StackType*> _sendStacks;
    const std::vector<StackType*> _receiveStacks;
  };

  /**
   * Every stack pair posts its send and receive in a task of its own, and one
   * further task polls every pair to completion.
   */
  template <class StackType, class StackInitializer, class StackChecker>
  void runTaskDrivenRingExchange(
    const int        numberOfElements,
    StackInitializer initialiseStack,
    StackChecker     checkStack
  ) {
    const int localRank     = tarch::mpi::Rank::getInstance().getRank();
    const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
    if (numberOfRanks == 1) {
      return;
    }

    constexpr int NumberOfStackPairs = 16;

    std::vector<std::unique_ptr<StackType>> sendStacks(NumberOfStackPairs);
    std::vector<std::unique_ptr<StackType>> receiveStacks(NumberOfStackPairs);

    std::vector<char> initialisationTokens(NumberOfStackPairs);
    char              completionToken{0};
    void* const       completionDependency = &completionToken;

    for (int pairIndex = 0; pairIndex < NumberOfStackPairs; pairIndex++) {
      sendStacks[pairIndex]    = std::make_unique<StackType>();
      receiveStacks[pairIndex] = std::make_unique<StackType>();
      sendStacks[pairIndex]->resize(numberOfElements);
      initialiseStack(pairIndex, *sendStacks[pairIndex]);
    }

    std::vector<void*>      initialisationDependencies(NumberOfStackPairs);
    std::vector<StackType*> sendStackPointers(NumberOfStackPairs);
    std::vector<StackType*> receiveStackPointers(NumberOfStackPairs);

    for (int pairIndex = 0; pairIndex < NumberOfStackPairs; pairIndex++) {
      const int successor   = (localRank + 1) % numberOfRanks;
      const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;
      const int tag         = 800 + pairIndex;

      tarch::tasking::spawnTask(
        new InitSendReceiveTask<StackType>(
          sendStacks[pairIndex].get(),
          receiveStacks[pairIndex].get(),
          successor,
          predecessor,
          tag,
          numberOfElements
        ),
        {},
        {&initialisationTokens[pairIndex]}
      );

      initialisationDependencies[pairIndex] = &initialisationTokens[pairIndex];
      sendStackPointers[pairIndex]          = sendStacks[pairIndex].get();
      receiveStackPointers[pairIndex]       = receiveStacks[pairIndex].get();
    }

    tarch::tasking::spawnTask(
      new ProgressSendReceiveTask<StackType>(sendStackPointers, receiveStackPointers),
      std::span<void* const>(initialisationDependencies.data(), initialisationDependencies.size()),
      std::span<void* const>(&completionDependency, 1)
    );

    tarch::tasking::waitForDependency(&completionToken);

    for (int pairIndex = 0; pairIndex < NumberOfStackPairs; pairIndex++) {
      checkStack(pairIndex, *sendStacks[pairIndex], *receiveStacks[pairIndex]);
    }
  }

  /** Element type declaring that its own merge() rebuilds a consolidated slot. */
  struct RebuildsByMerge {
    static constexpr bool RebuildsConsolidationSlotByMerge = true;
  };

  /** Element type declaring that an assignment rebuilds a consolidated slot. */
  struct AssignsOnConsolidation {
    static constexpr bool RebuildsConsolidationSlotByMerge = false;
  };

  struct DeclaresNothing {};

  /** Counts the merge() calls applyIncomingCopy() makes on one entity copy. */
  template <typename Declaration>
  struct MergeCountingValue : Declaration {
    int value{0};
    int numberOfMerges{0};

    bool receiveAndMerge(
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context
    ) const {
      return true;
    }

    void merge(
      [[maybe_unused]] const peano4::stacks::SendReceiveContext context,
      const MergeCountingValue&                                 neighbour,
      [[maybe_unused]] const int                                marker,
      [[maybe_unused]] const int                                treeId
    ) {
      value = neighbour.value;
      numberOfMerges++;
    }
  };
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
  testMethod(testSendAndReceiveThroughBaseReference);
  testMethod(testSendAndReceiveInARing);
  testMethod(testSendAndReceiveInARingFromTasks);
  testMethod(testTheConsolidationContextPicksThePathTheElementDeclares);
}

void peano4::stacks::tests::StackTest::testPushData() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(10);

    for (int value = 0; value < 5; value++) {
      stack.push(peano4::stacks::tests::TestDataOnStack(value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data[attributeIndex], static_cast<int>(elementIndex));
      }
    }

    for (int value = 5; value < 10; value++) {
      peano4::stacks::tests::TestDataOnStack testData(value);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data[attributeIndex], static_cast<int>(elementIndex));
      }
    }

    for (int value = 10; value < 15; value++) {
      const peano4::stacks::tests::TestDataOnStack testData(value);
      stack.push(testData);
    }
    validateEquals(stack.size(), static_cast<std::size_t>(15));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data[attributeIndex], static_cast<int>(elementIndex));
      }
    }

    // The elements stay constructed, so the copies land in the slots the earlier pushes filled.
    stack.shallowClear();
    for (int value = 0; value < 15; value++) {
      const peano4::stacks::tests::TestDataOnStack testData(2 * value);
      stack.push(testData);
    }
    validateEquals(stack.size(), static_cast<std::size_t>(15));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data[attributeIndex], 2 * static_cast<int>(elementIndex));
      }
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(10);

    for (int value = 0; value < 5; value++) {
      stack.push(peano4::stacks::tests::TestDataOnHeap(value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data()[attributeIndex], static_cast<int>(elementIndex));
      }
    }

    for (int value = 5; value < 10; value++) {
      peano4::stacks::tests::TestDataOnHeap testData(value);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        validateEquals(stack[elementIndex].data()[attributeIndex], static_cast<int>(elementIndex));
      }
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(10);

    for (int value = 0; value < 5; value++) {
      stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(5));

    for (int value = 5; value < 10; value++) {
      peano4::stacks::tests::TestDataOnHeapWithContainer testData(32, value);
      stack.push(std::move(testData));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(10));
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < 32; attributeIndex++) {
        validateEquals(stack[elementIndex].at(attributeIndex), static_cast<int>(elementIndex));
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testEraseData() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(4);

    stack.push(peano4::stacks::tests::TestDataOnStack(100));
    stack.push(peano4::stacks::tests::TestDataOnStack(200));
    stack.push(peano4::stacks::tests::TestDataOnStack(300));
    stack.push(peano4::stacks::tests::TestDataOnStack(400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validate(stack[elementIndex].data[attributeIndex] != 200);
      }
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(4);

    stack.push(peano4::stacks::tests::TestDataOnHeap(100));
    stack.push(peano4::stacks::tests::TestDataOnHeap(200));
    stack.push(peano4::stacks::tests::TestDataOnHeap(300));
    stack.push(peano4::stacks::tests::TestDataOnHeap(400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        validate(stack[elementIndex].data()[attributeIndex] != 200);
      }
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(4);

    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 100));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 200));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 300));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 400));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    stack.erase(1);
    validateEquals(stack.size(), static_cast<std::size_t>(3));

    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < 32; attributeIndex++) {
        validate(stack[elementIndex].at(attributeIndex) != 200);
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testClearData() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnStack(111));
    stack.push(peano4::stacks::tests::TestDataOnStack(222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnHeap(111));
    stack.push(peano4::stacks::tests::TestDataOnHeap(222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 111));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 222));
    stack.clear();
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackTest::testEmplaceData() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].data[0], 42);
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.emplace(42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].data()[0], 42);
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.emplace(32, 42);
    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].at(0), 42);
  }
}

void peano4::stacks::tests::StackTest::testPopData() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnStack(500));
    stack.push(peano4::stacks::tests::TestDataOnStack(600));

    const peano4::stacks::tests::TestDataOnStack topElement = stack.pop();
    validateEquals(topElement.data[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const peano4::stacks::tests::TestDataOnStack bottomElement = stack.pop();
    validateEquals(bottomElement.data[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnHeap(500));
    stack.push(peano4::stacks::tests::TestDataOnHeap(600));

    const peano4::stacks::tests::TestDataOnHeap topElement = stack.pop();
    validateEquals(topElement.data()[0], 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const peano4::stacks::tests::TestDataOnHeap bottomElement = stack.pop();
    validateEquals(bottomElement.data()[0], 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(2);
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 500));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 600));

    const peano4::stacks::tests::TestDataOnHeapWithContainer topElement = stack.pop();
    validateEquals(topElement.at(0), 600);
    validateEquals(stack.size(), static_cast<std::size_t>(1));

    const peano4::stacks::tests::TestDataOnHeapWithContainer bottomElement = stack.pop();
    validateEquals(bottomElement.at(0), 500);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
  }
}

void peano4::stacks::tests::StackTest::testClone() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(3);
    stack.push(peano4::stacks::tests::TestDataOnStack(1));
    stack.push(peano4::stacks::tests::TestDataOnStack(2));
    stack.push(peano4::stacks::tests::TestDataOnStack(3));

    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      validateEquals(clonedStack[elementIndex].data[0], stack[elementIndex].data[0]);
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(3);
    stack.push(peano4::stacks::tests::TestDataOnHeap(1));
    stack.push(peano4::stacks::tests::TestDataOnHeap(2));
    stack.push(peano4::stacks::tests::TestDataOnHeap(3));

    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      validateEquals(clonedStack[elementIndex].data()[0], stack[elementIndex].data()[0]);
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(3);
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 1));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 2));
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, 3));

    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> clonedStack;
    clonedStack.reserve(stack.size());
    clonedStack.clone(stack);

    validateEquals(clonedStack.size(), stack.size());
    for (std::size_t elementIndex = 0; elementIndex < stack.size(); elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < 32; attributeIndex++) {
        validateEquals(clonedStack[elementIndex].at(attributeIndex), stack[elementIndex].at(attributeIndex));
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testReusedSlotAssignmentClearsEmptyState() {
  {
    peano4::stacks::Stack<NullableSlotData> stack;
    stack.push(NullableSlotData(7));
    validateEquals(stack[0].value(), 7);

    stack.shallowClear();

    const NullableSlotData empty;
    stack.push(empty);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    peano4::stacks::Stack<NullableSlotData> stack;
    auto                                    initialBlock = stack.pushBlock(1);
    initialBlock.set(0, NullableSlotData(9));
    validateEquals(stack[0].value(), 9);

    stack.shallowClear();

    auto reusedBlock = stack.pushBlock(1);
    reusedBlock.set(0, NullableSlotData());

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    peano4::stacks::Stack<NullableSlotData> stack;
    auto                                    initialBlock = stack.pushBlock(1);
    initialBlock.set(0, NullableSlotData(10));
    validateEquals(stack[0].value(), 10);

    stack.shallowClear();

    auto reusedBlock = stack.pushBlock(1);
    reusedBlock.emplaceDefault(0);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].empty());
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.push(peano4::stacks::tests::TestDataOnHeap(11));
    validateEquals(stack[0].data()[0], 11);

    stack.shallowClear();

    peano4::stacks::tests::TestDataOnHeap       movedFrom(12);
    const peano4::stacks::tests::TestDataOnHeap owner(std::move(movedFrom));
    stack.push(movedFrom);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validate(stack[0].data() == nullptr);
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(1, 13));
    validateEquals(stack[0].at(0), 13);

    stack.shallowClear();

    const peano4::stacks::tests::TestDataOnHeapWithContainer empty;
    stack.push(empty);

    validateEquals(stack.size(), static_cast<std::size_t>(1));
    validateEquals(stack[0].size(), 0);
  }
}

void peano4::stacks::tests::StackTest::testPushBlockView() {
  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(16);

    auto blockView = stack.pushBlock(4);
    validateEquals(blockView.size(), static_cast<std::size_t>(4));

    blockView.set(0, peano4::stacks::tests::TestDataOnStack(101));
    blockView.set(1, peano4::stacks::tests::TestDataOnStack(102));

    peano4::stacks::tests::TestDataOnStack movedValue(103);
    blockView.set(2, std::move(movedValue));

    blockView.set(3, peano4::stacks::tests::TestDataOnStack(104));

    validateEquals(stack.size(), static_cast<std::size_t>(4));

    validateEquals(stack[0].data[0], 101);
    validateEquals(stack[1].data[0], 102);
    validateEquals(stack[2].data[0], 103);
    validateEquals(stack[3].data[0], 104);

    blockView.emplace(0, 201);
    blockView.emplace(1, 202);
    blockView.emplace(2, 203);
    blockView.emplace(3, 204);

    validateEquals(stack[0].data[0], 201);
    validateEquals(stack[1].data[0], 202);
    validateEquals(stack[2].data[0], 203);
    validateEquals(stack[3].data[0], 204);
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(16);

    auto blockView = stack.pushBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));

    blockView.set(0, peano4::stacks::tests::TestDataOnHeap(201));
    peano4::stacks::tests::TestDataOnHeap movedValue(202);
    blockView.set(1, std::move(movedValue));
    blockView.set(2, peano4::stacks::tests::TestDataOnHeap(203));

    validateEquals(stack.size(), static_cast<std::size_t>(3));

    validateEquals(stack[0].data()[0], 201);
    validateEquals(stack[1].data()[0], 202);
    validateEquals(stack[2].data()[0], 203);

    blockView.emplace(0, 301);
    blockView.emplace(1, 302);
    blockView.emplace(2, 303);

    validateEquals(stack[0].data()[0], 301);
    validateEquals(stack[1].data()[0], 302);
    validateEquals(stack[2].data()[0], 303);
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(16);

    auto blockView = stack.pushBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));

    blockView.set(0, peano4::stacks::tests::TestDataOnHeapWithContainer(32, 201));
    peano4::stacks::tests::TestDataOnHeapWithContainer movedValue(32, 202);
    blockView.set(1, std::move(movedValue));
    blockView.set(2, peano4::stacks::tests::TestDataOnHeapWithContainer(32, 203));

    validateEquals(stack.size(), static_cast<std::size_t>(3));

    validateEquals(stack[0].at(5), 201);
    validateEquals(stack[1].at(5), 202);
    validateEquals(stack[2].at(5), 203);

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
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    stack.reserve(16);

    for (int value = 0; value < 8; ++value) {
      stack.push(peano4::stacks::tests::TestDataOnStack(value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(8));

    auto blockView = stack.popBlock(3);
    validateEquals(blockView.size(), static_cast<std::size_t>(3));
    validateEquals(stack.size(), static_cast<std::size_t>(5));

    validateEquals(blockView.get(0).data[0], 5);
    validateEquals(blockView.get(1).data[0], 6);
    validateEquals(blockView.get(2).data[0], 7);

    blockView.set(0, peano4::stacks::tests::TestDataOnStack(999));
    validateEquals(stack.size(), static_cast<std::size_t>(5));
    validateEquals(stack[4].data[0], 4);

    blockView.emplace(1, 1001);
    blockView.emplace(2, 1002);
    validateEquals(blockView.get(0).data[0], 999);
    validateEquals(blockView.get(1).data[0], 1001);
    validateEquals(blockView.get(2).data[0], 1002);

    for (int elementIndex = 0; elementIndex < static_cast<int>(stack.size()); ++elementIndex) {
      validateEquals(stack[elementIndex].data[0], elementIndex);
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> stack;
    stack.reserve(16);

    for (int value = 0; value < 6; ++value) {
      stack.push(peano4::stacks::tests::TestDataOnHeap(value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(6));

    auto blockView = stack.popBlock(2);
    validateEquals(blockView.size(), static_cast<std::size_t>(2));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    validateEquals(blockView.get(0).data()[0], 4);
    validateEquals(blockView.get(1).data()[0], 5);

    peano4::stacks::tests::TestDataOnHeap moved(777);
    blockView.set(1, std::move(moved));
    validateEquals(blockView.get(1).data()[0], 777);

    for (int elementIndex = 0; elementIndex < static_cast<int>(stack.size()); ++elementIndex) {
      validateEquals(stack[elementIndex].data()[0], elementIndex);
    }

    blockView.emplace(0, 888);
    blockView.emplace(1, 889);
    validateEquals(blockView.get(0).data()[0], 888);
    validateEquals(blockView.get(1).data()[0], 889);

    for (int elementIndex = 0; elementIndex < static_cast<int>(stack.size()); ++elementIndex) {
      validateEquals(stack[elementIndex].data()[0], elementIndex);
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> stack;
    stack.reserve(16);

    for (int value = 0; value < 6; ++value) {
      stack.push(peano4::stacks::tests::TestDataOnHeapWithContainer(32, value));
    }
    validateEquals(stack.size(), static_cast<std::size_t>(6));

    auto blockView = stack.popBlock(2);
    validateEquals(blockView.size(), static_cast<std::size_t>(2));
    validateEquals(stack.size(), static_cast<std::size_t>(4));

    validateEquals(blockView.get(0).at(5), 4);
    validateEquals(blockView.get(1).at(5), 5);

    peano4::stacks::tests::TestDataOnHeapWithContainer moved(32, 777);
    blockView.set(1, std::move(moved));
    validateEquals(blockView.get(1).at(5), 777);

    for (int elementIndex = 0; elementIndex < static_cast<int>(stack.size()); ++elementIndex) {
      validateEquals(stack[elementIndex].at(5), elementIndex);
    }

    blockView.emplace(0, 32, 888);
    blockView.emplace(1, 32, 889);
    validateEquals(blockView.get(0).at(5), 888);
    validateEquals(blockView.get(1).at(5), 889);

    for (int elementIndex = 0; elementIndex < static_cast<int>(stack.size()); ++elementIndex) {
      validateEquals(stack[elementIndex].at(5), elementIndex);
    }
  }

  {
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> stack;
    for (int valueOffset = 0; valueOffset < 3; ++valueOffset) {
      stack.push(peano4::stacks::tests::TestDataOnStack(10 + valueOffset));
    }
    auto blockView = stack.popBlock(3);
    validateEquals(stack.size(), static_cast<std::size_t>(0));
    validateEquals(blockView.size(), static_cast<std::size_t>(3));
    validateEquals(blockView.get(0).data[0], 10);
    validateEquals(blockView.get(1).data[0], 11);
    validateEquals(blockView.get(2).data[0], 12);

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
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> sender;
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        sender[elementIndex].data[attributeIndex] = elementIndex * attributeIndex;
      }
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(receiver[elementIndex].data[attributeIndex], elementIndex * attributeIndex);
      }
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> sender;
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        sender[elementIndex].data()[attributeIndex] = elementIndex * attributeIndex;
      }
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        validateEquals(receiver[elementIndex].data()[attributeIndex], elementIndex * attributeIndex);
      }
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> sender;
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> receiver;

    sender.resize(NumberOfElements);
    validateEquals(sender.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      sender[elementIndex] = peano4::stacks::tests::TestDataOnHeapWithContainer(NumberOfContainers, elementIndex * 10);
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int containerIndex = 0; containerIndex < NumberOfContainers; containerIndex++) {
        validateEquals(receiver[elementIndex].at(containerIndex), elementIndex * 10);
      }
    }
  }
}

/**
 * A stack reached through a reference to its base still ships its own format.
 * A raw-byte transfer of a heap-backed element copies the pointer, so the
 * receiver would read the sender's storage.
 */
void peano4::stacks::tests::StackTest::testSendAndReceiveThroughBaseReference() {
  constexpr int NumberOfElements = 5;
  constexpr int Tag              = 5150;

  const auto&    rank         = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator = rank.getCommunicator();
  const int      localRank    = rank.getRank();

  peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> sender;
  peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> receiver;

  sender.resize(NumberOfElements);
  for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
    for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
         attributeIndex++) {
      sender[elementIndex].data()[attributeIndex] = elementIndex * attributeIndex;
    }
  }
  const peano4::stacks::tests::TestDataOnHeap::DoFType* const senderStorage = sender[0].data();

  peano4::stacks::Stack<peano4::stacks::tests::TestDataOnHeap>& senderThroughBase   = sender;
  peano4::stacks::Stack<peano4::stacks::tests::TestDataOnHeap>& receiverThroughBase = receiver;

  receiverThroughBase
    .startReceive(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator, NumberOfElements);
  senderThroughBase.startSend(peano4::stacks::SendReceiveContext::InterRank, localRank, Tag, communicator);
  completeTransfer(senderThroughBase, receiverThroughBase);

  validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
  for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
    for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
         attributeIndex++) {
      validateEquals(receiver[elementIndex].data()[attributeIndex], elementIndex * attributeIndex);
    }
  }
  validate(receiver[0].data() != senderStorage);
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
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> sender;
    peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack> receiver;

    sender.resize(NumberOfElements);
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        sender[elementIndex].data[attributeIndex] = localRank * 1000 + elementIndex * 10 + attributeIndex;
      }
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
           attributeIndex++) {
        validateEquals(
          receiver[elementIndex].data[attributeIndex],
          predecessor * 1000 + elementIndex * 10 + attributeIndex
        );
      }
    }
  }

  {
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> sender;
    peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap> receiver;

    sender.resize(NumberOfElements);
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        sender[elementIndex].data()[attributeIndex] = localRank * 1000 + elementIndex * 10 + attributeIndex;
      }
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
           attributeIndex++) {
        validateEquals(
          receiver[elementIndex].data()[attributeIndex],
          predecessor * 1000 + elementIndex * 10 + attributeIndex
        );
      }
    }
  }

  {
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> sender;
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer> receiver;

    sender.resize(NumberOfElements);
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      sender[elementIndex] =
        peano4::stacks::tests::TestDataOnHeapWithContainer(NumberOfContainers, localRank * 1000 + elementIndex * 10);
    }

    receiver
      .startReceive(peano4::stacks::SendReceiveContext::InterRank, predecessor, Tag, communicator, NumberOfElements);
    sender.startSend(peano4::stacks::SendReceiveContext::InterRank, successor, Tag, communicator);
    completeTransfer(sender, receiver);

    validateEquals(sender.size(), static_cast<std::size_t>(0));
    validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
    for (int elementIndex = 0; elementIndex < NumberOfElements; elementIndex++) {
      for (int containerIndex = 0; containerIndex < NumberOfContainers; containerIndex++) {
        validateEquals(receiver[elementIndex].at(containerIndex), predecessor * 1000 + elementIndex * 10);
      }
    }
  }
}

void peano4::stacks::tests::StackTest::testSendAndReceiveInARingFromTasks() {
  constexpr int NumberOfElements   = 5;
  constexpr int NumberOfContainers = 32;

  runTaskDrivenRingExchange<peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack>>(
    NumberOfElements,
    [](const int stackPairIndex, peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack>& sender) {
      for (int elementIndex = 0; elementIndex < static_cast<int>(sender.size()); elementIndex++) {
        for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
             attributeIndex++) {
          sender[elementIndex].data[attributeIndex] = 100 * stackPairIndex + 10 * elementIndex + attributeIndex;
        }
      }
    },
    [this](
      const int                                                      stackPairIndex,
      peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack>& sender,
      peano4::stacks::Stack<peano4::stacks::tests::TestDataOnStack>& receiver
    ) {
      validateEquals(sender.size(), static_cast<std::size_t>(0));
      validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
      for (int elementIndex = 0; elementIndex < static_cast<int>(receiver.size()); elementIndex++) {
        for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnStack::Cardinality;
             attributeIndex++) {
          validateEquals(
            receiver[elementIndex].data[attributeIndex],
            100 * stackPairIndex + 10 * elementIndex + attributeIndex
          );
        }
      }
    }
  );

  runTaskDrivenRingExchange<peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap>>(
    NumberOfElements,
    [](
      const int                                                                      stackPairIndex,
      peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap>& sender
    ) {
      for (int elementIndex = 0; elementIndex < static_cast<int>(sender.size()); elementIndex++) {
        for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
             attributeIndex++) {
          sender[elementIndex].data()[attributeIndex] = 200 * stackPairIndex + 10 * elementIndex + attributeIndex;
        }
      }
    },
    [this](
      const int                                                                      stackPairIndex,
      peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap>& sender,
      peano4::stacks::StackWithSerialisation<peano4::stacks::tests::TestDataOnHeap>& receiver
    ) {
      validateEquals(sender.size(), static_cast<std::size_t>(0));
      validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
      for (int elementIndex = 0; elementIndex < static_cast<int>(receiver.size()); elementIndex++) {
        for (int attributeIndex = 0; attributeIndex < peano4::stacks::tests::TestDataOnHeap::Cardinality;
             attributeIndex++) {
          validateEquals(
            receiver[elementIndex].data()[attributeIndex],
            200 * stackPairIndex + 10 * elementIndex + attributeIndex
          );
        }
      }
    }
  );

  runTaskDrivenRingExchange<
    peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>>(
    NumberOfElements,
    [](
      const int                                                                                         stackPairIndex,
      peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>& sender
    ) {
      for (int elementIndex = 0; elementIndex < static_cast<int>(sender.size()); elementIndex++) {
        sender[elementIndex] = peano4::stacks::tests::TestDataOnHeapWithContainer(
          NumberOfContainers,
          300 * stackPairIndex + 10 * elementIndex
        );
      }
    },
    [this](
      const int                                                                                         stackPairIndex,
      peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>& sender,
      peano4::stacks::StackWithCustomSerialisation<peano4::stacks::tests::TestDataOnHeapWithContainer>& receiver
    ) {
      validateEquals(sender.size(), static_cast<std::size_t>(0));
      validateEquals(receiver.size(), static_cast<std::size_t>(NumberOfElements));
      for (int elementIndex = 0; elementIndex < static_cast<int>(receiver.size()); elementIndex++) {
        for (int containerIndex = 0; containerIndex < NumberOfContainers; containerIndex++) {
          validateEquals(receiver[elementIndex].at(containerIndex), 300 * stackPairIndex + 10 * elementIndex);
        }
      }
    }
  );
}

/**
 * A consolidation overwrites the rank's slot. An element type that declares
 * RebuildsConsolidationSlotByMerge true overwrites it inside merge(), so it can
 * delete what the slot held. Every other element type is overwritten by an
 * assignment.
 */
void peano4::stacks::tests::StackTest::testTheConsolidationContextPicksThePathTheElementDeclares() {
  const auto applyOneCopy = [](auto& data, const peano4::stacks::SendReceiveContext context, const int incomingValue) {
    using Element               = std::remove_reference_t<decltype(data)>;
    int numberOfCopiesRequested = 0;
    peano4::stacks::applyIncomingCopy(data, context, 0, 0, [&]() {
      numberOfCopiesRequested++;
      Element incoming;
      incoming.value = incomingValue;
      return incoming;
    });
    return numberOfCopiesRequested;
  };

  MergeCountingValue<RebuildsByMerge>        rebuildingValue;
  MergeCountingValue<AssignsOnConsolidation> assigningValue;
  MergeCountingValue<DeclaresNothing>        silentValue;

  validateEquals(applyOneCopy(rebuildingValue, peano4::stacks::SendReceiveContext::Consolidation, 11), 1);
  validateEquals(rebuildingValue.value, 11);
  validateEquals(rebuildingValue.numberOfMerges, 1);

  validateEquals(applyOneCopy(assigningValue, peano4::stacks::SendReceiveContext::Consolidation, 22), 1);
  validateEquals(assigningValue.value, 22);
  validateEquals(assigningValue.numberOfMerges, 0);

  validateEquals(applyOneCopy(silentValue, peano4::stacks::SendReceiveContext::Consolidation, 33), 1);
  validateEquals(silentValue.value, 33);
  validateEquals(silentValue.numberOfMerges, 0);

  validateEquals(applyOneCopy(rebuildingValue, peano4::stacks::SendReceiveContext::IntraRank, 44), 1);
  validateEquals(rebuildingValue.numberOfMerges, 2);
  validateEquals(applyOneCopy(assigningValue, peano4::stacks::SendReceiveContext::InterRank, 55), 1);
  validateEquals(assigningValue.value, 55);
  validateEquals(assigningValue.numberOfMerges, 1);
  validateEquals(applyOneCopy(silentValue, peano4::stacks::SendReceiveContext::IntraRank, 66), 1);
  validateEquals(silentValue.value, 66);
  validateEquals(silentValue.numberOfMerges, 1);
}
