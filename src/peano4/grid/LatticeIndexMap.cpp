// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "LatticeIndexMap.h"

/**
 * Clears the map and sizes it for numberOfEntries entries. That many entries then fit without a
 * rehash.
 */
void
peano4::grid::LatticeIndexMap::reserve(const std::size_t numberOfEntries) {
  std::size_t capacity = SmallestCapacity;
  while (capacity < 2 * numberOfEntries) {
    capacity *= 2;
  }
  _keys.assign(capacity, EmptyKey);
  _values.assign(capacity, NoEntry);
  _slotMask           = capacity - 1;
  _size               = 0;
  _numberOfTombstones = 0;
}

void
peano4::grid::LatticeIndexMap::clear() {
  _keys.clear();
  _values.clear();
  _slotMask           = 0;
  _size               = 0;
  _numberOfTombstones = 0;
}

/**
 * Doubles the table and reinserts every entry, dropping the tombstones.
 */
void
peano4::grid::LatticeIndexMap::grow() {
  const auto keysBeforeGrowth   = std::move(_keys);
  const auto valuesBeforeGrowth = std::move(_values);

  const std::size_t capacity = keysBeforeGrowth.empty() ? SmallestCapacity : 2 * keysBeforeGrowth.size();
  _keys.assign(capacity, EmptyKey);
  _values.assign(capacity, NoEntry);
  _slotMask           = capacity - 1;
  _size               = 0;
  _numberOfTombstones = 0;

  for (std::size_t slot = 0; slot < keysBeforeGrowth.size(); slot++) {
    if (keysBeforeGrowth[slot] != EmptyKey and keysBeforeGrowth[slot] != TombstoneKey) {
      insert(keysBeforeGrowth[slot], valuesBeforeGrowth[slot]);
    }
  }
}
