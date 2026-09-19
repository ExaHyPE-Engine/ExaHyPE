// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace peano4::data {
  class MemoryUsage;

  template <class Data>
  INLINE_METHOD std::uint64_t payloadBytesOf(std::size_t numberOfSlots);

  template <class Data>
  INLINE_METHOD void releasePoolStorage();
} // namespace peano4::data

/**
 * Counts the bytes the cell, face and vertex data of all ranks occupy.
 *
 * With several ranks the counters hold the sums the preceding update reduced, so they lag one
 * sweep behind. The first update has no reduced sums and reports the bytes of its own rank.
 */
class peano4::data::MemoryUsage final {
public:
  static void update(std::uint64_t cellBytes, std::uint64_t faceBytes, std::uint64_t vertexBytes);

  [[nodiscard]] static std::size_t getGlobalCellDataMemoryUsageInBytes();
  [[nodiscard]] static std::size_t getGlobalFaceDataMemoryUsageInBytes();
  [[nodiscard]] static std::size_t getGlobalVertexDataMemoryUsageInBytes();

private:
  static constexpr int NumberOfDataMemoryWords = 3;

  inline static std::uint64_t _globalCellDataMemoryUsageInBytes{0};
  inline static std::uint64_t _globalFaceDataMemoryUsageInBytes{0};
  inline static std::uint64_t _globalVertexDataMemoryUsageInBytes{0};

  inline static std::array<std::uint64_t, NumberOfDataMemoryWords> _reducedDataMemory{};
  inline static bool                                               _hasReducedDataMemory{false};
};

#include "peano4/data/MemoryUsage.cpph"
