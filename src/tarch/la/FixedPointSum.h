// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace tarch::la {
  template <std::int64_t MaximumNumberOfTerms>
  class FixedPointSum;
} // namespace tarch::la

/**
 * Sums at most MaximumNumberOfTerms doubles as 128-bit integers on a fixed-point grid, on the host
 * and on a device. The magnitude of every term stays at or below twice the bound that clear() sets.
 * Every order of the terms and of merge() returns the same bits.
 */
template <std::int64_t MaximumNumberOfTerms>
class tarch::la::FixedPointSum final {
public:
  INLINE_METHOD GPU_CALLABLE_METHOD void clear(double bound);
  INLINE_METHOD GPU_CALLABLE_METHOD void add(double term);
  INLINE_METHOD GPU_CALLABLE_METHOD void merge(const FixedPointSum& other);
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double round() const;

  template <int Divisor>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double roundQuotient() const;

private:
  static_assert(MaximumNumberOfTerms > 0, "A fixed-point sum adds at least one term.");

  static constexpr int TermBits = std::bit_width(static_cast<std::uint64_t>(MaximumNumberOfTerms - 1));

  static INLINE_METHOD GPU_CALLABLE_METHOD void negate(std::uint64_t& high, std::uint64_t& low);
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int getBitWidthOf(std::uint64_t value);
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getNearestDoubleOf(
    std::uint64_t high,
    std::uint64_t low,
    int           exponent
  );

  double        _bound{0.0};
  std::int64_t  _numberOfTerms{0};
  int           _unitExponent{0};
  std::uint64_t _high{0};
  std::uint64_t _low{0};
};
TARGET_DECLARE_END

#include "tarch/la/FixedPointSum.cpph"
