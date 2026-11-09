// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/NonCriticalAssertions.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("Adopting the rank -1 leaves the non-critical assertion rank at -1 and keeps the violation flag false") {
  GIVEN("The rank -1") {
    constexpr int NoRank = -1;

    WHEN("The test adopts the rank as the non-critical assertion rank") {
      tarch::adoptNonCriticalAssertionRank(NoRank);
      const int  nonCriticalAssertionRank = tarch::getNonCriticalAssertionRank();
      const bool hasBeenViolated          = tarch::hasNonCriticalAssertionBeenViolated();

      THEN("The non-critical assertion rank stays -1 and hasNonCriticalAssertionBeenViolated() returns false") {
        VALIDATE_EQUALS(nonCriticalAssertionRank, -1);
        VALIDATE(not hasBeenViolated);
      }
    }
  }
}
