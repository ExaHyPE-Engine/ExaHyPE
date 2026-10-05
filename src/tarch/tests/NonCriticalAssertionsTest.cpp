// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonCriticalAssertionsTest.h"

#include "tarch/NonCriticalAssertions.h"

tarch::tests::NonCriticalAssertionsTest::NonCriticalAssertionsTest() :
  tarch::tests::TestCase("tarch::tests::NonCriticalAssertionsTest") {}

void
tarch::tests::NonCriticalAssertionsTest::run() {
  testMethod(testNoAssertionIsNotAdopted);
}

/**
 * Adopting the rank -1 leaves the non-critical assertion rank at -1 and raises no violation.
 */
void
tarch::tests::NonCriticalAssertionsTest::testNoAssertionIsNotAdopted() {
  tarch::adoptNonCriticalAssertionRank(-1);
  validateEquals(tarch::getNonCriticalAssertionRank(), -1);
  validateTrue(not tarch::hasNonCriticalAssertionBeenViolated());
}
