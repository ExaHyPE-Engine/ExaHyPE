// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractAdvectionSolver.h"

namespace applications::exahype2::advection {
  class AdvectionSolver;
} // namespace applications::exahype2::advection

class applications::exahype2::advection::AdvectionSolver: public AbstractAdvectionSolver {
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
    return c;
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
#endif

    F[0] = 0.0;
    F[1] = 0.0;
#if Dimensions == 3
    F[2] = 0.0;
#endif

    F[normal] = Q[normal];
  }
};
