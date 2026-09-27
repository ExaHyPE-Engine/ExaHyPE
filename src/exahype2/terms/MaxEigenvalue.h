// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::terms {
  template <typename SolverType, typename Precision>
  concept HasMaxEigenvalue = requires(
    const Precision* const                       Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal
  ) {
    { SolverType::maxEigenvalue(Q, x, h, t, dt, normal) } -> std::same_as<double>;
  };
} // namespace exahype2::terms
