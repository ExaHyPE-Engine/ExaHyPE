// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/la/tests/ScalarTest.h"

#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"

tarch::la::tests::ScalarTest::ScalarTest():
  TestCase("tarch::la::tests::ScalarTest") {}

void tarch::la::tests::ScalarTest::run() {
  testMethod(testComparison);
  testMethod(testAbs);
  testMethod(testAdditionalScalarFunctions);
}

void tarch::la::tests::ScalarTest::testComparison() {
  double a   = 1.0;
  double b   = 2.0;
  double eps = 1e-14;

  validate(greater(b, a, eps));
  validate(!greater(a, a - eps, eps));
  validate(greater(a, a - 10.0 * eps, eps));

  validate(!greaterEquals(a, b, eps));
  validate(greaterEquals(b, a, eps));
  validate(greaterEquals(a, a, eps));
  validate(greaterEquals(a, a + 0.1 * eps, eps));
  validate(!greaterEquals(a, a + 10 * eps, eps));

  validate(smaller(a, b, eps));
  validate(!smaller(a, a + eps, eps));
  validate(smaller(a, a + 10.0 * eps, eps));

  validate(smallerEquals(a, b, eps));
  validate(smallerEquals(a, a + eps, eps));
  validate(smallerEquals(a + eps, a, eps));
  validate(smallerEquals(a, a + 10.0 * eps, eps));

  validate(!equals(a, b, eps));
  validate(equals(a, a, eps));
  validate(equals(a, a + eps, eps));
  validate(!equals(a, a + 10.0 * eps, eps));
  validate(equals(a, a + 10.0 * eps, 10.0 * eps));
}

void tarch::la::tests::ScalarTest::testAbs() {
  double a = -1.0;
  int    b = -1;

  validateEquals(abs(a), 1.0);
  validateEquals(abs(b), 1);
}

void tarch::la::tests::ScalarTest::testAdditionalScalarFunctions() {
  validateEquals(aPowI(3, 2), 8);
  validateEquals(sign(2.0), 1);
  validateEquals(sign(-2.0), -1);
  validateEquals(sign(1.0e-10), 0);
  validateEquals(round(2.4), 2);
  validateEquals(round(2.6), 3);
  validateEquals(round(2.6f), 3);

  validateNumericalEquals(relativeEps(10.0, 20.0, 1.0e-3), 2.0e-2);
  validateNumericalEquals(relativeEpsNormalisedAgainstValueGreaterOne(0.5, 0.25, 1.0e-3), 1.0e-3);
  validateNumericalEquals(relativeEpsNormalisedAgainstValueGreaterOne(10.0, 0.25, 1.0e-3), 1.0e-2);
  validateEquals(max(1.0, 3.0, 2.0), 3.0);
  validateEquals(min(1.0, 3.0, 2.0), 1.0);
  validateNumericalEquals(pow(4.0, 0.5), 2.0);
  validateNumericalEquals(convertAbsoluteIntoRelativeValue(10.0, 2.0), 0.2);
  validateNumericalEquals(convertAbsoluteIntoRelativeValue(0.5, 2.0), 2.0);
}
