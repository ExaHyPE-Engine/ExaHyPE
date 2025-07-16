// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LinearisedTest.h"

#include "peano4/utils/Loop.h"

peano4::utils::tests::LinearisedTest::LinearisedTest():
  TestCase("peano4::utils::tests::LinearisedTest") {}

void peano4::utils::tests::LinearisedTest::run() {
  testMethod([this]() { testLinearisedBitset(); });
  testMethod([this]() { testLinearisedVectorWithoutLookup(); });
  testMethod([this]() { testLinearisedVectorRoundTripWithoutLookup(); });
  testMethod([this]() { testDelinearisedWithoutLookup(); });
  testMethod([this]() { testKeyComputationForLinearisedLookup(); });
  testMethod([this]() { testLookupTableLinearised(); });
  testMethod([this]() { testLookupTableDelinearised(); });
  testMethod([this]() { testLinearisedVector(); });
  testMethod([this]() { testDelinearisedVector(); });
}

void peano4::utils::tests::LinearisedTest::testLinearisedBitset() {
  constexpr int TotalCombinations = 1 << Dimensions;

  for (int i = 0; i < TotalCombinations; i++) {
    std::bitset<Dimensions> bits(i);
    int                     result = dLinearised(bits);

    validateEqualsWithParams1(result, i, bits);
  }
}

void peano4::utils::tests::LinearisedTest::testLinearisedVectorWithoutLookup() {
  constexpr int                      Max = 3;
  tarch::la::Vector<Dimensions, int> counter(0);

  std::vector<int> limits(Dimensions, Max);

  bool done = false;
  while (!done) {
    int manualIndex = 0;
    int factor      = 1;
    for (int d = 0; d < Dimensions; d++) {
      manualIndex += counter(d) * factor;
      factor *= Max;
    }

    int actual = dLinearisedWithoutLookup(counter, Max);
    validateEqualsWithParams1(actual, manualIndex, counter);

    for (int d = 0; d < Dimensions; d++) {
      counter(d)++;
      if (counter(d) < Max)
        break;
      counter(d) = 0;
      if (d == Dimensions - 1)
        done = true;
    }
  }
}

void peano4::utils::tests::LinearisedTest::testLinearisedVectorRoundTripWithoutLookup() {
  constexpr int                      Max = 3;
  tarch::la::Vector<Dimensions, int> counter(0);

  bool done = false;
  while (!done) {
    int                                lin      = dLinearisedWithoutLookup(counter, Max);
    tarch::la::Vector<Dimensions, int> restored = dDelinearisedWithoutLookup(lin, Max);
    for (int d = 0; d < Dimensions; d++) {
      validateEqualsWithParams1(restored(d), counter(d), counter);
    }

    for (int d = 0; d < Dimensions; d++) {
      counter(d)++;
      if (counter(d) < Max)
        break;
      counter(d) = 0;
      if (d == Dimensions - 1)
        done = true;
    }
  }
}

void peano4::utils::tests::LinearisedTest::testDelinearisedWithoutLookup() {
  constexpr int Max   = 3;
  constexpr int Total = BasePowerExponent<Max, Dimensions>::value;

  for (int value = 0; value < Total; value++) {
    tarch::la::Vector<Dimensions, int> coords     = dDelinearisedWithoutLookup(value, Max);
    int                                recomputed = dLinearisedWithoutLookup(coords, Max);

    validateEqualsWithParams1(recomputed, value, coords);
  }
}

void peano4::utils::tests::LinearisedTest::testKeyComputationForLinearisedLookup() {
  std::set<int> usedKeys;

  for (int max = 2; max <= MaxTable; max++) {
    tarch::la::Vector<Dimensions, int> counter(0);
    bool                               done = false;

    while (!done) {
      int key = getKeyForDLinearisedLookup(counter, max);

      validateWithParams1(key < MaxIndexOfLinearisation * (MaxTable - 1), key);
      validateWithParams1(usedKeys.find(key) == usedKeys.end(), key);
      usedKeys.insert(key);

      for (int d = 0; d < Dimensions; d++) {
        counter(d)++;
        if (counter(d) < max)
          break;
        counter(d) = 0;
        if (d == Dimensions - 1)
          done = true;
      }
    }
  }
}

void peano4::utils::tests::LinearisedTest::testLookupTableLinearised() {
  for (int max = 2; max <= MaxTable; max++) {
    tarch::la::Vector<Dimensions, int> counter(0);
    bool                               done = false;
    while (!done) {
      bool valid = true;
      for (int d = 0; d < Dimensions; d++) {
        if (counter(d) >= max) {
          valid = false;
          break;
        }
      }

      if (valid) {
        int expected = dLinearisedWithoutLookup(counter, max);
        int key      = getKeyForDLinearisedLookup(counter, max);
        int actual   = lookupTableDLinearised[key];
        validateEqualsWithParams3(actual, expected, counter, max, key);
      }

      for (int d = 0; d < Dimensions; d++) {
        counter(d)++;
        if (counter(d) < MaxTable /*max*/)
          break;
        counter(d) = 0;
        if (d == Dimensions - 1)
          done = true;
      }
    }
  }
}

void peano4::utils::tests::LinearisedTest::testLookupTableDelinearised() {
  for (int max = 2; max <= MaxTable; max++) {
    for (int value = 0; value < tarch::la::aPowI(Dimensions, max); value++) {
      int                                key      = getKeyForDDeLinearisedLookup(value, max);
      tarch::la::Vector<Dimensions, int> expected = dDelinearisedWithoutLookup(value, max);
      tarch::la::Vector<Dimensions, int> actual   = lookupTableDDeLinearised[key];
      for (int d = 0; d < Dimensions; d++) {
        validateEqualsWithParams1(actual(d), expected(d), value);
      }
    }
  }
}

void peano4::utils::tests::LinearisedTest::testLinearisedVector() {
  constexpr int                      Max = MaxTable * 2;
  tarch::la::Vector<Dimensions, int> counter(0);

  std::vector<int> limits(Dimensions, Max);

  bool done = false;
  while (!done) {
    int manualIndex = 0;
    int factor      = 1;
    for (int d = 0; d < Dimensions; d++) {
      manualIndex += counter(d) * factor;
      factor *= Max;
    }

    int actual = dLinearised(counter, Max);
    validateEqualsWithParams1(actual, manualIndex, counter);

    for (int d = 0; d < Dimensions; d++) {
      counter(d)++;
      if (counter(d) < Max)
        break;
      counter(d) = 0;
      if (d == Dimensions - 1)
        done = true;
    }
  }
}

void peano4::utils::tests::LinearisedTest::testDelinearisedVector() {
  constexpr int Max = MaxTable * 2;
  for (int index = 0; index < tarch::la::aPowI(Dimensions, Max); ++index) {
    tarch::la::Vector<Dimensions, int> result = dDelinearisedWithoutLookup(index, Max);

    int                                value = index;
    tarch::la::Vector<Dimensions, int> expected(0);
    for (int d = 0; d < Dimensions; ++d) {
      expected(d) = value % Max;
      value /= Max;
    }

    for (int d = 0; d < Dimensions; ++d) {
      validateEqualsWithParams1(result(d), expected(d), index);
    }
  }
}
