// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::fv::limiter {
  /**
   * A slope limiter maps the neighbourhood of one volume onto the limited
   * difference of one unknown, which drives the linear reconstruction.
   */
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
 * Zero slope, which falls back to the first-order piecewise-constant scheme.
 */
struct exahype2::fv::limiter::Zero final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};

/**
 * Unlimited central difference, which gives the full second-order slope. It is
 * not monotone, so it serves smooth problems and convergence studies.
 */
struct exahype2::fv::limiter::Average final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};

/**
 * Minmod, the most diffusive second-order limiter: The one-sided difference of smaller
 * magnitude when both differences have the same sign, zero otherwise.
 */
struct exahype2::fv::limiter::Minmod final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};

/**
 * Van Albada, a smooth limiter that is less compressive than Minmod and does
 * not clip smooth extrema as hard.
 */
struct exahype2::fv::limiter::VanAlbada final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};

/**
 * Superbee, the most compressive second-order limiter, which sharpens
 * discontinuities at the cost of steepening smooth profiles.
 */
struct exahype2::fv::limiter::Superbee final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};

/**
 * Monotonised central-difference limiter, a balanced choice between the
 * central difference and the two one-sided differences.
 */
struct exahype2::fv::limiter::MinmodCentral final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limitedDifference(
    const ComputePrecision forwardDifference,
    const ComputePrecision backwardDifference
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/limiter/SlopeLimiter.cpph"
