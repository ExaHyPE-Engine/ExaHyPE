// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Returns the distance from magnitude to the next larger double.
   */
  double getUnitInTheLastPlaceOf(const double magnitude) {
    return std::nextafter(magnitude, std::numeric_limits<double>::infinity()) - magnitude;
  }

  /**
   * Counts how often the runs of one unit test pass through its setup and through each block that changes its counter.
   */
  struct RunsThroughTheBlocks final {
    int setup{0};
    int firstWhen{0};
    int andWhen{0};
    int secondWhen{0};
  };
} // namespace

UNIT_TEST("A true condition yields std::nullopt and a false one an empty description") {
  GIVEN("A true and a false condition") {
    constexpr bool TrueCondition  = true;
    constexpr bool FalseCondition = false;

    WHEN("The test checks both conditions") {
      const std::optional<std::string> success = tarch::testing::checkCondition(TrueCondition);
      const std::optional<std::string> failure = tarch::testing::checkCondition(FalseCondition);

      THEN("The true condition yields std::nullopt and the false condition an empty description") {
        VALIDATE(not success.has_value());
        VALIDATE(failure.has_value());
        VALIDATE(failure->empty(), *failure);
      }
    }
  }
}

UNIT_TEST("Integers of different signedness compare by value and -1 differs from the largest unsigned integer") {
  GIVEN("Three as an unsigned and a signed integer, -1, the largest unsigned integer, two strings and 2.5") {
    constexpr std::size_t  UnsignedThree   = 3;
    constexpr int          SignedThree     = 3;
    constexpr int          MinusOne        = -1;
    constexpr unsigned int LargestUnsigned = std::numeric_limits<unsigned int>::max();
    const std::string      name("peano");
    constexpr char         OtherName[] = "peano4";
    constexpr double       Value       = 2.5;

    WHEN("The test compares the threes, -1 with the largest unsigned integer, the two strings and 2.5 with itself") {
      const std::optional<std::string> threes      = tarch::testing::checkEquals(UnsignedThree, SignedThree);
      const std::optional<std::string> minusOne    = tarch::testing::checkEquals(MinusOne, LargestUnsigned);
      const std::optional<std::string> names       = tarch::testing::checkEquals(name, OtherName);
      const std::optional<std::string> sameDoubles = tarch::testing::checkEquals(Value, Value);

      THEN("The threes and the doubles compare equal and -1 and the largest unsigned integer and the strings differ") {
        VALIDATE(not threes.has_value());
        VALIDATE(minusOne.has_value());
        VALIDATE(names.has_value());
        VALIDATE(not sameDoubles.has_value());
      }
    }
  }
}

UNIT_TEST("Values within the numerical tolerance compare equal and values further apart fail with their difference") {
  GIVEN("The value one and the values half and twice the numerical tolerance above it") {
    constexpr double Value       = 1.0;
    constexpr double InsideAbove = Value + 0.5 * tarch::testing::NumericalTolerance;
    constexpr double BeyondAbove = Value + 2.0 * tarch::testing::NumericalTolerance;

    WHEN("The test compares one with both values") {
      const std::optional<std::string> success = tarch::testing::checkNumericalEquals(Value, InsideAbove);
      const std::optional<std::string> failure = tarch::testing::checkNumericalEquals(Value, BeyondAbove);

      THEN("The value within the tolerance compares equal and the other value fails with a description") {
        VALIDATE(not success.has_value());
        VALIDATE(failure.has_value());
        VALIDATE(failure->starts_with("The difference "), *failure);
      }
    }
  }
}

UNIT_TEST("The tolerance of a buffer scales the unit in the last place of its largest magnitude, whatever its sign") {
  GIVEN("A buffer of 1, -3e5 and 2") {
    const std::vector<double> reference{1.0, -3.0e5, 2.0};

    WHEN("The test takes the tolerance of the buffer") {
      const double tolerance = tarch::testing::getToleranceInUnitsInTheLastPlace(reference);

      THEN("The tolerance is the maximum number of units in the last place of 3e5") {
        VALIDATE_EQUALS(tolerance, tarch::testing::MaxUnitsInTheLastPlace * getUnitInTheLastPlaceOf(3.0e5));
      }
    }
  }
}

UNIT_TEST("A buffer of zeros admits the maximum number of units in the last place times the smallest normal double") {
  GIVEN("A buffer of four zeros") {
    const std::vector<double> reference(4, 0.0);

    WHEN("The test takes the tolerance of the buffer") {
      const double tolerance = tarch::testing::getToleranceInUnitsInTheLastPlace(reference);

      THEN("The tolerance is the maximum number of units in the last place times the smallest normal double") {
        VALIDATE_EQUALS(tolerance, tarch::testing::MaxUnitsInTheLastPlace * std::numeric_limits<double>::min());
      }
    }
  }
}

UNIT_TEST("Entries one unit short of the tolerance compare equal and entries one unit beyond it differ") {
  GIVEN("The buffer 1.68e5, -4, 0 and a copy whose first entry lies one unit in the last place inside the tolerance") {
    const std::vector<double> valid{1.68e5, -4.0, 0.0};
    const double              unit = getUnitInTheLastPlaceOf(1.68e5);
    std::vector<double> actual{valid[0] + (tarch::testing::MaxUnitsInTheLastPlace - 1) * unit, valid[1], valid[2]};

    THEN("The buffers compare equal") { VALIDATE_NUMERICAL_BUFFERS_EQUALS(actual, valid); }

    WHEN("The test moves the last entry of the copy one unit in the last place beyond the tolerance") {
      actual[2]                                = (tarch::testing::MaxUnitsInTheLastPlace + 1) * unit;
      const std::optional<std::string> failure = tarch::testing::checkNumericalBuffersEquals(actual, valid);

      THEN("The buffers differ in one of their three entries") {
        VALIDATE(failure.has_value());
        VALIDATE(failure->starts_with("1 of 3 entries differ"), *failure);
      }
    }
  }
}

UNIT_TEST("Buffers of different sizes differ whatever their entries") {
  GIVEN("The buffers 1 and 1, 2") {
    const std::vector<double> shorter{1.0};
    const std::vector<double> longer{1.0, 2.0};

    WHEN("The test compares the buffers") {
      const std::optional<std::string> failure = tarch::testing::checkNumericalBuffersEquals(shorter, longer);

      THEN("The comparison fails") { VALIDATE(failure.has_value()); }
    }
  }
}

#if not defined(PEANO_WITH_FPE)
UNIT_TEST("A NaN on either side of a numerical comparison fails the check") {
  GIVEN("A NaN, the value one, the buffer 1, 2 and the buffer 1, NaN") {
    constexpr double          NotANumber = std::numeric_limits<double>::quiet_NaN();
    constexpr double          Value      = 1.0;
    const std::vector<double> valid{1.0, 2.0};
    const std::vector<double> actual{1.0, NotANumber};

    WHEN("The test compares the NaN and one in both orders and then the buffers") {
      const std::optional<std::string> failureOfLeftNaN  = tarch::testing::checkNumericalEquals(NotANumber, Value);
      const std::optional<std::string> failureOfRightNaN = tarch::testing::checkNumericalEquals(Value, NotANumber);
      const std::optional<std::string> failureOfBuffers  = tarch::testing::checkNumericalBuffersEquals(actual, valid);

      THEN("Every comparison fails") {
        VALIDATE(failureOfLeftNaN.has_value());
        VALIDATE(failureOfRightNaN.has_value());
        VALIDATE(failureOfBuffers.has_value());
      }
    }
  }
}
#endif

UNIT_TEST("The writable test directory exists") {
  WHEN("The test takes the writable test directory") {
    const std::filesystem::path directory = tarch::testing::getWritableTestDirectory();

    THEN("The directory exists") { VALIDATE(std::filesystem::is_directory(directory)); }
  }
}

UNIT_TEST("Every innermost block runs once in source order and every run repeats the code of its enclosing blocks") {
  static RunsThroughTheBlocks runs;

  GIVEN("A counter that every run sets to zero") {
    runs.setup++;
    int counter = 0;

    WHEN("The counter rises by one") {
      runs.firstWhen++;
      counter++;

      THEN("The counter is one") { VALIDATE_EQUALS(counter, 1); }

      AND_WHEN("The counter rises by two more") {
        runs.andWhen++;
        counter += 2;

        THEN("The counter is three") { VALIDATE_EQUALS(counter, 3); }
      }
    }

    WHEN("The counter falls by one") {
      runs.secondWhen++;
      counter--;

      THEN("The counter is minus one, the setup ran in all three runs and each change once per innermost block") {
        VALIDATE_EQUALS(counter, -1);
        VALIDATE_EQUALS(runs.setup, 3);
        VALIDATE_EQUALS(runs.firstWhen, 2);
        VALIDATE_EQUALS(runs.andWhen, 1);
        VALIDATE_EQUALS(runs.secondWhen, 1);
      }
    }
  }
}
