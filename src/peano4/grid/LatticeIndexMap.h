// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace peano4::grid {
  class LatticeIndexMap;
} // namespace peano4::grid

/**
 * Hash map from non-negative lattice keys to entity indices, with open addressing over a key array
 * of power-of-two length. A map that reserve() sizes for the entities of a level fills without a
 * rehash.
 */
class peano4::grid::LatticeIndexMap final {
public:
  /**
   * Marks a free slot. Lattice keys are non-negative, so no key collides with it.
   */
  static constexpr std::int64_t EmptyKey{-1};

  /**
   * Marks an erased slot, which a probe passes and an insert may reuse.
   */
  static constexpr std::int64_t TombstoneKey{-2};

  /**
   * The value find() returns for a missing key.
   */
  static constexpr int NoEntry{-1};

  LatticeIndexMap()                                  = default;
  LatticeIndexMap(const LatticeIndexMap&)            = default;
  LatticeIndexMap(LatticeIndexMap&&)                 = default;
  LatticeIndexMap& operator=(const LatticeIndexMap&) = default;
  LatticeIndexMap& operator=(LatticeIndexMap&&)      = default;
  ~LatticeIndexMap()                                 = default;

  void reserve(std::size_t numberOfEntries);

  void clear();

  [[nodiscard]] INLINE_METHOD int find(std::int64_t key) const;

  INLINE_METHOD std::pair<int, bool> insert(std::int64_t key, int value);

  INLINE_METHOD bool erase(std::int64_t key);

  [[nodiscard]] INLINE_METHOD std::size_t getSize() const;
  [[nodiscard]] INLINE_METHOD std::size_t getAllocatedBytes() const;

  template <typename Callback>
  INLINE_METHOD void forEachEntry(const Callback& callback) const;

private:
  /**
   * Smallest length of the key array.
   */
  static constexpr std::size_t SmallestCapacity{8};

  std::vector<std::int64_t> _keys{};
  std::vector<int>          _values{};

  /**
   * The length of the key array minus one, so a probe wraps with a bitwise and.
   */
  std::size_t _slotMask{0};

  std::size_t _size{0};

  /**
   * Number of erased slots, which the next growth clears.
   */
  std::size_t _numberOfTombstones{0};

  [[nodiscard]] INLINE_METHOD std::size_t getFirstSlot(std::int64_t key) const;

  void grow();
};

#include "peano4/grid/LatticeIndexMap.cpph"
