// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ScalarTest.h"

#include "tarch/la/Scalar.h"
#include "tarch/la/ScalarOperations.h"

tarch::la::tests::ScalarTest::ScalarTest() :
  TestCase("tarch::la::tests::ScalarTest") {}

void
tarch::la::tests::ScalarTest::run() {
  testMethod(testComparison);
  testMethod(testAbs);
  testMethod(testAdditionalScalarFunctions);
}

void
tarch::la::tests::ScalarTest::testComparison() {
  const double a   = 1.0;
  const double b   = 2.0;
  const double eps = 1e-14;

  validate(greater(b, a, eps));
  validate(not greater(a, a - eps, eps));
  validate(greater(a, a - 10.0 * eps, eps));

  validate(not greaterEquals(a, b, eps));
  validate(greaterEquals(b, a, eps));
  validate(greaterEquals(a, a, eps));
  validate(greaterEquals(a, a + 0.1 * eps, eps));
  validate(not greaterEquals(a, a + 10 * eps, eps));

  validate(smaller(a, b, eps));
  validate(not smaller(a, a + eps, eps));
  validate(smaller(a, a + 10.0 * eps, eps));

  validate(smallerEquals(a, b, eps));
  validate(smallerEquals(a, a + eps, eps));
  validate(smallerEquals(a + eps, a, eps));
  validate(smallerEquals(a, a + 10.0 * eps, eps));

  validate(not equals(a, b, eps));
  validate(equals(a, a, eps));
  validate(equals(a, a + eps, eps));
  validate(not equals(a, a + 10.0 * eps, eps));
  validate(equals(a, a + 10.0 * eps, 10.0 * eps));
}

void
tarch::la::tests::ScalarTest::testAbs() {
  const double a = -1.0;
  const int    b = -1;

  validateEquals(abs(a), 1.0);
  validateEquals(abs(b), 1);
}

void
tarch::la::tests::ScalarTest::testAdditionalScalarFunctions() {
  validateEquals(sign(2.0), 1);
  validateEquals(sign(-2.0), -1);
  validateEquals(sign(1.0e-10), 0);
  validateEquals(sign(-1.0e-10), 0);
  validateEquals(sign(0.0), 0);
  validateEquals(sign(0.5, 1.0), 0);
  validateEquals(sign(-0.5, 1.0), 0);
  validateEquals(sign(1.5, 1.0), 1);
  validateEquals(sign(-1.5, 1.0), -1);
  validateEquals(round(2.4), 2);
  validateEquals(round(2.6), 3);
  validateEquals(round(2.6F), 3);

  validateEquals(max(1.0, 3.0, 2.0), 3.0);
  validateEquals(min(1.0, 3.0, 2.0), 1.0);
  validateNumericalEquals(pow(4.0, 0.5), 2.0);
}
