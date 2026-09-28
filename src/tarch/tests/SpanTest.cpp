// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SpanTest.h"

#include "tarch/Span.h"

tarch::tests::SpanTest::SpanTest() :
  tarch::tests::TestCase("tarch::tests::SpanTest") {}

void
tarch::tests::SpanTest::run() {
  testMethod(testTheSpanReadsTheElementsBehindItsData);
  testMethod(testTheEmptySpanHasNoElements);
}

/**
 * The index operator and the range loop visit the size elements behind data and no other.
 */
void
tarch::tests::SpanTest::testTheSpanReadsTheElementsBehindItsData() {
  const std::vector<int>       values{7, 3, 5, 11, 2};
  const tarch::Span<const int> span{values.data() + 1, 3};
  validateFalse(span.isEmpty());
  validateEquals(span[0], 3);
  validateEquals(span[2], 11);
  validateEquals(span.end() - span.begin(), 3);
  int sum = 0;
  for (const int value : span) {
    sum += value;
  }
  validateEquals(sum, 3 + 5 + 11);

  std::vector<double>       writable{1.0, 2.0};
  const tarch::Span<double> target{writable.data(), 2};
  target[1] = 4.0;
  validateEquals(writable[1], 4.0);
}

void
tarch::tests::SpanTest::testTheEmptySpanHasNoElements() {
  const tarch::Span<const int> empty{};
  validateTrue(empty.isEmpty());
  validateEquals(empty.size, 0);
  validateEquals(empty.begin(), empty.end());
  int visits = 0;
  for ([[maybe_unused]] const int value : empty) {
    visits++;
  }
  validateEquals(visits, 0);
}
