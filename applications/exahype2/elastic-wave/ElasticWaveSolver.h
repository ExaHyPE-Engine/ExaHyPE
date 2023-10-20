// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractElasticWaveSolver.h"

namespace applications::exahype2::elasticwave {
  class ElasticWaveSolver;
} // namespace applications::exahype2::elasticwave

class applications::exahype2::elasticwave::ElasticWaveSolver: public AbstractElasticWaveSolver {
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

  /*void sourceTerm(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    double                                       t,
    double                                       dt,
    double* __restrict__ S
  ) override final;*/

  void algebraicSource(
    const tarch::la::Vector<Dimensions, double>& x, double t, const double* __restrict__ const Q, double* __restrict__ S
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
    return std::max(std::abs(cp), std::abs(cs));
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
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    const double mu     = rho * cs * cs;
    const double lambda = rho * cp * cp - 2.0 * mu;

    const double u   = Q[0];
    const double v   = Q[1];
    const double sxx = Q[2];
    const double syy = Q[3];
    const double sxy = Q[4];

    switch (normal) {
    case 0:
      F[0] = -1.0 / rho * sxx;
      F[1] = -1.0 / rho * sxy;
      F[2] = -(2.0 * mu + lambda) * u;
      F[3] = -lambda * u;
      F[4] = -mu * v;
      break;
    case 1:
      F[0] = -1.0 / rho * sxy;
      F[1] = -1.0 / rho * syy;
      F[2] = -lambda * v;
      F[3] = -(2.0 * mu + lambda) * v;
      F[4] = -mu * u;
      break;
    }
  }
};
