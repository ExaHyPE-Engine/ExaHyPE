// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

TARGET_DECLARE
namespace tarch::la {
  class ExactAccumulator;
} // namespace tarch::la

/**
 * Fixed-point accumulator that sums doubles exactly, on the host and on a device.
 */
class tarch::la::ExactAccumulator final {
public:
  INLINE_METHOD GPU_CALLABLE_METHOD void add(double value);
  INLINE_METHOD GPU_CALLABLE_METHOD void merge(const tarch::la::ExactAccumulator& other);
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double round() const;
  template <int Divisor>
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD double roundQuotient() const;
  INLINE_METHOD GPU_CALLABLE_METHOD void clear();

private:
  static constexpr int          BitsPerLimb   = 32;
  static constexpr int          NumberOfLimbs = 67;
  static constexpr std::int64_t LimbMask      = 0xFFFFFFFF;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD int leadingZerosOfDigit(std::uint64_t digit);

  std::int64_t _limbs[NumberOfLimbs]{};
};
TARGET_DECLARE_END

#include "tarch/la/ExactAccumulator.cpph"
