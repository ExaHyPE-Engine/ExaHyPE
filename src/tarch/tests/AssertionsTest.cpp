// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AssertionsTest.h"

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace {
  /**
   * Returns twice value; the assertion holds for every non-negative value.
   */
  template <typename Value>
  constexpr Value getTwiceTheNonNegative(const Value value) {
    assertion(value >= Value{0}, value, tarch::la::Vector<2, int>(1));
    return 2 * value;
  }
} // namespace

tarch::tests::AssertionsTest::AssertionsTest() :
  tarch::tests::TestCase("tarch::tests::AssertionsTest") {}

void
tarch::tests::AssertionsTest::run() {
  testMethod(testAConstexprFunctionCarriesAnAssertion);
}

/**
 * A constexpr function with an assertion on a vector parameter evaluates at compile time and at run time.
 */
void
tarch::tests::AssertionsTest::testAConstexprFunctionCarriesAnAssertion() {
  constexpr int Twice = getTwiceTheNonNegative(21);
  validateEquals(Twice, 42);
  validateNumericalEquals(getTwiceTheNonNegative(1.5), 3.0);
}
