// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractEulerSolver.h"

namespace applications::exahype2::euler {
  class EulerSolver;
} // namespace applications::exahype2::euler

class applications::exahype2::euler::EulerSolver: public AbstractEulerSolver {
protected:
  static tarch::logging::Log _log;

public:
  void initialCondition(
    double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const tarch::la::Vector<Dimensions, int>&    index,
    bool                                         gridIsConstructred
  ) override final;

  void boundaryConditions(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    int                                          normal
  ) override final;

  ::exahype2::RefinementCommand refinementCriterion(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t
  ) override final;

  double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal
  ) override final;

  static inline double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    Offloadable
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[0] != Q[0]);
    assertion(Q[0] > 0);
#endif

    const double irho = 1.0 / std::abs(Q[0]);
#if Dimensions == 3
    const double p = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));
#else
    const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
#endif

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(p, 0.0));
#endif
    const double c = std::sqrt(gamma * std::abs(p) * irho);

    const double u_n    = Q[normal + 1] * irho;
    const double result = std::max(std::abs(u_n - c), std::abs(u_n + c));

#ifdef GPUOffloadingOff
    assertion(tarch::la::greaterEquals(result, 0.0));
#endif

    return result;
  }

  void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) override final;

  static inline void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F,
    Offloadable
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, x, t, normal);
    assertion3(normal < Dimensions, x, t, normal);
    assertion(Q[0] != Q[0]);
    assertion(Q[0] > 0);
#endif

    const double irho = 1.0 / Q[0];
#if Dimensions == 3
    const double p = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));
#else
    const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
#endif

    const double coeff = irho * Q[normal + 1];
    F[0]               = coeff * Q[0];
    F[1]               = coeff * Q[1];
    F[2]               = coeff * Q[2];

#if Dimensions == 3
    F[3] = coeff * Q[3];
    F[4] = coeff * Q[4] + coeff * p;
#else
    F[3] = coeff * Q[3] + coeff * p;
#endif

    F[normal + 1] += p;
  }
};
