// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SlopeLimiter.h"

namespace {
  template <typename Limiter, typename ComputePrecision>
  [[maybe_unused]] double instantiateSlopeLimiter() {
    return static_cast<double>(Limiter::limitedDifference(ComputePrecision(0), ComputePrecision(0)));
  }

  [[maybe_unused]] double instantiateSlopeLimiters() {
    double result = 0.0;
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Zero, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Zero, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Zero, long double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Average, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Average, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Average, long double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Minmod, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Minmod, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Minmod, long double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::VanAlbada, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::VanAlbada, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::VanAlbada, long double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Superbee, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Superbee, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::Superbee, long double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::MinmodCentral, float>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::MinmodCentral, double>();
    result += instantiateSlopeLimiter<exahype2::fv::limiter::MinmodCentral, long double>();
    return result;
  }
} // namespace
