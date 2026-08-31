// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"

namespace tarch::la {
  class ExactAccumulator;
} // namespace tarch::la

/**
 * Fixed-point accumulator that sums doubles exactly.
 */
class tarch::la::ExactAccumulator final {
public:
  INLINE_METHOD void add(double value);
  double             round() const;
  INLINE_METHOD void clear();

private:
  static constexpr int          BitsPerLimb   = 32;
  static constexpr int          NumberOfLimbs = 67;
  static constexpr std::int64_t LimbMask      = 0xffffffff;

  std::int64_t _limbs[NumberOfLimbs]{};
};

#include "tarch/la/ExactAccumulator.cpph"
