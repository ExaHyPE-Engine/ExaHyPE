// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace exahype2::euler {
  /**
   * Apply Neumann BCs to all quantities. This includes the material
   * parameters (auxiliary ones).
   */
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

  /**
   * Dimension-generic implementation of the max eigenvalue computation
   *
   * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
   * return one scalar over the numberOfUnknowns PDE equations.
   */
  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables,
    double                                       gamma = 1.4
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);

    if (Q[0] <= 0.0 or Q[0] != Q[0]) {
      ::exahype2::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        "Q[0]>0",
        "density negative (Q[0]=" + std::to_string(Q[0]) + ") at x=" + toString(faceCentre)
      );
    }
#endif

    const double irho = 1.0 / std::abs(Q[0]);
#if Dimensions == 3
    const double p = (gamma - 1) * (Q[4] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));
#else
    const double p = (gamma - 1) * (Q[3] - 0.5 * irho * (Q[1] * Q[1] + Q[2] * Q[2]));
#endif

#ifdef GPUOffloadingOff
    nonCriticalAssertion8(tarch::la::greaterEquals(p, 0.0), Q[0], Q[1], Q[2], Q[3], Q[4], faceCentre, t, normal);
#endif
    const double c = std::sqrt(gamma * std::abs(p) * irho);

    const double u_n    = Q[normal + 1] * irho;
    const double result = std::max(std::abs(u_n - c), std::abs(u_n + c));

#ifdef GPUOffloadingOff
    nonCriticalAssertion8(result >= 0.0, result, p, u_n, irho, c, faceCentre, t, normal);
#endif

    return result;
  }

  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables,
    double                                       gamma = 1.4
  ) InlineMethod {
    return maxEigenvalue(Q, faceCentre, t, dt, normal, numberOfUnknowns, numberOfAuxiliaryVariables, gamma);
  }

  /**
   * Dimension-generic implementation of the flux computation
   *
   * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
   * return F[numberOfUnknowns] fluxes.
   */
  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables,
    double* __restrict__ F,
    double gamma = 1.4
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion3(normal >= 0, faceCentre, t, normal);
    assertion3(normal < Dimensions, faceCentre, t, normal);
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
    F[normal + 1] += p;

#if Dimensions == 3
    F[3] = coeff * Q[3];
    F[4] = coeff * Q[4] + coeff * p;
#else
    F[3]           = coeff * Q[3] + coeff * p;
#endif
  }

  static void flux(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables,
    double* __restrict__ F,
    double gamma = 1.4
  ) InlineMethod {
    flux(Q, faceCentre, t, dt, normal, numberOfUnknowns, numberOfAuxiliaryVariables, F, gamma);
  }
} // namespace exahype2::euler
