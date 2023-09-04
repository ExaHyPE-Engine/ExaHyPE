// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace exahype2::advection {
  static void neumannBoundaryConditions(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables
  ) InlineMethod {
#ifdef GPUOffloadingOff
    nonCriticalAssertion4(Qinside[0] > 1e-12, faceCentre, t, normal, Qinside[0]);
#endif

    for (int i = 0; i < numberOfUnknowns + numberOfAuxiliaryVariables; i++) {
      Qoutside[i] = Qinside[i];
    }
  }

  static void neumannBoundaryConditions(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables
  ) InlineMethod {
    neumannBoundaryConditions(Qinside, Qoutside, faceCentre, t, normal, numberOfUnknowns, numberOfAuxiliaryVariables);
  }

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
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, faceCentre, t, normal);
    assertion3(normal < Dimensions, faceCentre, t, normal);
#endif

    F[0] = 0.0;
    F[1] = 0.0;
#if Dimensions == 3
    F[2] = 0.0;
#endif

    F[normal] = Q[normal];
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    double* __restrict__ F
  ) InlineMethod {
    flux(Q, faceCentre, t, dt, normal, F);
  }
} // namespace exahype2::advection
