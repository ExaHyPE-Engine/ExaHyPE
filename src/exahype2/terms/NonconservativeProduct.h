// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasNonconservativeProduct = requires(
    const Precision* const                        Q,
    const Precision* const                        deltaQ,
    const tarch::la::Vector<DIMENSIONS, double>&  x,
    const tarch::la::Vector<DIMENSIONS, double>&  h,
    const double                                  t,
    const double                                  dt,
    const int                                     normal,
    Precision* const                              BTimesDeltaQ,
    const typename SolverType::RuntimeParameters& parameters
  ) {
    {
      SolverType::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ, parameters)
    } -> std::same_as<void>;
  };
} // namespace exahype2::terms
