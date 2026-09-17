// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackTest.h"

#include "peano4/grid/Stack.h"
#include "tarch/tests/TestMacros.h"

namespace {
  /**
   * Stands in for the plain records the stack carries, such as grid vertices
   * and the generated data-stack entries.
   */
  struct Record final {
    int value{0};

    Record()                                   = default;
    Record(const Record& other)                = default;
    Record(Record&& other) noexcept            = default;
    Record& operator=(const Record& other)     = default;
    Record& operator=(Record&& other) noexcept = default;
    explicit Record(int initialValue) :
      value(initialValue) {}

    ~Record() = default;
  };

  /**
   * A record that can be empty. A slot the stack hands out again still holds
   * the previous record, so an empty record assigned into it has to leave it
   * empty.
   */
  class RecordWithOptionalValue final {
  public:
    RecordWithOptionalValue() = default;

    RecordWithOptionalValue(const RecordWithOptionalValue& other) :
      _value(other._value == nullptr ? nullptr : new int(*other._value)) {}

    RecordWithOptionalValue(RecordWithOptionalValue&& other) noexcept :
      _value(std::exchange(other._value, nullptr)) {}

    RecordWithOptionalValue& operator=(const RecordWithOptionalValue& other) {
      RecordWithOptionalValue copy(other);
      *this = std::move(copy);
      return *this;
    }

    RecordWithOptionalValue& operator=(RecordWithOptionalValue&& other) noexcept {
      if (this != &other) {
        delete _value;
        _value = std::exchange(other._value, nullptr);
      }
      return *this;
    }

    explicit RecordWithOptionalValue(int value) :
      _value(new int(value)) {}

    ~RecordWithOptionalValue() { delete _value; }

    bool empty() const { return _value == nullptr; }

    int value() const {
      assertion(_value != nullptr);
      return *_value;
    }

  private:
    int* _value{nullptr};
  };
} // namespace

peano4::grid::tests::StackTest::StackTest() :
  TestCase("peano4::grid::tests::StackTest") {}

void peano4::grid::tests::StackTest::run() {
  testMethod(testPush);
  testMethod(testPop);
  testMethod(testTop);
  testMethod(testResize);
  testMethod(testClear);
  testMethod(testPushBlock);
  testMethod(testReusedSlotDropsPreviousValue);
}

/** Both overloads append, and the stack grows past its buffer on its own. */
void peano4::grid::tests::StackTest::testPush() {
  Stack<Record> stack;

  for (int value = 0; value < 5; value++) {
    const Record record(value);
    stack.push(record);
  }
  for (int value = 5; value < 10; value++) {
    stack.push(Record(value));
  }

  validateEquals(stack.size(), static_cast<std::size_t>(10));
  for (std::size_t index = 0; index < stack.size(); index++) {
    validateEquals(stack[index].value, static_cast<int>(index));
  }

  Stack<std::unique_ptr<int>> moveOnlyStack;
  moveOnlyStack.push(std::make_unique<int>(17));
  const auto movedValue = moveOnlyStack.pop();
  validateEquals(*movedValue, 17);
  validate(moveOnlyStack.empty());
}

/** Popping returns the top record and releases its slot. */
void peano4::grid::tests::StackTest::testPop() {
  Stack<Record> stack;
  stack.push(Record(500));
  stack.push(Record(600));

  validateEquals(stack.pop().value, 600);
  validateEquals(stack.size(), static_cast<std::size_t>(1));
  validateEquals(stack.pop().value, 500);
  validate(stack.empty());
}

/** The shift counts down from the top, so a shift of zero is the top record. */
void peano4::grid::tests::StackTest::testTop() {
  Stack<Record> stack;
  stack.push(Record(10));
  stack.push(Record(11));
  stack.push(Record(12));

  validateEquals(stack.top().value, 12);
  validateEquals(stack.top(1).value, 11);
  validateEquals(stack.top(2).value, 10);

  stack.top().value = 42;
  validateEquals(stack[2].value, 42);
}

/** Growing adds slots, shrinking releases them, and regrowing hands them back. */
void peano4::grid::tests::StackTest::testResize() {
  Stack<Record> stack;

  stack.resize(4);
  validateEquals(stack.size(), static_cast<std::size_t>(4));
  for (std::size_t index = 0; index < stack.size(); index++) {
    validateEquals(stack[index].value, 0);
  }

  stack[3].value = 7;
  stack.resize(2);
  validateEquals(stack.size(), static_cast<std::size_t>(2));

  stack.resize(4);
  validateEquals(stack.size(), static_cast<std::size_t>(4));
  validateEquals(stack[3].value, 7);
}

/** Both clears release every slot; only clear() also destroys the records. */
void peano4::grid::tests::StackTest::testClear() {
  Stack<Record> stack;
  stack.push(Record(111));
  stack.push(Record(222));

  stack.shallowClear();
  validate(stack.empty());

  stack.resize(2);
  validateEquals(stack[0].value, 111);

  stack.clear();
  validate(stack.empty());

  stack.resize(2);
  validateEquals(stack[0].value, 0);
}

/** A pushed block covers the new top slots and grows the stack by its size. */
void peano4::grid::tests::StackTest::testPushBlock() {
  Stack<Record> stack;

  auto block = stack.pushBlock(4);
  validateEquals(stack.size(), static_cast<std::size_t>(4));

  for (std::size_t index = 0; index < stack.size(); index++) {
    block.get(index) = Record(101 + static_cast<int>(index));
  }

  const auto& readOnlyBlock = block;
  for (std::size_t index = 0; index < stack.size(); index++) {
    validateEquals(stack[index].value, 101 + static_cast<int>(index));
    validateEquals(readOnlyBlock.get(index).value, 101 + static_cast<int>(index));
  }
}

/**
 * Every route into a slot the stack hands out again has to overwrite the whole
 * record, or a record that was emptied would come back holding its old value.
 */
void peano4::grid::tests::StackTest::testReusedSlotDropsPreviousValue() {
  Stack<RecordWithOptionalValue> stack;
  stack.push(RecordWithOptionalValue(7));

  validateEquals(stack.size(), static_cast<std::size_t>(1));
  validateEquals(stack[0].value(), 7);

  stack.shallowClear();

  const RecordWithOptionalValue empty;
  stack.push(empty);

  validateEquals(stack.size(), static_cast<std::size_t>(1));
  validate(stack[0].empty());
}
