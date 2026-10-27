// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2::fv::limiter {
  template <typename Limiter>
  concept SlopeLimiter = std::is_empty_v<Limiter>;

  struct Zero;
  struct Average;
  struct Minmod;
  struct VanAlbada;
  struct Superbee;
  struct MinmodCentral;
} // namespace exahype2::fv::limiter

TARGET_DECLARE
/**
 * Zero slope. The scheme becomes first-order piecewise constant.
 */
struct exahype2::fv::limiter::Zero final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};

/**
 * Unlimited central difference. The scheme is not monotone.
 */
struct exahype2::fv::limiter::Average final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};

/**
 * Most diffusive second-order limiter.
 */
struct exahype2::fv::limiter::Minmod final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};

/**
 * Smooth limiter, less diffusive than Minmod. Opposite-sign differences give a small counter-slope.
 */
struct exahype2::fv::limiter::VanAlbada final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};

/**
 * Most compressive second-order limiter. The limiter also steepens smooth profiles.
 */
struct exahype2::fv::limiter::Superbee final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};

/**
 * Monotonised central-difference limiter.
 */
struct exahype2::fv::limiter::MinmodCentral final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    ComputePrecision forwardDifference,
    ComputePrecision backwardDifference
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/limiter/SlopeLimiter.cpph"
