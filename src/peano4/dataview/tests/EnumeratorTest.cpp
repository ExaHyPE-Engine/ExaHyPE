// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EnumeratorTest.h"

#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "tarch/tests/TestMacros.h"

namespace {
  struct Item final {
    int value{0};
  };
} // namespace

peano4::dataview::tests::EnumeratorTest::EnumeratorTest() :
  TestCase("peano4::dataview::tests::EnumeratorTest") {}

void peano4::dataview::tests::EnumeratorTest::run() {
  testMethod(testVertexEnumeratorFollowsItsSlots);
  testMethod(testFaceEnumeratorFollowsItsSlots);
  testMethod(testEnumeratorWritesThrough);
  testMethod(testPositionWithoutEntityReadsTheSentinel);
}

void peano4::dataview::tests::EnumeratorTest::testVertexEnumeratorFollowsItsSlots() {
  // A cell's entities are scattered over the pool, so the slots are neither
  // ordered nor contiguous.
  Item pool[16];
  for (int index = 0; index < 16; index++) {
    pool[index].value = index;
  }

  int slots[TWO_POWER_D];
  for (int index = 0; index < TWO_POWER_D; index++) {
    slots[index] = 15 - 2 * index;
  }

  const peano4::dataview::VertexEnumerator<Item> enumerator(pool, slots, 15);
  for (int index = 0; index < TWO_POWER_D; index++) {
    validateEquals(enumerator(index).value, 15 - 2 * index);
  }
}

void peano4::dataview::tests::EnumeratorTest::testFaceEnumeratorFollowsItsSlots() {
  Item pool[16];
  for (int index = 0; index < 16; index++) {
    pool[index].value = index;
  }

  int slots[TWO_TIMES_D];
  for (int index = 0; index < TWO_TIMES_D; index++) {
    slots[index] = 2 * index + 1;
  }

  const peano4::dataview::FaceEnumerator<Item> enumerator(pool, slots, 15);
  for (int index = 0; index < TWO_TIMES_D; index++) {
    validateEquals(enumerator(index).value, 2 * index + 1);
  }
}

void peano4::dataview::tests::EnumeratorTest::testEnumeratorWritesThrough() {
  Item pool[16];
  int  slots[TWO_POWER_D];
  for (int index = 0; index < TWO_POWER_D; index++) {
    slots[index] = index + 3;
  }

  const peano4::dataview::VertexEnumerator<Item> enumerator(pool, slots, 15);
  enumerator(0).value = 42;

  validateEquals(pool[3].value, 42);
}

void peano4::dataview::tests::EnumeratorTest::testPositionWithoutEntityReadsTheSentinel() {
  Item pool[16];
  for (int index = 0; index < 16; index++) {
    pool[index].value = index;
  }

  // A cell next to another rank has positions no local entity backs. Their
  // slots are negative and every one of them resolves to the sentinel.
  int slots[TWO_TIMES_D];
  for (int index = 0; index < TWO_TIMES_D; index++) {
    slots[index] = index == 0 ? 4 : -1;
  }

  const peano4::dataview::FaceEnumerator<Item> enumerator(pool, slots, 15);

  validateEquals(enumerator(0).value, 4);
  for (int index = 1; index < TWO_TIMES_D; index++) {
    validateEquals(enumerator(index).value, 15);
  }
}
