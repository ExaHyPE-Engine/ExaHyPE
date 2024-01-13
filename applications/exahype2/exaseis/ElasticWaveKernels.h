// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "VariableShortcuts.h"

namespace applications::exahype2::exaseis {
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod double maxEigenvalue(
    const double* __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, int normal
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    return std::max(std::abs(Q[VARS_cp]), std::abs(Q[VARS_cs]));
  }


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
#endif

    switch (normal) {
    case 0:
      F[VARS_vx] = -Q[VARS_sxx];
      F[VARS_vy] = -Q[VARS_sxy];
      F[VARS_vz] = -Q[VARS_sxz];
      break;
    case 1:
      F[VARS_vx] = -Q[VARS_sxy];
      F[VARS_vy] = -Q[VARS_syy];
      F[VARS_vz] = -Q[VARS_syz];
      break;
    case 2:
      F[VARS_vx] = -Q[VARS_sxz];
      F[VARS_vy] = -Q[VARS_syz];
      F[VARS_vz] = -Q[VARS_szz];
    }

    F[VARS_sxx] = 0.0;
    F[VARS_syy] = 0.0;
    F[VARS_szz] = 0.0;
    F[VARS_sxy] = 0.0;
    F[VARS_sxz] = 0.0;
    F[VARS_syz] = 0.0;
  }


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void sourceTerm(
    const double* const __restrict__ Q, const tarch::la::Vector<Dimensions, double>& x, const tarch::la::Vector<Dimensions, double>& h, double t, double dt, double* __restrict__ S
  ) InlineMethod {
    S[VARS_vx]  = 0.0;
    S[VARS_vy]  = 0.0;
    S[VARS_vz]  = 0.0;
    S[VARS_sxx] = 0.0;
    S[VARS_syy] = 0.0;
    S[VARS_szz] = 0.0;
    S[VARS_sxy] = 0.0;
    S[VARS_sxz] = 0.0;
    S[VARS_syz] = 0.0;

    tarch::la::Vector<Dimensions, double> sourceLocation({SOURCE_LOCATION_X, SOURCE_LOCATION_Y, SOURCE_LOCATION_Z});
    if (tarch::la::norm2(x - sourceLocation) < SOURCE_RADIUS) {
      S[VARS_sxz] = SOURCE_M0 * t / (SOURCE_t0 * SOURCE_t0) * std::exp(-t / SOURCE_t0);
    }
  }


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void pointSource(
    const double* const Q, const double* const x, const double t, const double dt, double* const forceVector, int n
  ) InlineMethod {
    forceVector[VARS_vx]  = 0.0;
    forceVector[VARS_vy]  = 0.0;
    forceVector[VARS_vz]  = 0.0;
    forceVector[VARS_sxx] = 0.0;
    forceVector[VARS_syy] = 0.0;
    forceVector[VARS_szz] = 0.0;
    forceVector[VARS_sxy] = 0.0;
    forceVector[VARS_sxz] = 0.0;
    forceVector[VARS_syz] = 0.0;

    forceVector[VARS_sxz] = SOURCE_M0 * t / (SOURCE_t0 * SOURCE_t0) * std::exp(-t / SOURCE_t0);
}


  KeywordToAvoidDuplicateSymbolsForInlinedFunctions GPUCallableInlineMethod void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ BTimesDeltaQ
  ) InlineMethod {
    // Lamé parameters
    const double mu     = Q[VARS_rho] * Q[VARS_cs] * Q[VARS_cs];
    const double lambda = Q[VARS_rho] * Q[VARS_cp] * Q[VARS_cp] - 2 * mu;

    switch (normal) {
    case 0:
      BTimesDeltaQ[VARS_sxx] = -deltaQ[VARS_vx];
      BTimesDeltaQ[VARS_syy] = 0.0;
      BTimesDeltaQ[VARS_szz] = 0.0;
      BTimesDeltaQ[VARS_sxy] = -deltaQ[VARS_vy];
      BTimesDeltaQ[VARS_sxz] = -deltaQ[VARS_vz];
      BTimesDeltaQ[VARS_syz] = 0.0;
      break;
    case 1:
      BTimesDeltaQ[VARS_sxx] = 0.0;
      BTimesDeltaQ[VARS_syy] = -deltaQ[VARS_vy];
      BTimesDeltaQ[VARS_szz] = 0.0;
      BTimesDeltaQ[VARS_sxy] = -deltaQ[VARS_vx];
      BTimesDeltaQ[VARS_sxz] = 0.0;
      BTimesDeltaQ[VARS_syz] = -deltaQ[VARS_vz];
      break;
    case 2:
      BTimesDeltaQ[VARS_sxx] = 0.0;
      BTimesDeltaQ[VARS_syy] = 0.0;
      BTimesDeltaQ[VARS_szz] = -deltaQ[VARS_vz];
      BTimesDeltaQ[VARS_sxy] = 0.0;
      BTimesDeltaQ[VARS_sxz] = -deltaQ[VARS_vx];
      BTimesDeltaQ[VARS_syz] = -deltaQ[VARS_vy];
    }

    BTimesDeltaQ[VARS_vx] = 0.0;
    BTimesDeltaQ[VARS_vy] = 0.0;
    BTimesDeltaQ[VARS_vz] = 0.0;
  }
} // namespace applications::exahype2::exaseis
