// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LoopTest.h"

#include "peano4/Constants.h"
#include "peano4/Loop.h"
#include "tarch/la/Vector.h"

peano4::tests::LoopTest::LoopTest() :
  TestCase("peano4::tests::LoopTest") {}

void
peano4::tests::LoopTest::run() {
  testMethod(testDForEachVisitsAllPointsInLinearOrder);
  testMethod(testDForEachWithPerAxisRange);
  testMethod(testDLinearised);
  testMethod(testDDelinearisedInvertsDLinearised);
  testMethod(testDDelinearisedExclusiveKeepsFixedDimensionAtZero);
  testMethod(testDLinearisedExclusiveInvertsDDelinearisedExclusive);
  testMethod(testDForEachCornerEnumeratesCornersInLinearisedOrder);
}

/**
 * dForEach(max, body) visits every point of [0,max)^d exactly once in
 * dLinearised order, with the 0th component running fastest.
 */
void
peano4::tests::LoopTest::testDForEachVisitsAllPointsInLinearOrder() {
  for (int max = 0; max <= 5; max++) {
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
    validateEquals(expected, peano4::power(max, DIMENSIONS));
  }
}

/**
 * dForEach(maxVector, body) with individual per-axis ranges, including a
 * degenerate axis of extent one, has to enumerate the full box exactly once.
 */
void
peano4::tests::LoopTest::testDForEachWithPerAxisRange() {
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
 * dLinearised(counter,max) maps a counter to sum_d counter(d)*max^d.
 */
void
peano4::tests::LoopTest::testDLinearised() {
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
 * dDelinearised(index,max) returns the counter that dLinearised maps onto index.
 */
void
peano4::tests::LoopTest::testDDelinearisedInvertsDLinearised() {
  for (int max = 1; max <= 4; max++) {
    peano4::dForEach(max, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {
      const auto restored = peano4::dDelinearised(peano4::dLinearised(k, max), max);
      validateEqualsWithParams2(restored, k, max, peano4::dLinearised(k, max));
    });
  }
}

/**
 * dDelinearisedExclusive(index,max,dim) enumerates the points of a face: Entry dim stays zero and
 * every other entry runs over [0,max) once.
 */
void
peano4::tests::LoopTest::testDDelinearisedExclusiveKeepsFixedDimensionAtZero() {
  constexpr int Max = 3;
  for (int dim = 0; dim < DIMENSIONS; dim++) {
    int pointsPerFace = 1;
    for (int d = 0; d < DIMENSIONS - 1; d++) {
      pointsPerFace *= Max;
    }
    std::set<std::vector<int>> visited;
    for (int index = 0; index < pointsPerFace; index++) {
      const auto counter = peano4::dDelinearisedExclusive(index, Max, dim);
      validateEqualsWithParams2(counter(dim), 0, index, dim);
      std::vector<int> entries;
      for (int d = 0; d < DIMENSIONS; d++) {
        validate(counter(d) >= 0 and counter(d) < Max);
        entries.push_back(counter(d));
      }
      visited.insert(entries);
    }
    validateEqualsWithParams2(static_cast<int>(visited.size()), pointsPerFace, dim, Max);
  }
}

/**
 * dLinearisedExclusive(counter,max,dim) returns the index that dDelinearisedExclusive maps onto
 * counter, whatever entry dim holds.
 */
void
peano4::tests::LoopTest::testDLinearisedExclusiveInvertsDDelinearisedExclusive() {
  constexpr int Max = 3;
  for (int dim = 0; dim < DIMENSIONS; dim++) {
    int pointsPerFace = 1;
    for (int d = 0; d < DIMENSIONS - 1; d++) {
      pointsPerFace *= Max;
    }
    for (int index = 0; index < pointsPerFace; index++) {
      auto counter = peano4::dDelinearisedExclusive(index, Max, dim);
      validateEqualsWithParams2(peano4::dLinearisedExclusive(counter, Max, dim), index, dim, Max);
      counter(dim) = Max - 1;
      validateEqualsWithParams2(peano4::dLinearisedExclusive(counter, Max, dim), index, dim, Max);
    }
  }
}

/**
 * dForEachCorner enumerates the 2^d cube corners exactly once and its scalar
 * argument equals dLinearised(corner,2).
 */
void
peano4::tests::LoopTest::testDForEachCornerEnumeratesCornersInLinearisedOrder() {
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
