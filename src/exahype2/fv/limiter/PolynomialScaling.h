// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::limiter {
  template <typename Basis>
  struct PolynomialScaling;

  template <typename Basis>
  struct Unlimited;
} // namespace exahype2::fv::limiter

TARGET_DECLARE
/**
 * Mean-preserving scaling of a nodal polynomial after Zhang and Shu (2010). The polynomial p of a
 * volume with the average Q becomes Q + theta (p - Q) with the largest theta in [0, 1] for which
 * every node of Basis and every node on a face of the volume passes the physical admissibility term
 * of Derived. One theta scales all entries of the state, so a linear relation that holds throughout
 * the volume keeps holding. Bisection along the segment from Q to every point finds theta, which is
 * the largest one if the admissible states form a convex set. An inadmissible average gives
 * theta = 0, the constant polynomial.
 */
template <typename Basis>
struct exahype2::fv::limiter::PolynomialScaling final {
  static constexpr int NodesPerAxis   = Basis::NumberOfNodes;
  static constexpr int NodesPerCell   = peano4::power(NodesPerAxis, DIMENSIONS);
  static constexpr int NodesPerFace   = peano4::power(NodesPerAxis, DIMENSIONS - 1);
  static constexpr int Points         = NodesPerCell + 2 * DIMENSIONS * NodesPerFace;
  static constexpr int BisectionSteps = 24;

  template <typename Derived, int NumberOfData, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static ComputePrecision limit(
    const StoragePrecision* NOALIAS const        average,
    ComputePrecision* NOALIAS const              nodalState,
    const tarch::la::Vector<DIMENSIONS, double>& volumeCentre,
    const tarch::la::Vector<DIMENSIONS, double>& volumeSize
  );

  template <int NumberOfData, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void evaluate(
    const ComputePrecision* NOALIAS const nodalState,
    int                                   point,
    ComputePrecision* NOALIAS const       values
  );
};

/**
 * Leaves the nodal polynomial of every volume as the reconstruction yields it.
 */
template <typename Basis>
struct exahype2::fv::limiter::Unlimited final {};
TARGET_DECLARE_END

#include "exahype2/fv/limiter/PolynomialScaling.cpph"
