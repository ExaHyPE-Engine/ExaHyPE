// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/Span.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("The index operator and the range loop visit exactly the size elements behind data") {
  GIVEN("A span of size three that starts at the second of the values 7, 3, 5, 11 and 2") {
    const std::vector<int>       values{7, 3, 5, 11, 2};
    const tarch::Span<const int> span{values.data() + 1, 3};

    WHEN("The range loop sums the elements of the span") {
      int sum = 0;
      for (const int value : span) {
        sum += value;
      }

      THEN("The span covers the three values from 3 to 11 and their sum is 19") {
        VALIDATE(not span.isEmpty());
        VALIDATE_EQUALS(span[0], 3);
        VALIDATE_EQUALS(span[2], 11);
        VALIDATE_EQUALS(span.end() - span.begin(), 3);
        VALIDATE_EQUALS(sum, 3 + 5 + 11);
      }
    }
  }

  GIVEN("A span of size two over the writable values 1 and 2") {
    std::vector<double>       writable{1.0, 2.0};
    const tarch::Span<double> target{writable.data(), 2};

    WHEN("The index operator writes four into the second element of the span") {
      target[1] = 4.0;

      THEN("The second writable value is four") { VALIDATE_EQUALS(writable[1], 4.0); }
    }
  }
}

UNIT_TEST("The empty span reports zero elements") {
  GIVEN("A span without data") {
    const tarch::Span<const int> empty{};

    WHEN("The range loop counts the elements of the span") {
      int visits = 0;
      for ([[maybe_unused]] const int value : empty) {
        visits++;
      }

      THEN("The span is empty, its size is zero, it ends where it begins and the loop makes zero visits") {
        VALIDATE(empty.isEmpty());
        VALIDATE_EQUALS(empty.size, 0);
        VALIDATE_EQUALS(empty.begin(), empty.end());
        VALIDATE_EQUALS(visits, 0);
      }
    }
  }
}
