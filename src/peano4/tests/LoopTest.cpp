// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoopTest.h"

#include "peano4/Loop.h"

peano4::tests::LoopTest::LoopTest() :
  TestCase("peano4::tests::LoopTest") {}

void peano4::tests::LoopTest::run() {
  testMethod([this]() { testDForEachVisitsAllPointsInLinearOrder(); });
  testMethod([this]() { testDForEachWithPerAxisRange(); });
  testMethod([this]() { testDStartVector(); });
  testMethod([this]() { testDIncrementOverloadConsistency(); });
  testMethod([this]() { testDIncrementExclusive(); });
  testMethod([this]() { testDLinearised(); });
  testMethod([this]() { testDForEachExclusiveKeepsFixedDimensionAndIsComplete(); });
  testMethod([this]() { testDCompare(); });
  testMethod([this]() { testDCompareExclusive(); });
  testMethod([this]() { testDForEachCornerEnumeratesCornersInLinearisedOrder(); });
  testMethod([this]() { testDForEachStencilEnumeratesStencil(); });
  testMethod([this]() { testZForEach3IsAHamiltonianPath(); });
  testMethod([this]() { testGenericZForEachIsAHamiltonianPath(); });
  testMethod([this]() { testGenericZForEachMatchesZForEach3(); });
}

/**
 * dForEach(max, body) has to visit every point of [0,max)^d exactly once, with
 * the 0th component running fastest, i.e. in dLinearised order.
 */
void peano4::tests::LoopTest::testDForEachVisitsAllPointsInLinearOrder() {
  for (int max = 1; max <= 5; max++) {
    int expected = 0;
    peano4::dForEach(max, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      // independent linearisation: component 0 runs fastest
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
  // distinct per-axis extents, and one degenerate axis of extent 1 in every
  // position, which is how a caller enumerates a face.
  for (int flatDimension = -1; flatDimension < DIMENSIONS; flatDimension++) {
    tarch::la::Vector<DIMENSIONS, int> range;
    long                               total = 1;
    for (int d = 0; d < DIMENSIONS; d++) {
      range(d) = (d == flatDimension) ? 1 : (2 + d);
      total *= range(d);
    }

    long count = 0;
    peano4::dForEach(range, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      // independent mixed-radix expectation, component 0 fastest
      long t = count;
      for (int d = 0; d < DIMENSIONS; d++) {
        validateEqualsWithParams2(k(d), static_cast<int>(t % range(d)), k, range);
        t /= range(d);
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
  // The scalar dIncrement(counter,int) and the per-axis dIncrement(counter,Vector) are the
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
    peano4::dIncrement(viaScalar, max);
    peano4::dIncrement(viaVector, uniformMax);
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

/**
 * dStartVector seeds a d-dimensional loop: the plain overload is the zero
 * vector, dStartVector(dim,value) is zero except for the given dimension, and
 * dStartVector(max,direction) seeds the oscillating z-loop with 0 where the
 * direction bit is set and max-1 where it is not.
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

  // The direction-seeded start: a set bit maps to 0, an unset bit to max-1.
  const int max = 4;
  for (int directionBits = 0; directionBits < TWO_POWER_D; directionBits++) {
    const peano4::LoopDirection direction(directionBits);
    const auto                  seed = peano4::dStartVector(max, direction);
    for (int d = 0; d < DIMENSIONS; d++) {
      const int expected = direction[d] ? 0 : (max - 1);
      validateEqualsWithParams2(seed(d), expected, directionBits, d);
    }
  }
}

/**
 * dIncrement(counter,max,doNotExamine) is the increment behind
 * dForEachExclusive: it leaves the doNotExamine axis untouched and walks the
 * remaining axes over [0,max), terminating by pushing one free axis to max.
 */
void peano4::tests::LoopTest::testDIncrementExclusive() {
  const int max = 3;
  for (int doNotExamine = 0; doNotExamine < DIMENSIONS; doNotExamine++) {
    auto          counter = peano4::dStartVector();
    std::set<int> visited;
    while (peano4::dCompareExclusive(counter, max, doNotExamine)) {
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
      // The pinned dimension must keep its start value for every value in
      // [0,max). A caller enumerating a face pins it to the side, so a
      // non-zero value has to work.
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
  const int                          max = 3;
  tarch::la::Vector<DIMENSIONS, int> inRange(0);
  validate(peano4::dCompare(inRange, max));

  tarch::la::Vector<DIMENSIONS, int> atUpperBound(0);
  atUpperBound(DIMENSIONS - 1) = max; // the loop terminal state
  validate(not peano4::dCompare(atUpperBound, max));

  // per-axis overload
  tarch::la::Vector<DIMENSIONS, int> range;
  for (int d = 0; d < DIMENSIONS; d++) {
    range(d) = 2 + d;
  }
  tarch::la::Vector<DIMENSIONS, int> insideBox(0);
  validate(peano4::dCompare(insideBox, range));
  tarch::la::Vector<DIMENSIONS, int> onBoxBoundary(0);
  onBoxBoundary(0) = range(0);
  validate(not peano4::dCompare(onBoxBoundary, range));
}

/**
 * dCompareExclusive returns true if every component except the ignored
 * dimension is within range; the ignored component must not be consulted. This
 * is the comparison behind the exclusive dForEachExclusive loop.
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
  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) == 0 or k(d) == 1, k);
    }
    validateEqualsWithParams1(kScalar, peano4::dLinearised(k, 2), k);
    visited.insert(kScalar);
    count++;
  });
  validateEquals(count, TWO_POWER_D);
  validateEquals(static_cast<int>(visited.size()), TWO_POWER_D);
}

/**
 * dForEachStencil enumerates the 3^d stencil exactly once in dLinearised order.
 */
void peano4::tests::LoopTest::testDForEachStencilEnumeratesStencil() {
  int expected = 0;
  peano4::dForEachStencil([&](const tarch::la::Vector<DIMENSIONS, int>& k, int) {
    for (int d = 0; d < DIMENSIONS; d++) {
      validateWithParams1(k(d) >= 0 and k(d) < 3, k);
    }
    validateEqualsWithParams1(peano4::dLinearised(k, 3), expected, k);
    expected++;
  });
  validateEquals(expected, THREE_POWER_D);
}

/**
 * zForEach3 is a space-filling (Peano) loop and therefore has to visit every
 * cell of [0,3)^d exactly once for every initial direction.
 */
void peano4::tests::LoopTest::testZForEach3IsAHamiltonianPath() {
  for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
    std::set<int>                      visited;
    int                                count = 0;
    tarch::la::Vector<DIMENSIONS, int> previous;
    peano4::zForEach3(peano4::LoopDirection(initialDirection), [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      for (int d = 0; d < DIMENSIONS; d++) {
        validateWithParams1(k(d) >= 0 and k(d) < 3, k);
      }
      // A space-filling path visits neighbours: consecutive cells differ by one
      // in exactly one coordinate.
      if (count > 0) {
        int changedAxes = 0;
        for (int d = 0; d < DIMENSIONS; d++) {
          const int delta = k(d) - previous(d);
          if (delta != 0) {
            changedAxes++;
            validateWithParams1(delta == 1 or delta == -1, k);
          }
        }
        validateEqualsWithParams2(changedAxes, 1, k, previous);
      }
      visited.insert(peano4::dLinearised(k, 3));
      previous = k;
      count++;
    });
    validateEqualsWithParams1(count, THREE_POWER_D, initialDirection);
    validateEqualsWithParams1(static_cast<int>(visited.size()), THREE_POWER_D, initialDirection);
  }
}

/**
 * The generic zForEach (and the dIncrement(...,LoopDirection&) behind it) has
 * to be a Hamiltonian path over [0,max)^d for every initial direction and
 * arbitrary max.
 */
void peano4::tests::LoopTest::testGenericZForEachIsAHamiltonianPath() {
  for (int max = 2; max <= 5; max++) {
    for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
      std::set<int>                      visited;
      int                                count = 0;
      tarch::la::Vector<DIMENSIONS, int> previous;
      peano4::zForEach(max, peano4::LoopDirection(initialDirection), [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
        for (int d = 0; d < DIMENSIONS; d++) {
          validateWithParams1(k(d) >= 0 and k(d) < max, k);
        }
        // Consecutive cells of the space-filling path are neighbours.
        if (count > 0) {
          int changedAxes = 0;
          for (int d = 0; d < DIMENSIONS; d++) {
            const int delta = k(d) - previous(d);
            if (delta != 0) {
              changedAxes++;
              validateWithParams1(delta == 1 or delta == -1, k);
            }
          }
          validateEqualsWithParams2(changedAxes, 1, k, previous);
        }
        visited.insert(peano4::dLinearised(k, max));
        previous = k;
        count++;
      });
      validateEqualsWithParams2(count, tarch::la::aPowI(DIMENSIONS, max), max, initialDirection);
      validateEqualsWithParams2(
        static_cast<int>(visited.size()),
        tarch::la::aPowI(DIMENSIONS, max),
        max,
        initialDirection
      );
    }
  }
}

/**
 * For max=3 the generic zForEach must reproduce the table-driven zForEach3
 * cell for cell.
 */
void peano4::tests::LoopTest::testGenericZForEachMatchesZForEach3() {
  const int max = 3;
  for (int initialDirection = 0; initialDirection < TWO_POWER_D; initialDirection++) {
    std::vector<int> genericSequence;
    peano4::zForEach(max, peano4::LoopDirection(initialDirection), [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      genericSequence.push_back(peano4::dLinearised(k, max));
    });

    std::vector<int> tableSequence;
    peano4::zForEach3(peano4::LoopDirection(initialDirection), [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      tableSequence.push_back(peano4::dLinearised(k, max));
    });

    validateEqualsWithParams1(
      static_cast<int>(genericSequence.size()),
      static_cast<int>(tableSequence.size()),
      initialDirection
    );
    for (int i = 0; i < static_cast<int>(tableSequence.size()); i++) {
      validateEqualsWithParams2(genericSequence[i], tableSequence[i], initialDirection, i);
    }
  }
}
