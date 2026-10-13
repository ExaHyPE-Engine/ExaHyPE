// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LatticeIndexMapTest.h"

#include "peano4/grid/LatticeIndexMap.h"

namespace {
  /**
   * Packs a lattice position like Tree::serialiseLatticeKey(). Neighbouring positions differ in
   * their low bits only.
   */
  std::int64_t getLatticeKey(const int firstAxisPosition, const int secondAxisPosition) {
    constexpr int BitsPerAxis = 21;
    return static_cast<std::int64_t>(firstAxisPosition)
      | (static_cast<std::int64_t>(secondAxisPosition) << BitsPerAxis);
  }
} // namespace

peano4::grid::tests::LatticeIndexMapTest::LatticeIndexMapTest() :
  TestCase("peano4::grid::tests::LatticeIndexMapTest") {}

void
peano4::grid::tests::LatticeIndexMapTest::run() {
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
  testMethod(testBytesCountTheKeyAndValueArrays);
}

/**
 * The allocated bytes are zero for an empty map and the key and value arrays of the capacity after
 * reserve() and clear().
 */
void
peano4::grid::tests::LatticeIndexMapTest::testBytesCountTheKeyAndValueArrays() {
  peano4::grid::LatticeIndexMap map;
  validateEquals(map.getAllocatedBytes(), std::size_t{0});

  map.reserve(5);
  const std::size_t capacity = 16;
  validateEquals(map.getAllocatedBytes(), capacity * (sizeof(std::int64_t) + sizeof(int)));

  map.clear();
  validateEquals(map.getAllocatedBytes(), capacity * (sizeof(std::int64_t) + sizeof(int)));
}

void
peano4::grid::tests::LatticeIndexMapTest::testEmptyMapHoldsNothing() {
  const peano4::grid::LatticeIndexMap map;
  validateEquals(map.getSize(), 0);
  validateEquals(map.find(0), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(getLatticeKey(17, 4)), peano4::grid::LatticeIndexMap::NoEntry);

  peano4::grid::LatticeIndexMap reservedMap;
  reservedMap.reserve(64);
  validateEquals(reservedMap.getSize(), 0);
  validateEquals(reservedMap.find(getLatticeKey(17, 4)), peano4::grid::LatticeIndexMap::NoEntry);
}

void
peano4::grid::tests::LatticeIndexMapTest::testInsertReportsWhetherItStored() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(16);

  const auto [firstValue, firstIsNew] = map.insert(getLatticeKey(3, 5), 11);
  validateEquals(firstValue, 11);
  validateTrue(firstIsNew);
  validateEquals(map.getSize(), 1);

  const auto [secondValue, secondIsNew] = map.insert(getLatticeKey(3, 5), 99);
  validateEquals(secondValue, 11);
  validateTrue(not secondIsNew);
  validateEquals(map.getSize(), 1);
  validateEquals(map.find(getLatticeKey(3, 5)), 11);
}

void
peano4::grid::tests::LatticeIndexMapTest::testEveryStoredKeyIsFound() {
  constexpr int NumberOfPositionsPerAxis = 40;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);

  int value = 0;
  for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
    for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
      const auto [storedValue, isNew] = map.insert(getLatticeKey(firstAxisPosition, secondAxisPosition), value);
      validateEquals(storedValue, value);
      validateTrue(isNew);
      value++;
    }
  }
  validateEquals(static_cast<int>(map.getSize()), NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);

  value = 0;
  for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
    for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
      validateEquals(map.find(getLatticeKey(firstAxisPosition, secondAxisPosition)), value);
      value++;
    }
  }
  validateEquals(map.find(getLatticeKey(NumberOfPositionsPerAxis, 0)), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(getLatticeKey(0, NumberOfPositionsPerAxis)), peano4::grid::LatticeIndexMap::NoEntry);
}

void
peano4::grid::tests::LatticeIndexMapTest::testClusteredKeysCollide() {
  // Consecutive positions along one axis land in consecutive keys.
  constexpr int NumberOfKeys = 200;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(getLatticeKey(position, 7), position);
  }
  validateEquals(static_cast<int>(map.getSize()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(map.find(getLatticeKey(position, 7)), position);
  }
  validateEquals(map.find(getLatticeKey(NumberOfKeys, 7)), peano4::grid::LatticeIndexMap::NoEntry);
}

void
peano4::grid::tests::LatticeIndexMapTest::testMapGrowsBeyondItsReservation() {
  constexpr int NumberOfKeys = 500;

  peano4::grid::LatticeIndexMap map;
  map.reserve(4);
  for (int position = 0; position < NumberOfKeys; position++) {
    const auto [storedValue, isNew] = map.insert(getLatticeKey(position, position), 2 * position);
    validateEquals(storedValue, 2 * position);
    validateTrue(isNew);
  }
  validateEquals(static_cast<int>(map.getSize()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(map.find(getLatticeKey(position, position)), 2 * position);
  }

  peano4::grid::LatticeIndexMap unreservedMap;
  for (int position = 0; position < NumberOfKeys; position++) {
    unreservedMap.insert(getLatticeKey(position, 0), position);
  }
  validateEquals(static_cast<int>(unreservedMap.getSize()), NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    validateEquals(unreservedMap.find(getLatticeKey(position, 0)), position);
  }
}

void
peano4::grid::tests::LatticeIndexMapTest::testForEachEntryVisitsEveryEntryOnce() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(getLatticeKey(position, 3), position);
  }

  std::vector<int> numberOfVisitsPerValue(NumberOfKeys, 0);
  int              numberOfVisits = 0;
  map.forEachEntry([&](const std::int64_t key, const int value) {
    validateEquals(key, getLatticeKey(value, 3));
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

void
peano4::grid::tests::LatticeIndexMapTest::testClearReleasesEveryEntry() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(32);
  for (int position = 0; position < 20; position++) {
    map.insert(getLatticeKey(position, 1), position);
  }
  validateEquals(static_cast<int>(map.getSize()), 20);

  map.clear();
  validateEquals(map.getSize(), 0);
  validateEquals(map.find(getLatticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);

  map.insert(getLatticeKey(0, 1), 5);
  validateEquals(map.find(getLatticeKey(0, 1)), 5);
  validateEquals(map.getSize(), 1);

  map.reserve(8);
  validateEquals(map.getSize(), 0);
  validateEquals(map.find(getLatticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);
}

void
peano4::grid::tests::LatticeIndexMapTest::testCopyAndMoveCarryTheEntries() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(32);
  for (int position = 0; position < 25; position++) {
    map.insert(getLatticeKey(position, 9), position);
  }

  const peano4::grid::LatticeIndexMap copiedMap(map);
  validateEquals(copiedMap.getSize(), map.getSize());
  for (int position = 0; position < 25; position++) {
    validateEquals(copiedMap.find(getLatticeKey(position, 9)), position);
  }

  peano4::grid::LatticeIndexMap assignedMap;
  assignedMap = copiedMap;
  validateEquals(assignedMap.getSize(), copiedMap.getSize());
  for (int position = 0; position < 25; position++) {
    validateEquals(assignedMap.find(getLatticeKey(position, 9)), position);
  }

  const peano4::grid::LatticeIndexMap movedMap(std::move(map));
  validateEquals(static_cast<int>(movedMap.getSize()), 25);
  for (int position = 0; position < 25; position++) {
    validateEquals(movedMap.find(getLatticeKey(position, 9)), position);
  }
}

void
peano4::grid::tests::LatticeIndexMapTest::testEraseReportsWhetherItRemoved() {
  peano4::grid::LatticeIndexMap map;
  map.reserve(16);
  map.insert(getLatticeKey(2, 6), 40);
  map.insert(getLatticeKey(3, 6), 41);

  validateTrue(map.erase(getLatticeKey(2, 6)));
  validateEquals(map.getSize(), 1);
  validateEquals(map.find(getLatticeKey(2, 6)), peano4::grid::LatticeIndexMap::NoEntry);
  validateEquals(map.find(getLatticeKey(3, 6)), 41);

  validateTrue(not map.erase(getLatticeKey(2, 6)));
  validateTrue(not map.erase(getLatticeKey(9, 9)));
  validateEquals(map.getSize(), 1);

  peano4::grid::LatticeIndexMap emptyMap;
  validateTrue(not emptyMap.erase(getLatticeKey(0, 0)));
  validateEquals(emptyMap.getSize(), 0);
}

/**
 * A probe still reaches every entry behind an erased slot.
 */
void
peano4::grid::tests::LatticeIndexMapTest::testProbeChainsSurviveAnErase() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(getLatticeKey(position, 5), position);
  }

  for (int position = 0; position < NumberOfKeys; position += 2) {
    validateTrue(map.erase(getLatticeKey(position, 5)));
  }
  validateEquals(static_cast<int>(map.getSize()), NumberOfKeys / 2);

  for (int position = 0; position < NumberOfKeys; position++) {
    const auto expectedValue = position % 2 == 0 ? peano4::grid::LatticeIndexMap::NoEntry : position;
    validateEquals(map.find(getLatticeKey(position, 5)), expectedValue);
  }

  int numberOfVisits = 0;
  map.forEachEntry([&](const std::int64_t key, const int value) {
    validateEquals(key, getLatticeKey(value, 5));
    validateTrue(value % 2 == 1);
    numberOfVisits++;
  });
  validateEquals(numberOfVisits, NumberOfKeys / 2);
}

/**
 * An erased slot takes an entry again. The growth that drops the tombstones keeps every key.
 */
void
peano4::grid::tests::LatticeIndexMapTest::testErasedSlotsTakeEntriesAgain() {
  constexpr int NumberOfKeys = 64;

  peano4::grid::LatticeIndexMap map;
  map.reserve(NumberOfKeys);
  for (int position = 0; position < NumberOfKeys; position++) {
    map.insert(getLatticeKey(position, 2), position);
  }
  for (int position = 0; position < NumberOfKeys; position++) {
    validateTrue(map.erase(getLatticeKey(position, 2)));
  }
  validateEquals(map.getSize(), 0);

  for (int position = 0; position < NumberOfKeys; position++) {
    const auto [storedValue, isNew] = map.insert(getLatticeKey(position, 2), 1000 + position);
    validateEquals(storedValue, 1000 + position);
    validateTrue(isNew);
  }
  validateEquals(static_cast<int>(map.getSize()), NumberOfKeys);

  for (int position = NumberOfKeys; position < 4 * NumberOfKeys; position++) {
    map.insert(getLatticeKey(position, 2), 1000 + position);
  }
  validateEquals(static_cast<int>(map.getSize()), 4 * NumberOfKeys);
  for (int position = 0; position < 4 * NumberOfKeys; position++) {
    validateEquals(map.find(getLatticeKey(position, 2)), 1000 + position);
  }
}
