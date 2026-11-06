// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/Assertions.h"

#include "tarch/la/Vector.h"
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Returns twice the non-negative value.
   */
  template <typename Value>
  constexpr Value getTwiceTheNonNegative(const Value value) {
    ASSERTION(value >= Value{0}, value, tarch::la::Vector<2, int>(1));
    return 2 * value;
  }
} // namespace

UNIT_TEST("A constexpr function with an assertion on a vector parameter evaluates at compile time and at run time") {
  GIVEN("The integer 21 and the double 1.5") {
    constexpr int    IntegerValue = 21;
    constexpr double DoubleValue  = 1.5;

    WHEN("The function doubles the integer at compile time and the double at run time") {
      constexpr int TwiceTheInteger = getTwiceTheNonNegative(IntegerValue);
      const double  twiceTheDouble  = getTwiceTheNonNegative(DoubleValue);

      THEN("The doubled values are 42 and three") {
        VALIDATE_EQUALS(TwiceTheInteger, 42);
        VALIDATE_NUMERICAL_EQUALS(twiceTheDouble, 3.0);
      }
    }
  }
}
