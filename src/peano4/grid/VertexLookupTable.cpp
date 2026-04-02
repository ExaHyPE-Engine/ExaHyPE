// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VertexLookupTable.h"

#include "PeanoCurve.h"

#include <cstdint>
#include <unordered_map>

namespace {
  constexpr int AccessNumberEncodingBits = 5;
  constexpr int AccessNumberEncodingBias = 2 * DIMENSIONS - 1;

  using VertexLookupCache = std::unordered_map<std::uint64_t, peano4::grid::VertexLookupTable>;

  INLINE_METHOD std::uint64_t encodeVertexLookupTableKey(
    const peano4::grid::AutomatonState&       fineGridState,
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  ) {
    std::uint64_t key = 0;

    for (int d = 0; d < DIMENSIONS; ++d) {
      assertion(cellPositionWithin3x3Patch[d] >= 0);
      assertion(cellPositionWithin3x3Patch[d] < 3);
      key = key * 3 + static_cast<std::uint64_t>(cellPositionWithin3x3Patch[d]);
    }

    constexpr std::uint64_t usedFlagBitsMask = (std::uint64_t(1) << (DIMENSIONS + 1)) - 1;
    key = (key << (DIMENSIONS + 1)) | (static_cast<std::uint64_t>(fineGridState.flags) & usedFlagBitsMask);

    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int encodedAccessNumber = static_cast<int>(fineGridState.accessNumber[i]) + AccessNumberEncodingBias;
      assertion(encodedAccessNumber >= 0);
      assertion(encodedAccessNumber < (1 << AccessNumberEncodingBits));
      key = (key << AccessNumberEncodingBits) | static_cast<std::uint64_t>(encodedAccessNumber);
    }

    return key;
  }

  VertexLookupCache& getVertexLookupTableCache() {
    thread_local VertexLookupCache cache = []() {
      VertexLookupCache result;
      result.reserve(512);
      return result;
    }();
    return cache;
  }

  peano4::grid::VertexLookupTable buildVertexLookupTable(
    const peano4::grid::AutomatonState&       fineGridState,
    const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
  ) {
    peano4::grid::VertexLookupTable lut;

    const int first = static_cast<int>(peano4::grid::PeanoCurve::getFirstVertexIndex(fineGridState).to_ulong());

    for (int i = 0; i < TWO_POWER_D; ++i) {
      const int vIdx = first ^ i;

      lut.vIdx[i] = vIdx;
      for (int d = 0; d < DIMENSIONS; ++d) {
        lut.vPos[i][d] = cellPositionWithin3x3Patch[d] + ((vIdx >> d) & 1);
      }

      const int readStack  = peano4::grid::PeanoCurve::getVertexReadStackNumber(fineGridState, vIdx);
      const int writeStack = peano4::grid::PeanoCurve::getVertexWriteStackNumber(fineGridState, vIdx);
      lut.readStack[i]     = readStack;
      lut.writeStack[i]    = writeStack;
      lut.inOutRead[i]     = peano4::grid::PeanoCurve::isInOutStack(readStack);
      lut.inOutWrite[i]    = peano4::grid::PeanoCurve::isInOutStack(writeStack);
    }

    for (int i = 0; i < TWO_TIMES_D; ++i) {
      const int faceIndex   = peano4::grid::PeanoCurve::getFaceNumberAlongCurve(fineGridState, i);
      lut.faceIndex[i]      = faceIndex;
      lut.faceReadStack[i]  = peano4::grid::PeanoCurve::getFaceReadStackNumber(fineGridState, faceIndex);
      lut.faceWriteStack[i] = peano4::grid::PeanoCurve::getFaceWriteStackNumber(fineGridState, faceIndex);
    }
    lut.cellReadStack  = peano4::grid::PeanoCurve::getCellReadStackNumber(fineGridState);
    lut.cellWriteStack = peano4::grid::PeanoCurve::getCellWriteStackNumber(fineGridState);

    return lut;
  }
} // namespace

const peano4::grid::VertexLookupTable& peano4::grid::createVertexLookupTable(
  const AutomatonState&                     fineGridState,
  const tarch::la::Vector<DIMENSIONS, int>& cellPositionWithin3x3Patch
) {
  VertexLookupCache&  cache = getVertexLookupTableCache();
  const std::uint64_t key   = encodeVertexLookupTableKey(fineGridState, cellPositionWithin3x3Patch);

  const auto cachedEntry = cache.find(key);
  if (cachedEntry != cache.end()) {
    return cachedEntry->second;
  }

  const auto insertedEntry = cache.emplace(key, buildVertexLookupTable(fineGridState, cellPositionWithin3x3Patch));
  return insertedEntry.first->second;
}
