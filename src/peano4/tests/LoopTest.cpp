// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoopTest.h"

#include "peano4/Loop.h"

#include <set>
#include <vector>

peano4::tests::LoopTest::LoopTest():
  TestCase("peano4::tests::LoopTest") {}

void peano4::tests::LoopTest::run() {
  testMethod([this]() { testDForVisitsAllPointsInLinearOrder(); });
  testMethod([this]() { testDForWithPerAxisRange(); });
  testMethod([this]() { testDIncOverloadConsistency(); });
  testMethod([this]() { testDLinearised(); });
  testMethod([this]() { testDForeKeepsFixedDimensionAndIsComplete(); });
  testMethod([this]() { testDCmp(); });
  testMethod([this]() { testDFor2EnumeratesCornersInLinearisedOrder(); });
  testMethod([this]() { testDFor3EnumeratesStencil(); });
  testMethod([this]() { testZFor3IsAHamiltonianPath(); });
  testMethod([this]() { testGenericZForIsAHamiltonianPath(); });
  testMethod([this]() { testGenericZForMatchesZFor3(); });
}

void peano4::tests::LoopTest::testDForVisitsAllPointsInLinearOrder() {
  for (int max = 1; max <= 5; max++) {
    int expected = 0;
    dfor(k, max) {
      // independent linearisation: component 0 runs fastest
      int manualIndex = 0;
      int factor      = 1;
      for (int d = 0; d < DIMENSIONS; d++) {
        manualIndex += k(d) * factor;
        factor *= max;
      }
      validateEqualsWithParams1(manualIndex, expected, k);
      expected++;
    }
    validateEquals(expected, tarch::la::aPowI(DIMENSIONS, max));
  }
}

void peano4::tests::LoopTest::testDForWithPerAxisRange() {
  // distinct per-axis extents, and one degenerate axis of extent 1 in every
  // position (this is what the blockstructured face loops do).
  for (int flatDimension = -1; flatDimension < DIMENSIONS; flatDimension++) {
    tarch::la::Vector<DIMENSIONS, int> range;
    long                               total = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      range(d) = (d == flatDimension) ? 1 : (2 + d);
      total *= range(d);
    }

    long count = 0;
    dfor(k, range) {
      // independent mixed-radix expectation, component 0 fastest
      long t = count;
      for (int d = 0; d < DIMENSIONS; d++) {
        validateEqualsWithParams2(k(d), static_cast<int>(t % range(d)), k, range);
        t /= range(d);
      }
      count++;
    }
    validateEqualsWithParams1(count, total, range);
  }
}

void peano4::tests::LoopTest::testDIncOverloadConsistency() {
  // The scalar dInc(counter,int) and the per-axis dInc(counter,Vector) are the
  // same operation, so they must agree for every counter - also for
  // non-canonical counters whose components already reach/exceed max. We sweep
  // each component over [0,max] so that genuinely non-canonical states (a
  // component == max) are included.
  const int                          max = 3;
  tarch::la::Vector<DIMENSIONS, int> uniformMax(max);

  tarch::la::Vector<DIMENSIONS, int> counter(0);
  bool                               done = false;
  while (not done) {
    tarch::la::Vector<DIMENSIONS, int> viaScalar = counter;
    tarch::la::Vector<DIMENSIONS, int> viaVector = counter;
    peano4::dInc(viaScalar, max);
    peano4::dInc(viaVector, uniformMax);
    for (int d = 0; d < DIMENSIONS; d++) {
      validateEqualsWithParams1(viaScalar(d), viaVector(d), counter);
    }

    // odometer over [0,max] per component (max+1 values) to reach non-canonical states
    for (int d = 0; d < DIMENSIONS; d++) {
      counter(d)++;
      if (counter(d) <= max) {
        break;
      }
      counter(d) = 0;
      if (d == DIMENSIONS - 1) {
        done = true;
      }
    }
  }
}

void peano4::tests::LoopTest::testDLinearised() {
  const int max = 4;
  dfor(k, max) {
    int manualIndex = 0;
    int factor      = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      manualIndex += k(d) * factor;
      factor *= max;
    }
    validateEqualsWithParams1(peano4::dLinearised(k, max), manualIndex, k);
  }
}

void peano4::tests::LoopTest::testDForeKeepsFixedDimensionAndIsComplete() {
  for (int fixedDimension = 0; fixedDimension < DIMENSIONS; fixedDimension++) {
    for (int max = 2; max <= 4; max++) {
      std::set<int> visited;
      dfore(k, max, fixedDimension, 0) {
        validateEqualsWithParams1(k(fixedDimension), 0, k);
        int linearised = peano4::dLinearised(k, max);
        validateWithParams1(visited.find(linearised) == visited.end(), k);
        visited.insert(linearised);
      }
      int expectedCount = 1;
      for (int d = 0; d < DIMENSIONS; d++) {
        if (d != fixedDimension) {
          expectedCount *= max;
        }
      }
      validateEqualsWithParams2(static_cast<int>(visited.size()), expectedCount, fixedDimension, max);
    }
  }
}

void peano4::tests::LoopTest::testDCmp() {
  const int                          max = 3;
  tarch::la::Vector<DIMENSIONS, int> inRange(0);
  validate(peano4::dCmp(inRange, max));

  tarch::la::Vector<DIMENSIONS, int> atUpperBound(0);
  atUpperBound(DIMENSIONS - 1) = max; // the dfor terminal state
  validate(not peano4::dCmp(atUpperBound, max));

  // per-axis overload
  tarch::la::Vector<DIMENSIONS, int> range;
  for (int d = 0; d < DIMENSIONS; d++) {
    range(d) = 2 + d;
  }
  tarch::la::Vector<DIMENSIONS, int> insideBox(0);
  validate(peano4::dCmp(insideBox, range));
  tarch::la::Vector<DIMENSIONS, int> onBoxBoundary(0);
  onBoxBoundary(0) = range(0);
  validate(not peano4::dCmp(onBoxBoundary, range));
}

void peano4::tests::LoopTest::testDFor2EnumeratesCornersInLinearisedOrder() {
  std::set<int> visited;
  int           count = 0;
  dfor2(k) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) == 0 or k(d) == 1, k);
    }
    validateEqualsWithParams1(kScalar, peano4::dLinearised(k, 2), k);
    visited.insert(kScalar);
    count++;
  }
  enddforx
  validateEquals(count, TWO_POWER_D);
  validateEquals(static_cast<int>(visited.size()), TWO_POWER_D);
}

void peano4::tests::LoopTest::testDFor3EnumeratesStencil() {
  int expected = 0;
  dfor3(k) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) >= 0 and k(d) < 3, k);
    }
    validateEqualsWithParams1(peano4::dLinearised(k, 3), expected, k);
    expected++;
  }
  enddforx
  validateEquals(expected, THREE_POWER_D);
}

void peano4::tests::LoopTest::testZFor3IsAHamiltonianPath() {
  for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
    peano4::LoopDirection direction(initialDirection);
    std::set<int>         visited;
    int                   count = 0;
    zfor3(k, direction) {
      for (int d = 0; d < DIMENSIONS; d++) {
        validateWithParams1(k(d) >= 0 and k(d) < 3, k);
      }
      visited.insert(peano4::dLinearised(k, 3));
      count++;
    }
    endzfor
    validateEqualsWithParams1(count, THREE_POWER_D, initialDirection);
    validateEqualsWithParams1(static_cast<int>(visited.size()), THREE_POWER_D, initialDirection);
  }
}

void peano4::tests::LoopTest::testGenericZForIsAHamiltonianPath() {
  for (int max = 2; max <= 5; max++) {
    for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
      peano4::LoopDirection direction(initialDirection);
      std::set<int>         visited;
      int                   count = 0;
      zfor(k, max, direction) {
        for (int d = 0; d < DIMENSIONS; d++) {
          validateWithParams1(k(d) >= 0 and k(d) < max, k);
        }
        visited.insert(peano4::dLinearised(k, max));
        count++;
      }
      endzfor
      validateEqualsWithParams2(count, tarch::la::aPowI(DIMENSIONS, max), max, initialDirection);
      validateEqualsWithParams2(static_cast<int>(visited.size()), tarch::la::aPowI(DIMENSIONS, max), max, initialDirection);
    }
  }
}

void peano4::tests::LoopTest::testGenericZForMatchesZFor3() {
  const int max = 3;
  for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
    std::vector<int> genericSequence;
    {
      peano4::LoopDirection direction(initialDirection);
      zfor(k, max, direction) {
        genericSequence.push_back(peano4::dLinearised(k, max));
      }
      endzfor
    }

    std::vector<int> tableSequence;
    {
      peano4::LoopDirection direction(initialDirection);
      zfor3(k, direction) {
        tableSequence.push_back(peano4::dLinearised(k, max));
      }
      endzfor
    }

    validateEqualsWithParams1(static_cast<int>(genericSequence.size()), static_cast<int>(tableSequence.size()), initialDirection);
    for (int i = 0; i < static_cast<int>(tableSequence.size()); i++) {
      validateEqualsWithParams2(genericSequence[i], tableSequence[i], initialDirection, i);
    }
  }
}
