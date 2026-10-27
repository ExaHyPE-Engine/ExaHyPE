// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/grid/LatticeIndexMap.h"

#include "tarch/testing/UnitTest.h"

namespace {
  /**
   * Packs a lattice position like Tree::serialiseLatticeKey(). Neighbouring positions differ in their
   * low bits only.
   */
  std::int64_t getLatticeKey(const int firstAxisPosition, const int secondAxisPosition) {
    constexpr int BitsPerAxis = 21;
    return static_cast<std::int64_t>(firstAxisPosition)
      | (static_cast<std::int64_t>(secondAxisPosition) << BitsPerAxis);
  }
} // namespace

UNIT_TEST("An empty map contains zero entries, also after a reservation") {
  GIVEN("An empty map and the lattice key of the position 17, 4") {
    peano4::grid::LatticeIndexMap map;
    const std::int64_t            key = getLatticeKey(17, 4);

    THEN("The map stores zero entries and returns NoEntry for the key zero and the lattice key") {
      VALIDATE_EQUALS(map.getSize(), 0);
      VALIDATE_EQUALS(map.find(0), peano4::grid::LatticeIndexMap::NoEntry);
      VALIDATE_EQUALS(map.find(key), peano4::grid::LatticeIndexMap::NoEntry);
    }

    WHEN("The map reserves 64 entries") {
      map.reserve(64);

      THEN("The map stores zero entries and does not find the lattice key") {
        VALIDATE_EQUALS(map.getSize(), 0);
        VALIDATE_EQUALS(map.find(key), peano4::grid::LatticeIndexMap::NoEntry);
      }
    }
  }
}

UNIT_TEST("An insert reports whether it stored its value and a second insert of a key keeps the first value") {
  GIVEN("A map reserved for 16 entries and the lattice key of the position 3, 5") {
    peano4::grid::LatticeIndexMap map;
    map.reserve(16);
    const std::int64_t key = getLatticeKey(3, 5);

    WHEN("The test inserts the key with the value 11") {
      const auto [firstValue, firstIsNew] = map.insert(key, 11);

      THEN("The insert stores the value 11 as a new entry") {
        VALIDATE_EQUALS(firstValue, 11);
        VALIDATE(firstIsNew);
        VALIDATE_EQUALS(map.getSize(), 1);
      }

      AND_WHEN("The test inserts the key again with the value 99") {
        const auto [secondValue, secondIsNew] = map.insert(key, 99);

        THEN("The second insert returns the stored value 11 and keeps the size one") {
          VALIDATE_EQUALS(secondValue, 11);
          VALIDATE(not secondIsNew);
          VALIDATE_EQUALS(map.getSize(), 1);
          VALIDATE_EQUALS(map.find(key), 11);
        }
      }
    }
  }
}

UNIT_TEST("A map finds exactly the stored keys with their values") {
  GIVEN("A map reserved for the 40 by 40 positions of two axes") {
    constexpr int NumberOfPositionsPerAxis = 40;

    peano4::grid::LatticeIndexMap map;
    map.reserve(NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);

    WHEN("The test inserts the keys of all positions with ascending values") {
      std::vector<std::pair<int, bool>> insertResults;
      int                               value = 0;
      for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
        for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
          insertResults.push_back(map.insert(getLatticeKey(firstAxisPosition, secondAxisPosition), value));
          value++;
        }
      }

      THEN("Every insert stores its value as a new entry and the map stores one entry per position") {
        for (std::size_t insertNumber = 0; insertNumber < insertResults.size(); insertNumber++) {
          const auto [storedValue, isNew] = insertResults[insertNumber];
          VALIDATE_EQUALS(storedValue, static_cast<int>(insertNumber));
          VALIDATE(isNew);
        }
        VALIDATE_EQUALS(static_cast<int>(map.getSize()), NumberOfPositionsPerAxis * NumberOfPositionsPerAxis);
      }

      THEN("The map finds every key with its value and not the keys one position beyond either axis") {
        int expectedValue = 0;
        for (int firstAxisPosition = 0; firstAxisPosition < NumberOfPositionsPerAxis; firstAxisPosition++) {
          for (int secondAxisPosition = 0; secondAxisPosition < NumberOfPositionsPerAxis; secondAxisPosition++) {
            VALIDATE_EQUALS(map.find(getLatticeKey(firstAxisPosition, secondAxisPosition)), expectedValue);
            expectedValue++;
          }
        }
        VALIDATE_EQUALS(map.find(getLatticeKey(NumberOfPositionsPerAxis, 0)), peano4::grid::LatticeIndexMap::NoEntry);
        VALIDATE_EQUALS(map.find(getLatticeKey(0, NumberOfPositionsPerAxis)), peano4::grid::LatticeIndexMap::NoEntry);
      }
    }
  }
}

UNIT_TEST("A map finds the clustered keys of consecutive positions along one axis") {
  GIVEN("A map reserved for 200 keys") {
    constexpr int NumberOfKeys = 200;

    peano4::grid::LatticeIndexMap map;
    map.reserve(NumberOfKeys);

    WHEN("The test inserts the consecutive keys of 200 consecutive positions along the first axis") {
      for (int position = 0; position < NumberOfKeys; position++) {
        map.insert(getLatticeKey(position, 7), position);
      }

      THEN("The map finds every key with its position and not the key of the next position") {
        VALIDATE_EQUALS(static_cast<int>(map.getSize()), NumberOfKeys);
        for (int position = 0; position < NumberOfKeys; position++) {
          VALIDATE_EQUALS(map.find(getLatticeKey(position, 7)), position);
        }
        VALIDATE_EQUALS(map.find(getLatticeKey(NumberOfKeys, 7)), peano4::grid::LatticeIndexMap::NoEntry);
      }
    }
  }
}

UNIT_TEST("A map grows beyond its reservation and keeps every key") {
  GIVEN("A map reserved for four entries and the number of keys 500") {
    constexpr int NumberOfKeys = 500;

    peano4::grid::LatticeIndexMap map;
    map.reserve(4);

    WHEN("The test inserts the keys of 500 positions on the diagonal with twice the position as value") {
      std::vector<std::pair<int, bool>> insertResults;
      for (int position = 0; position < NumberOfKeys; position++) {
        insertResults.push_back(map.insert(getLatticeKey(position, position), 2 * position));
      }

      THEN("Every insert stores a new entry and the map finds every key with its value") {
        for (int position = 0; position < NumberOfKeys; position++) {
          const auto [storedValue, isNew] = insertResults[static_cast<std::size_t>(position)];
          VALIDATE_EQUALS(storedValue, 2 * position);
          VALIDATE(isNew);
        }
        VALIDATE_EQUALS(static_cast<int>(map.getSize()), NumberOfKeys);
        for (int position = 0; position < NumberOfKeys; position++) {
          VALIDATE_EQUALS(map.find(getLatticeKey(position, position)), 2 * position);
        }
      }
    }
  }

  GIVEN("A map without a reservation and the number of keys 500") {
    constexpr int NumberOfKeys = 500;

    peano4::grid::LatticeIndexMap unreservedMap;

    WHEN("The test inserts the keys of 500 positions along the first axis with the position as value") {
      for (int position = 0; position < NumberOfKeys; position++) {
        unreservedMap.insert(getLatticeKey(position, 0), position);
      }

      THEN("The map finds every key with its value") {
        VALIDATE_EQUALS(static_cast<int>(unreservedMap.getSize()), NumberOfKeys);
        for (int position = 0; position < NumberOfKeys; position++) {
          VALIDATE_EQUALS(unreservedMap.find(getLatticeKey(position, 0)), position);
        }
      }
    }
  }
}

UNIT_TEST("A visit of the entries of a map reaches every entry once with its key and its value") {
  GIVEN("A map of 64 keys along the first axis, each with its position as value") {
    constexpr int NumberOfKeys = 64;

    peano4::grid::LatticeIndexMap map;
    map.reserve(NumberOfKeys);
    for (int position = 0; position < NumberOfKeys; position++) {
      map.insert(getLatticeKey(position, 3), position);
    }

    WHEN("The map visits its entries") {
      std::vector<std::pair<std::int64_t, int>> visitedEntries;
      map.forEachEntry([&visitedEntries](const std::int64_t key, const int value) {
        visitedEntries.emplace_back(key, value);
      });

      THEN("Every visit passes a stored key with its value and every value receives one visit") {
        std::vector<int> numberOfVisitsPerValue(NumberOfKeys, 0);
        for (const auto& [key, value] : visitedEntries) {
          VALIDATE_EQUALS(key, getLatticeKey(value, 3));
          VALIDATE(value >= 0 and value < NumberOfKeys);
          numberOfVisitsPerValue[value]++;
        }
        VALIDATE_EQUALS(static_cast<int>(visitedEntries.size()), NumberOfKeys);
        for (int position = 0; position < NumberOfKeys; position++) {
          VALIDATE_EQUALS(numberOfVisitsPerValue[position], 1);
        }
      }
    }
  }

  GIVEN("An empty map") {
    const peano4::grid::LatticeIndexMap emptyMap;

    WHEN("The map visits its entries") {
      int numberOfEmptyVisits = 0;
      emptyMap.forEachEntry(
        [&numberOfEmptyVisits]([[maybe_unused]] const std::int64_t key, [[maybe_unused]] const int value) {
          numberOfEmptyVisits++;
        }
      );

      THEN("The map makes zero visits") { VALIDATE_EQUALS(numberOfEmptyVisits, 0); }
    }
  }
}

UNIT_TEST("A clear and a reservation drop every entry of a map and a cleared map takes entries again") {
  GIVEN("A map reserved for 32 entries that stores 20 keys") {
    peano4::grid::LatticeIndexMap map;
    map.reserve(32);
    for (int position = 0; position < 20; position++) {
      map.insert(getLatticeKey(position, 1), position);
    }

    THEN("The map stores 20 entries") { VALIDATE_EQUALS(static_cast<int>(map.getSize()), 20); }

    WHEN("The map clears") {
      map.clear();

      THEN("The map stores zero entries and does not find the first key") {
        VALIDATE_EQUALS(map.getSize(), 0);
        VALIDATE_EQUALS(map.find(getLatticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);
      }

      AND_WHEN("The test inserts the first key with the value five") {
        map.insert(getLatticeKey(0, 1), 5);

        THEN("The map finds the key with the value five and stores one entry") {
          VALIDATE_EQUALS(map.find(getLatticeKey(0, 1)), 5);
          VALIDATE_EQUALS(map.getSize(), 1);
        }

        AND_WHEN("The map reserves eight entries") {
          map.reserve(8);

          THEN("The reservation drops the entry the map stored") {
            VALIDATE_EQUALS(map.getSize(), 0);
            VALIDATE_EQUALS(map.find(getLatticeKey(0, 1)), peano4::grid::LatticeIndexMap::NoEntry);
          }
        }
      }
    }
  }
}

UNIT_TEST("A copy, a copy assignment and a move of a map keep every entry") {
  GIVEN("A map reserved for 32 entries that stores 25 keys, each with its position as value") {
    peano4::grid::LatticeIndexMap map;
    map.reserve(32);
    for (int position = 0; position < 25; position++) {
      map.insert(getLatticeKey(position, 9), position);
    }

    WHEN("The test copies the map into a second map") {
      const peano4::grid::LatticeIndexMap copiedMap(map);

      THEN("The copy stores as many entries as the map and finds every key with its value") {
        VALIDATE_EQUALS(copiedMap.getSize(), map.getSize());
        for (int position = 0; position < 25; position++) {
          VALIDATE_EQUALS(copiedMap.find(getLatticeKey(position, 9)), position);
        }
      }

      AND_WHEN("The test assigns the copy to a third map") {
        peano4::grid::LatticeIndexMap assignedMap;
        assignedMap = copiedMap;

        THEN("The assigned map stores as many entries as the copy and finds every key with its value") {
          VALIDATE_EQUALS(assignedMap.getSize(), copiedMap.getSize());
          for (int position = 0; position < 25; position++) {
            VALIDATE_EQUALS(assignedMap.find(getLatticeKey(position, 9)), position);
          }
        }
      }
    }

    WHEN("The test moves the map into a second map") {
      const peano4::grid::LatticeIndexMap movedMap(std::move(map));

      THEN("The second map stores the 25 entries and finds every key with its value") {
        VALIDATE_EQUALS(static_cast<int>(movedMap.getSize()), 25);
        for (int position = 0; position < 25; position++) {
          VALIDATE_EQUALS(movedMap.find(getLatticeKey(position, 9)), position);
        }
      }
    }
  }
}

UNIT_TEST("An erase reports whether it removed an entry") {
  GIVEN("A map that stores the keys of the positions 2, 6 and 3, 6 with the values 40 and 41") {
    peano4::grid::LatticeIndexMap map;
    map.reserve(16);
    map.insert(getLatticeKey(2, 6), 40);
    map.insert(getLatticeKey(3, 6), 41);

    WHEN("The map erases the first key") {
      const bool erasedFirstKey = map.erase(getLatticeKey(2, 6));

      THEN("The erase removes the first key and keeps the second one") {
        VALIDATE(erasedFirstKey);
        VALIDATE_EQUALS(map.getSize(), 1);
        VALIDATE_EQUALS(map.find(getLatticeKey(2, 6)), peano4::grid::LatticeIndexMap::NoEntry);
        VALIDATE_EQUALS(map.find(getLatticeKey(3, 6)), 41);
      }

      AND_WHEN("The map erases the first key again and a key it never stored") {
        const bool erasedFirstKeyAgain = map.erase(getLatticeKey(2, 6));
        const bool erasedAbsentKey     = map.erase(getLatticeKey(9, 9));

        THEN("Both erases return false and the size stays one") {
          VALIDATE(not erasedFirstKeyAgain);
          VALIDATE(not erasedAbsentKey);
          VALIDATE_EQUALS(map.getSize(), 1);
        }
      }
    }
  }

  GIVEN("An empty map") {
    peano4::grid::LatticeIndexMap emptyMap;

    WHEN("The map erases the key of the origin") {
      const bool erased = emptyMap.erase(getLatticeKey(0, 0));

      THEN("The erase returns false and the size stays zero") {
        VALIDATE(not erased);
        VALIDATE_EQUALS(emptyMap.getSize(), 0);
      }
    }
  }
}

UNIT_TEST("A probe still reaches every entry behind an erased slot") {
  GIVEN("A map of 64 keys along the first axis, each with its position as value") {
    constexpr int NumberOfKeys = 64;

    peano4::grid::LatticeIndexMap map;
    map.reserve(NumberOfKeys);
    for (int position = 0; position < NumberOfKeys; position++) {
      map.insert(getLatticeKey(position, 5), position);
    }

    WHEN("The map erases the keys of the even positions") {
      std::vector<bool> eraseResults;
      for (int position = 0; position < NumberOfKeys; position += 2) {
        eraseResults.push_back(map.erase(getLatticeKey(position, 5)));
      }

      THEN("Every erase removes its key and the map finds the odd positions alone") {
        for (const bool erased : eraseResults) {
          VALIDATE(erased);
        }
        VALIDATE_EQUALS(static_cast<int>(map.getSize()), NumberOfKeys / 2);
        for (int position = 0; position < NumberOfKeys; position++) {
          const auto expectedValue = position % 2 == 0 ? peano4::grid::LatticeIndexMap::NoEntry : position;
          VALIDATE_EQUALS(map.find(getLatticeKey(position, 5)), expectedValue);
        }
      }

      AND_WHEN("The map visits its entries") {
        std::vector<std::pair<std::int64_t, int>> visitedEntries;
        map.forEachEntry([&visitedEntries](const std::int64_t key, const int value) {
          visitedEntries.emplace_back(key, value);
        });

        THEN("The visits reach the keys of the odd positions alone, each with its value") {
          for (const auto& [key, value] : visitedEntries) {
            VALIDATE_EQUALS(key, getLatticeKey(value, 5));
            VALIDATE(value % 2 == 1);
          }
          VALIDATE_EQUALS(static_cast<int>(visitedEntries.size()), NumberOfKeys / 2);
        }
      }
    }
  }
}

UNIT_TEST("An erased slot takes an entry again and the growth that drops the tombstones keeps every key") {
  GIVEN("A map of 64 keys along the first axis, each with its position as value") {
    constexpr int NumberOfKeys = 64;

    peano4::grid::LatticeIndexMap map;
    map.reserve(NumberOfKeys);
    for (int position = 0; position < NumberOfKeys; position++) {
      map.insert(getLatticeKey(position, 2), position);
    }

    WHEN("The map erases every key") {
      std::vector<bool> eraseResults;
      for (int position = 0; position < NumberOfKeys; position++) {
        eraseResults.push_back(map.erase(getLatticeKey(position, 2)));
      }

      THEN("Every erase removes its key and the map stores zero entries") {
        for (const bool erased : eraseResults) {
          VALIDATE(erased);
        }
        VALIDATE_EQUALS(map.getSize(), 0);
      }

      AND_WHEN("The test inserts the keys again with the value 1000 plus the position") {
        std::vector<std::pair<int, bool>> insertResults;
        for (int position = 0; position < NumberOfKeys; position++) {
          insertResults.push_back(map.insert(getLatticeKey(position, 2), 1000 + position));
        }

        THEN("Every insert stores its value as a new entry") {
          for (int position = 0; position < NumberOfKeys; position++) {
            const auto [storedValue, isNew] = insertResults[static_cast<std::size_t>(position)];
            VALIDATE_EQUALS(storedValue, 1000 + position);
            VALIDATE(isNew);
          }
          VALIDATE_EQUALS(static_cast<int>(map.getSize()), NumberOfKeys);
        }

        AND_WHEN("The map grows by the keys of three times as many further positions") {
          for (int position = NumberOfKeys; position < 4 * NumberOfKeys; position++) {
            map.insert(getLatticeKey(position, 2), 1000 + position);
          }

          THEN("The map finds every key with its value") {
            VALIDATE_EQUALS(static_cast<int>(map.getSize()), 4 * NumberOfKeys);
            for (int position = 0; position < 4 * NumberOfKeys; position++) {
              VALIDATE_EQUALS(map.find(getLatticeKey(position, 2)), 1000 + position);
            }
          }
        }
      }
    }
  }
}

UNIT_TEST("An empty map counts zero bytes and a reserved map its key and value arrays before and after clear()") {
  GIVEN("An empty map and the capacity 16 that a reservation of five entries takes") {
    constexpr std::size_t Capacity = 16;

    peano4::grid::LatticeIndexMap map;

    THEN("The empty map counts zero bytes") { VALIDATE_EQUALS(map.getAllocatedBytes(), std::size_t{0}); }

    WHEN("The map reserves five entries") {
      map.reserve(5);

      THEN("The map stores the key and value arrays of the capacity") {
        VALIDATE_EQUALS(map.getAllocatedBytes(), Capacity * (sizeof(std::int64_t) + sizeof(int)));
      }

      AND_WHEN("The map clears") {
        map.clear();

        THEN("The map keeps the key and value arrays of the capacity") {
          VALIDATE_EQUALS(map.getAllocatedBytes(), Capacity * (sizeof(std::int64_t) + sizeof(int)));
        }
      }
    }
  }
}
