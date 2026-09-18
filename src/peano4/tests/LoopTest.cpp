// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoopTest.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

peano4::tests::LoopTest::LoopTest() :
  TestCase("peano4::tests::LoopTest") {}

void peano4::tests::LoopTest::run() {
  testMethod(testDForEachVisitsAllPointsInLinearOrder);
  testMethod(testDForEachWithPerAxisRange);
  testMethod(testDStartVector);
  testMethod(testDIncrementOverloadConsistency);
  testMethod(testDIncrementExclusive);
  testMethod(testDLinearised);
  testMethod(testDForEachExclusiveKeepsFixedDimensionAndIsComplete);
  testMethod(testDCompare);
  testMethod(testDCompareExclusive);
  testMethod(testDForEachCornerEnumeratesCornersInLinearisedOrder);
  testMethod(testDForEachStencilEnumeratesStencil);
}

/**
 * dForEach(max, body) visits every point of [0,max)^d exactly once in
 * dLinearised order, with the 0th component running fastest.
 */
void peano4::tests::LoopTest::testDForEachVisitsAllPointsInLinearOrder() {
  for (int max = 1; max <= 5; max++) {
    int expected = 0;
    peano4::dForEach(max, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      int manualIndex = 0;
      int factor      = 1;
      for (int d = 0; d < DIMENSIONS; d++) {
        manualIndex += k(d) * factor;
        factor *= max;
      }
      validateEqualsWithParams1(manualIndex, expected, k);
      expected++;
    });
    validateEquals(expected, tarch::la::aPowI(DIMENSIONS, max));
  }
}

/**
 * dForEach(maxVector, body) with individual per-axis ranges, including a
 * degenerate axis of extent one, has to enumerate the full box exactly once.
 */
void peano4::tests::LoopTest::testDForEachWithPerAxisRange() {
  for (int flatDimension = -1; flatDimension < DIMENSIONS; flatDimension++) {
    tarch::la::Vector<DIMENSIONS, int> range;
    long                               total = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      range(d) = (d == flatDimension) ? 1 : (2 + d);
      total *= range(d);
    }

    long count = 0;
    peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      long remainder = count;
      for (int d = 0; d < DIMENSIONS; d++) {
        validateEqualsWithParams2(k(d), static_cast<int>(remainder % range(d)), k, range);
        remainder /= range(d);
      }
      count++;
    });
    validateEqualsWithParams1(count, total, range);
  }
}

/**
 * The scalar dIncrement(counter,int) and the vector dIncrement(counter,Vector)
 * implement the same operation and must agree for every counter, canonical or
 * not.
 */
void peano4::tests::LoopTest::testDIncrementOverloadConsistency() {
  const int                                max = 3;
  const tarch::la::Vector<DIMENSIONS, int> uniformMax(max);

  tarch::la::Vector<DIMENSIONS, int> counter(0);
  bool                               done = false;
  while (not done) {
    tarch::la::Vector<DIMENSIONS, int> viaScalar = counter;
    tarch::la::Vector<DIMENSIONS, int> viaVector = counter;
    peano4::dIncrement(viaScalar, max);
    peano4::dIncrement(viaVector, uniformMax);
    for (int d = 0; d < DIMENSIONS; d++) {
      validateEqualsWithParams1(viaScalar(d), viaVector(d), counter);
    }

    // Every component runs up to max, so the sweep also reaches the
    // non-canonical counters whose components equal max.
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

/**
 * dStartVector seeds a d-dimensional loop: The plain overload is the zero
 * vector and dStartVector(dim,value) is zero except for the given dimension.
 */
void peano4::tests::LoopTest::testDStartVector() {
  const auto zero = peano4::dStartVector();
  for (int d = 0; d < DIMENSIONS; d++) {
    validateEquals(zero(d), 0);
  }

  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    const auto seed = peano4::dStartVector(dimension, 7);
    for (int d = 0; d < DIMENSIONS; d++) {
      const int expected = (d == dimension) ? 7 : 0;
      validateEqualsWithParams2(seed(d), expected, dimension, d);
    }
  }
}

/**
 * dIncrement(counter,max,doNotExamine) leaves the doNotExamine axis untouched
 * and walks the remaining axes over [0,max), terminating by pushing one free
 * axis to max.
 */
void peano4::tests::LoopTest::testDIncrementExclusive() {
  const int max = 3;
  for (int doNotExamine = 0; doNotExamine < DIMENSIONS; doNotExamine++) {
    auto          counter = peano4::dStartVector();
    std::set<int> visited;
    while (peano4::dCompareExclusive(counter, max, doNotExamine) != 0) {
      validateEqualsWithParams2(counter(doNotExamine), 0, doNotExamine, counter);
      const int linearised = peano4::dLinearised(counter, max);
      validateWithParams1(visited.find(linearised) == visited.end(), counter);
      visited.insert(linearised);
      peano4::dIncrement(counter, max, doNotExamine);
    }
    int expectedCount = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (d != doNotExamine) {
        expectedCount *= max;
      }
    }
    validateEqualsWithParams2(static_cast<int>(visited.size()), expectedCount, doNotExamine, max);
  }
}

/**
 * dLinearised(counter,max) maps a counter to sum_d counter(d)*max^d.
 */
void peano4::tests::LoopTest::testDLinearised() {
  const int max = 4;
  peano4::dForEach(max, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
    int manualIndex = 0;
    int factor      = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      manualIndex += k(d) * factor;
      factor *= max;
    }
    validateEqualsWithParams1(peano4::dLinearised(k, max), manualIndex, k);
  });
}

/**
 * dForEachExclusive(max, fixedDim, value, body) keeps the fixed dimension at
 * its start value and visits every combination of the remaining dimensions
 * exactly once.
 */
void peano4::tests::LoopTest::testDForEachExclusiveKeepsFixedDimensionAndIsComplete() {
  for (int fixedDimension = 0; fixedDimension < DIMENSIONS; fixedDimension++) {
    for (int max = 2; max <= 4; max++) {
      for (int value = 0; value < max; value++) {
        std::set<int> visited;
        peano4::dForEachExclusive(max, fixedDimension, value, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
          validateEqualsWithParams2(k(fixedDimension), value, k, value);
          const auto linearised = peano4::dLinearised(k, max);
          validateWithParams1(visited.find(linearised) == visited.end(), k);
          visited.insert(linearised);
        });
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
}

/**
 * dCompare returns true if every component is within range, for both the
 * scalar and the per-axis overload.
 */
void peano4::tests::LoopTest::testDCompare() {
  const int                                max = 3;
  const tarch::la::Vector<DIMENSIONS, int> inRange(0);
  validate(peano4::dCompare(inRange, max));

  tarch::la::Vector<DIMENSIONS, int> atUpperBound(0);
  atUpperBound(DIMENSIONS - 1) = max;
  validate(not peano4::dCompare(atUpperBound, max));

  tarch::la::Vector<DIMENSIONS, int> range;
  for (int d = 0; d < DIMENSIONS; d++) {
    range(d) = 2 + d;
  }
  const tarch::la::Vector<DIMENSIONS, int> insideBox(0);
  validate(peano4::dCompare(insideBox, range));
  tarch::la::Vector<DIMENSIONS, int> onBoxBoundary(0);
  onBoxBoundary(0) = range(0);
  validate(not peano4::dCompare(onBoxBoundary, range));
}

/**
 * dCompareExclusive returns true if every component except the ignored
 * dimension is within range, and it does not consult the ignored component.
 */
void peano4::tests::LoopTest::testDCompareExclusive() {
  const int max = 3;
  for (int ignoredDimension = 0; ignoredDimension < DIMENSIONS; ignoredDimension++) {
    // The ignored dimension is out of range but must not affect the result.
    tarch::la::Vector<DIMENSIONS, int> onlyIgnoredOutOfRange(0);
    onlyIgnoredOutOfRange(ignoredDimension) = max;
    validateWithParams1(peano4::dCompareExclusive(onlyIgnoredOutOfRange, max, ignoredDimension), onlyIgnoredOutOfRange);

    // Any examined dimension at max makes the comparison false.
    for (int d = 0; d < DIMENSIONS; d++) {
      if (d == ignoredDimension) {
        continue;
      }
      tarch::la::Vector<DIMENSIONS, int> examinedOutOfRange(0);
      examinedOutOfRange(d) = max;
      validateWithParams1(not peano4::dCompareExclusive(examinedOutOfRange, max, ignoredDimension), examinedOutOfRange);
    }
  }
}

/**
 * dForEachCorner enumerates the 2^d cube corners exactly once and its scalar
 * argument equals dLinearised(corner,2).
 */
void peano4::tests::LoopTest::testDForEachCornerEnumeratesCornersInLinearisedOrder() {
  std::set<int> visited;
  int           count = 0;
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, const int kScalar) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) == 0 or k(d) == 1, k);
    }
    validateEqualsWithParams1(kScalar, peano4::dLinearised(k, 2), k);
    visited.insert(kScalar);
    count++;
  });
  validateEquals(count, peano4::VerticesPerCell);
  validateEquals(static_cast<int>(visited.size()), peano4::VerticesPerCell);
}

/**
 * dForEachStencil enumerates the 3^d stencil exactly once in dLinearised order.
 */
void peano4::tests::LoopTest::testDForEachStencilEnumeratesStencil() {
  int expected = 0;
  peano4::dForEachStencil([&](const tarch::la::Vector<DIMENSIONS, int>& k, [[maybe_unused]] const int kScalar) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) >= 0 and k(d) < peano4::NeighbourhoodWidth, k);
    }
    validateEqualsWithParams1(peano4::dLinearised(k, peano4::NeighbourhoodWidth), expected, k);
    expected++;
  });
  validateEquals(expected, peano4::CellsPerNeighbourhood);
}
