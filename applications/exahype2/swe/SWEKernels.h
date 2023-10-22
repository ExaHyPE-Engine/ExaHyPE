// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace exahype2::swe {
  static void reflectiveBoundaryConditions(
    const double* __restrict__ Qinside,
    double* __restrict__ Qoutside,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables
  ) InlineMethod {
#ifdef GPUOffloadingOff
    nonCriticalAssertion5(Qinside[0] > 1e-12, faceCentre, volumeH, t, normal, Qinside[0]);
#endif

    Qoutside[0] = Qinside[0];
    Qoutside[1] = -Qinside[1];
    Qoutside[2] = -Qinside[2];
    Qoutside[3] = Qinside[3];
  }

  static double maxEigenvalue(
    const double* __restrict__ Q,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion(normal >= 0);
    assertion(normal < Dimensions);
    assertion(Q[0] > 0);
#endif

    static constexpr double g      = 9.81; // [m/s^2]
    double                  result = 1.0;
    const double            ih     = 1.0 / Q[0]; // Inverse water height
    const double            c      = std::sqrt(g * Q[0]);
    double                  u      = 0.0;

    switch (normal) {
    case 0: // x
            // Q[1] is the momentum in x-direction but we need the velocity here.
            // Therefore, multiply by the inverse water height (i.e., we divide by the height).
      u      = ih * Q[1];
      result = std::max(std::abs(u - c), std::abs(u + c));
      break;
    case 1: // y
      u      = ih * Q[2];
      result = std::max(std::abs(u - c), std::abs(u + c));
      break;
    }

#ifdef GPUOffloadingOff
    assertion(result >= 0.0);
#endif

    return result;
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
    double* __restrict__ F
  ) InlineMethod {
#ifdef GPUOffloadingOff
    assertion4(normal >= 0, faceCentre, volumeH, t, normal);
    assertion4(normal < Dimensions, faceCentre, volumeH, t, normal);
    assertion(Q[0] > 0);
#endif

    const double ih = 1.0 / Q[0];

    switch (normal) {
    case 0:
      F[0] = Q[1];             // hu_1
      F[1] = ih * Q[1] * Q[1]; // h(u_1)^2
      F[2] = ih * Q[1] * Q[2]; // hu_1u_2
      break;
    case 1:
      F[0] = Q[2];             // hu_2
      F[1] = ih * Q[2] * Q[1]; // hu_1u_2
      F[2] = ih * Q[2] * Q[2]; // h(u_2)^2
      break;
    }
  }

  static void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const tarch::la::Vector<Dimensions, double>& faceCentre,
    const tarch::la::Vector<Dimensions, double>& volumeH,
    double                                       t,
    double                                       dt,
    int                                          normal,
    int                                          numberOfUnknowns,
    int                                          numberOfAuxiliaryVariables,
    double* __restrict__ BTimesDeltaQ
  ) {
    static constexpr double g = 9.81; // [m/s^2]
    switch (normal) {
    case 0: // x
      BTimesDeltaQ[0] = 0.0;
      BTimesDeltaQ[1] = g * Q[0] * deltaQ[3];
      BTimesDeltaQ[2] = 0.0;
      break;
    case 1: // y
      BTimesDeltaQ[0] = 0.0;
      BTimesDeltaQ[1] = 0.0;
      BTimesDeltaQ[2] = g * Q[0] * deltaQ[3];
      break;
    }
  }
} // namespace exahype2::swe
