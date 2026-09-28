// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestCaseTest.h"

namespace {
  /**
   * Returns the distance from magnitude to the next larger double.
   */
  double getUnitInTheLastPlaceOf(const double magnitude) {
    return std::nextafter(magnitude, std::numeric_limits<double>::infinity()) - magnitude;
  }
} // namespace

tarch::tests::TestCaseTest::TestCaseTest() :
  tarch::tests::TestCase("tarch::tests::TestCaseTest") {}

void
tarch::tests::TestCaseTest::run() {
  testMethod(testToleranceScalesWithTheLargestMagnitude);
  testMethod(testAZeroBufferGetsTheSmallestTolerance);
  testMethod(testBuffersWithinTheToleranceAreEqual);
  testMethod(testANaNComparesUnequal);
}

/**
 * The tolerance is MaxUnitsInTheLastPlace units in the last place of the largest magnitude, whatever its sign.
 */
void
tarch::tests::TestCaseTest::testToleranceScalesWithTheLargestMagnitude() {
  const std::vector<double> reference{1.0, -3.0e5, 2.0};
  validateEquals(
    tarch::tests::getToleranceInUnitsInTheLastPlace(reference),
    tarch::tests::MaxUnitsInTheLastPlace * getUnitInTheLastPlaceOf(3.0e5)
  );
}

/**
 * A buffer of zeros admits MaxUnitsInTheLastPlace times the smallest normal double.
 */
void
tarch::tests::TestCaseTest::testAZeroBufferGetsTheSmallestTolerance() {
  const std::vector<double> reference(4, 0.0);
  validateEquals(
    tarch::tests::getToleranceInUnitsInTheLastPlace(reference),
    tarch::tests::MaxUnitsInTheLastPlace * std::numeric_limits<double>::min()
  );
}

/**
 * Entries one unit short of the tolerance compare equal, and one unit beyond it do not.
 */
void
tarch::tests::TestCaseTest::testBuffersWithinTheToleranceAreEqual() {
  const std::vector<double> valid{1.68e5, -4.0, 0.0};
  const double              unit = getUnitInTheLastPlaceOf(1.68e5);
  std::vector<double>       actual{valid[0] + (tarch::tests::MaxUnitsInTheLastPlace - 1) * unit, valid[1], valid[2]};
  validateNumericalBuffersEquals(actual, valid);

  actual[2] = (tarch::tests::MaxUnitsInTheLastPlace + 1) * unit;
  validateTrue(std::abs(actual[2] - valid[2]) > tarch::tests::getToleranceInUnitsInTheLastPlace(valid));
}

/**
 * A NaN on either side of a numerical comparison compares unequal at every tolerance.
 */
void
tarch::tests::TestCaseTest::testANaNComparesUnequal() {
#if not defined(PEANO_WITH_FPE)
  // A build with floating-point traps aborts at an ordered comparison with a NaN.
  constexpr double NotANumber = std::numeric_limits<double>::quiet_NaN();
  constexpr double Tolerance  = std::numeric_limits<double>::max();
  validateTrue(testNumericalEqualsWithEps(1.0, -1.0, Tolerance));
  validateFalse(testNumericalEqualsWithEps(NotANumber, 1.0, Tolerance));
  validateFalse(testNumericalEqualsWithEps(1.0, NotANumber, Tolerance));
  validateFalse(testNumericalEqualsWithEps(NotANumber, NotANumber, Tolerance));
#endif
}
