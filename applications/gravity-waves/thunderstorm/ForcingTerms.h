#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::GravityWaves::Thunderstorm::ForcingTerms {

  static inline GPU_CALLABLE_METHOD void scenario(
    [[maybe_unused]] const double* const __restrict__ Q,
    [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
    [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& h,
    [[maybe_unused]] const double                                 t,
    [[maybe_unused]] double* const __restrict__ S
  );
} // namespace applications::exahype2::GravityWaves::Thunderstorm::ForcingTerms

#include "ForcingTerms.cpph"
