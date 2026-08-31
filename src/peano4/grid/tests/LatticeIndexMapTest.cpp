// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LatticeIndexMapTest.h"

#include "peano4/grid/LatticeIndexMap.h"

namespace {
  /**
   * Packs a lattice position the way the spacetree serialises one, so
   * neighbouring positions differ in their low bits alone.
   */
  std::int64_t latticeKey(const int firstAxisPosition, const int secondAxisPosition) {
    constexpr int BitsPerAxis = 21;
    return static_cast<std::int64_t>(firstAxisPosition)
      | (static_cast<std::int64_t>(secondAxisPosition) << BitsPerAxis);
  }
} // namespace

peano4::grid::tests::LatticeIndexMapTest::LatticeIndexMapTest() :
  TestCase("peano4::grid::tests::LatticeIndexMapTest") {}

void peano4::grid::tests::LatticeIndexMapTest::run() {
  testMethod(testEmptyMapHoldsNothing);
  testMethod(testInsertReportsWhetherItStored);
  testMethod(testEveryStoredKeyIsFound);
  testMethod(testClusteredKeysCollide);
  testMethod(testMapGrowsBeyondItsReservation);
  testMethod(testForEachEntryVisitsEveryEntryOnce);
  testMethod(testClearReleasesEveryEntry);
  testMethod(testCopyAndMoveCarryTheEntries);
  testMethod(testEraseReportsWhetherItRemoved);
  testMethod(testProbeChainsSurviveAnErase);
  testMethod(testErasedSlotsTakeEntriesAgain);
}

void peano4::grid::tests::LatticeIndexMapTest::testEmptyMapHoldsNothing() {
  const peano4::grid::LatticeIndexMap map;
  validateEquals(map.size(), 0);
  validateEquals(map.find(0), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(latticeKey(17, 4)), peano4::grid::LatticeIndexMap::NoEntry);

  peano4::grid::LatticeIndexMap reservedMap;
  reservedMap.reserve(64);
  validateEquals(reservedMap.size(), 0);
  validateEquals(reservedMap.find(latticeKey(17, 4)), peano4::grid::LatticeIndexMap::NoEntry);
}

void peano4::grid::tests::LatticeIndexMapTest::testInsertReportsWhetherItStored() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(16);

  const auto [firstValue, firstIsNew] = map.insert(latticeKey(3, 5), 11);
  validateEquals(firstValue, 11);
  validateTrue(firstIsNew);
  validateEquals(map.size(), 1);

  const auto [secondValue, secondIsNew] = map.insert(latticeKey(3, 5), 99);
  validateEquals(secondValue, 11);
  validateTrue(not secondIsNew);
  validateEquals(map.size(), 1);
  validateEquals(map.find(latticeKey(3, 5)), 11);
}

void peano4::grid::tests::LatticeIndexMapTest::testEveryStoredKeyIsFound() {
  constexpr int NumberOfPositionsPerAxis = 40;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);

  int value = 0;
  for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
    for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
      const auto [storedValue, isNew] = map.insert(latticeKey(firstAxisPosition, secondAxisPosition), value);
      validateEquals(storedValue, value);
      validateTrue(isNew);
      value++;
    }
  }
  validateEquals(static_cast<int>(map.size()), NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);

  value = 0;
  for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
    for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
      validateEquals(map.find(latticeKey(firstAxisPosition, secondAxisPosition)), value);
      value++;
    }
  }
  validateEquals(map.find(latticeKey(NumberOfPositionsPerAxis, 0)), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(latticeKey(0, NumberOfPositionsPerAxis)), peano4::grid::LatticeIndexMap::NoEntry);
}

void peano4::grid::tests::LatticeIndexMapTest::testClusteredKeysCollide() {
  // Consecutive positions along one axis land in consecutive keys.
  constexpr int NumberOfKeys = 200;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(latticeKey(position, 7), position);
  }
  validateEquals(static_cast<int>(map.size()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(map.find(latticeKey(position, 7)), position);
  }
  validateEquals(map.find(latticeKey(NumberOfKeys, 7)), peano4::grid::LatticeIndexMap::NoEntry);
}

void peano4::grid::tests::LatticeIndexMapTest::testMapGrowsBeyondItsReservation() {
  constexpr int NumberOfKeys = 500;

  peano4::grid::LatticeIndexMap map;
  map.reserve(4);
  for (int position = 0; position < NumberOfKeys; position++) {
    const auto [storedValue, isNew] = map.insert(latticeKey(position, position), 2 * position);
    validateEquals(storedValue, 2 * position);
    validateTrue(isNew);
  }
  validateEquals(static_cast<int>(map.size()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(map.find(latticeKey(position, position)), 2 * position);
  }

  peano4::grid::LatticeIndexMap unreservedMap;
  for (int position = 0; position < NumberOfKeys; position++) {
    unreservedMap.insert(latticeKey(position, 0), position);
  }
  validateEquals(static_cast<int>(unreservedMap.size()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(unreservedMap.find(latticeKey(position, 0)), position);
  }
}

void peano4::grid::tests::LatticeIndexMapTest::testForEachEntryVisitsEveryEntryOnce() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(latticeKey(position, 3), position);
  }

  std::vector<int> numberOfVisitsPerValue(NumberOfKeys, 0);
  int              numberOfVisits = 0;
  map.forEachEntry([&](const std::int64_t key, const int value) {
    validateEquals(key, latticeKey(value, 3));
    validateTrue(value >= 0 and value < NumberOfKeys);
    numberOfVisitsPerValue[value]++;
    numberOfVisits++;
  });
  validateEquals(numberOfVisits, NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(numberOfVisitsPerValue[position], 1);
  }

  const peano4::grid::LatticeIndexMap emptyMap;
  int                                 numberOfEmptyVisits = 0;
  emptyMap.forEachEntry([&numberOfEmptyVisits](const std::int64_t key, const int value) {
    static_cast<void>(key);
    static_cast<void>(value);
    numberOfEmptyVisits++;
  });
  validateEquals(numberOfEmptyVisits, 0);
}

void peano4::grid::tests::LatticeIndexMapTest::testClearReleasesEveryEntry() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(32);
  for (int position = 0; position < 20; position++) {
    map.insert(latticeKey(position, 1), position);
  }
  validateEquals(static_cast<int>(map.size()), 20);

  map.clear();
  validateEquals(map.size(), 0);
  validateEquals(map.find(latticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);

  map.insert(latticeKey(0, 1), 5);
  validateEquals(map.find(latticeKey(0, 1)), 5);
  validateEquals(map.size(), 1);

  // Reserving drops what the map held.
  map.reserve(8);
  validateEquals(map.size(), 0);
  validateEquals(map.find(latticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);
}

void peano4::grid::tests::LatticeIndexMapTest::testCopyAndMoveCarryTheEntries() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(32);
  for (int position = 0; position < 25; position++) {
    map.insert(latticeKey(position, 9), position);
  }

  const peano4::grid::LatticeIndexMap copiedMap(map);
  validateEquals(copiedMap.size(), map.size());
  for (int position = 0; position < 25; position++) {
    validateEquals(copiedMap.find(latticeKey(position, 9)), position);
  }

  peano4::grid::LatticeIndexMap assignedMap;
  assignedMap = copiedMap;
  validateEquals(assignedMap.size(), copiedMap.size());
  for (int position = 0; position < 25; position++) {
    validateEquals(assignedMap.find(latticeKey(position, 9)), position);
  }

  const peano4::grid::LatticeIndexMap movedMap(std::move(map));
  validateEquals(static_cast<int>(movedMap.size()), 25);
  for (int position = 0; position < 25; position++) {
    validateEquals(movedMap.find(latticeKey(position, 9)), position);
  }
}

void peano4::grid::tests::LatticeIndexMapTest::testEraseReportsWhetherItRemoved() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(16);
  map.insert(latticeKey(2, 6), 40);
  map.insert(latticeKey(3, 6), 41);

  validateTrue(map.erase(latticeKey(2, 6)));
  validateEquals(map.size(), 1);
  validateEquals(map.find(latticeKey(2, 6)), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(latticeKey(3, 6)), 41);

  validateTrue(not map.erase(latticeKey(2, 6)));
  validateTrue(not map.erase(latticeKey(9, 9)));
  validateEquals(map.size(), 1);

  peano4::grid::LatticeIndexMap emptyMap;
  validateTrue(not emptyMap.erase(latticeKey(0, 0)));
  validateEquals(emptyMap.size(), 0);
}

/**
 * An erase leaves a tombstone, so a probe run still reaches every entry behind
 * the erased slot.
 */
void peano4::grid::tests::LatticeIndexMapTest::testProbeChainsSurviveAnErase() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(latticeKey(position, 5), position);
  }

  for (int position = 0; position < NumberOfKeys; position += 2) {
    validateTrue(map.erase(latticeKey(position, 5)));
  }
  validateEquals(static_cast<int>(map.size()), NumberOfKeys / 2);

  for (int position = 0; position < NumberOfKeys; position++) {
    const auto expectedValue = position % 2 == 0 ? peano4::grid::LatticeIndexMap::NoEntry : position;
    validateEquals(map.find(latticeKey(position, 5)), expectedValue);
  }

  int numberOfVisits = 0;
  map.forEachEntry([&](const std::int64_t key, const int value) {
    validateEquals(key, latticeKey(value, 5));
    validateTrue(value % 2 == 1);
    numberOfVisits++;
  });
  validateEquals(numberOfVisits, NumberOfKeys / 2);
}

/**
 * An erased slot takes an entry again, and the growth that drops the tombstones
 * keeps every key.
 */
void peano4::grid::tests::LatticeIndexMapTest::testErasedSlotsTakeEntriesAgain() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(latticeKey(position, 2), position);
  }
  for (int position = 0; position < NumberOfKeys; position++) {
    validateTrue(map.erase(latticeKey(position, 2)));
  }
  validateEquals(map.size(), 0);

  for (int position = 0; position < NumberOfKeys; position++) {
    const auto [storedValue, isNew] = map.insert(latticeKey(position, 2), 1000 + position);
    validateEquals(storedValue, 1000 + position);
    validateTrue(isNew);
  }
  validateEquals(static_cast<int>(map.size()), NumberOfKeys);

  for (int position = NumberOfKeys; position < 4 * NumberOfKeys; position++) {
    map.insert(latticeKey(position, 2), 1000 + position);
  }
  validateEquals(static_cast<int>(map.size()), 4 * NumberOfKeys);
  for (int position = 0; position < 4 * NumberOfKeys; position++) {
    validateEquals(map.find(latticeKey(position, 2)), 1000 + position);
  }
}
