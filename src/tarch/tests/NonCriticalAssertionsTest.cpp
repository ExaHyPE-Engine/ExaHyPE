// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonCriticalAssertionsTest.h"

#include "tarch/NonCriticalAssertions.h"

tarch::tests::NonCriticalAssertionsTest::NonCriticalAssertionsTest() :
  TestCase("tarch::tests::NonCriticalAssertionsTest") {}

void tarch::tests::NonCriticalAssertionsTest::run() { testMethod(testNoAssertionIsNotAdopted); }

/**
 * A violation is a one-off event that no operation can undo, so a test may only exercise
 * the path that raises nothing. That path runs on every grid sweep: a caller reduces the
 * asserting rank over all ranks and feeds the result back, so a run in which the adoption
 * misread "nobody asserted" would wind down on its first sweep.
 */
void tarch::tests::NonCriticalAssertionsTest::testNoAssertionIsNotAdopted() {
  tarch::adoptNonCriticalAssertionRank(-1);
  validateEquals(tarch::getNonCriticalAssertionRank(), -1);
  validateTrue(not tarch::hasNonCriticalAssertionBeenViolated());
}
