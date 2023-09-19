// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace exahype2::elasticwave {
  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       c
  ) InlineMethod {
    return c;
  }

  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       c
  ) InlineMethod {
    return maxEigenvalue(Q, faceCentre, t, dt, normal, c);
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       rho,
    double                                       cp,
    double                                       cs,
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, faceCentre, t, normal);
    assertion3(normal < Dimensions, faceCentre, t, normal);
#endif

    const double mu  = rho*cs*cs;
    const double lambda = rho*cp*cp-2.0*mu;

    const double u = Q[0];
    const double v = Q[1];
    const double sxx = Q[2];
    const double syy = Q[3];
    const double sxy = Q[4];

    switch(normal) {
    case 0:
      F[0] = -1.0/rho*sxx;
      F[1] = -1.0/rho*sxy;
      F[2] = -(2.0*mu+lambda)*u;
      F[3] = -lambda * u;
      F[4] = -mu*v;
      break;
    case 1:
      F[0] = -1.0/rho*sxy;
      F[1] = -1.0/rho*syy;
      F[2] = -lambda * v;
      F[3] = -(2.0*mu+lambda)*v;
      F[4] = -mu*u;
      break;
    }
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double                                       rho,
    double                                       cp,
    double                                       cs,
    double* __restrict__ F
  ) InlineMethod {
    flux(Q, faceCentre, t, dt, normal, rho, cp, cs, F);
  }
} // namespace exahype2::elasticwave
