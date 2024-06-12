// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Landslide.h"

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "VariableShortcuts.h"

using s = applications::exahype2::landslide::VariableShortcuts;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
double applications::exahype2::landslide::maxEigenvalue(
  const double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal
) {
#ifdef GPUOffloadingOff
  assertion(normal >= 0);
  assertion(normal < Dimensions);
#endif

  if (tarch::la::greater(Q[s::h], 0.0)) {
    assertion(Q[s::h] > 0.0);
    double       result = 1.0;
    const double ih     = 1.0 / Q[s::h];
    const double c      = std::sqrt(g * std::cos(Zeta) * Q[s::h]);
    double       u      = 0.0;

    switch (normal) {
    case 0: // x
      u      = ih * Q[s::hu + 0];
      result = std::max(std::abs(u - c), std::abs(u + c));
      break;
    case 1: // y
      u      = ih * Q[s::hu + 1];
      result = std::max(std::abs(u - c), std::abs(u + c));
      break;
    }
    return result;
  }

  return 0.0;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::landslide::flux(
  const double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal,
  double* __restrict__ F
) {
#ifdef GPUOffloadingOff
  assertion(normal >= 0);
  assertion(normal < Dimensions);
#endif

  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double ih = 1.0 / Q[s::h];
    switch (normal) {
    case 0:
      F[s::h]      = Q[s::hu + 0];
      F[s::hu + 0] = ih * Q[s::hu + 0] * Q[s::hu + 0]
                     + 0.5 * g * std::cos(Zeta) * Q[s::h] * Q[s::h];
      F[s::hu + 1] = ih * Q[s::hu + 0] * Q[s::hu + 1];
      break;
    case 1:
      F[s::h]      = Q[s::hu + 1];
      F[s::hu + 0] = ih * Q[s::hu + 1] * Q[s::hu + 0];
      F[s::hu + 1] = ih * Q[s::hu + 1] * Q[s::hu + 1]
                     + 0.5 * g * std::cos(Zeta) * Q[s::h] * Q[s::h];
      break;
    }
  } else {
    switch (normal) {
    case 0:
      F[s::h]      = 0.0;
      F[s::hu + 0] = 0.0;
      F[s::hu + 1] = 0.0;
      break;
    case 1:
      F[s::h]      = 0.0;
      F[s::hu + 0] = 0.0;
      F[s::hu + 1] = 0.0;
      break;
    }
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::landslide::nonconservativeProduct(
  const double* __restrict__ Q,
  const double* __restrict__ deltaQ,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal,
  double* __restrict__ BTimesDeltaQ
) {
#ifdef GPUOffloadingOff
  assertion(normal >= 0);
  assertion(normal < Dimensions);
  assertion(!std::isnan(Q[s::h]));
  assertion(!std::isnan(Q[s::hu + 0]));
  assertion(!std::isnan(Q[s::hu + 1]));
  assertion(!std::isnan(Q[s::b]));
  assertion(!std::isnan(deltaQ[s::h]));
  assertion(!std::isnan(deltaQ[s::hu + 0]));
  assertion(!std::isnan(deltaQ[s::hu + 1]));
  assertion(!std::isnan(deltaQ[s::b]));
  nonCriticalAssertion(Q[s::h] == Q[s::h]);
  nonCriticalAssertion(Q[s::hu + 0] == Q[s::hu + 0]);
  nonCriticalAssertion(Q[s::hu + 1] == Q[s::hu + 1]);
  nonCriticalAssertion(Q[s::b] == Q[s::b]);
  nonCriticalAssertion(deltaQ[s::h] == deltaQ[s::h]);
  nonCriticalAssertion(deltaQ[s::hu + 0] == deltaQ[s::hu + 0]);
  nonCriticalAssertion(deltaQ[s::hu + 1] == deltaQ[s::hu + 1]);
  nonCriticalAssertion(deltaQ[s::b] == deltaQ[s::b]);
#endif

  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double ih = 1.0 / Q[s::h];

    // Partial derivatives of unknowns
    // Retrieving true value from average value and deltaQ

    // h
    const double hx = (Q[s::hx] - 0.5 * deltaQ[s::hx]) / (1.0 * h(0));
    const double hy = (Q[s::hy] - 0.5 * deltaQ[s::hy]) / (1.0 * h(1));

    // u
    const double ux  = (Q[s::ux] - 0.5 * deltaQ[s::ux]) / (1.0 * h(0));
    const double uy  = (Q[s::uy] - 0.5 * deltaQ[s::uy]) / (1.0 * h(1));
    const double uxx = (Q[s::uxx] - 0.5 * deltaQ[s::uxx]) / (h(0) * h(0));
    const double uyy = (Q[s::uyy] - 0.5 * deltaQ[s::uyy]) / (h(1) * h(1));
    const double uxy = (Q[s::uxy] - 0.5 * deltaQ[s::uxy]) / (4.0 * h(0) * h(1));

    // v
    const double vx  = (Q[s::vx] - 0.5 * deltaQ[s::vx]) / (1.0 * h(0));
    const double vy  = (Q[s::vy] - 0.5 * deltaQ[s::vy]) / (1.0 * h(1));
    const double vxx = (Q[s::vxx] - 0.5 * deltaQ[s::vxx]) / (h(0) * h(0));
    const double vyy = (Q[s::vyy] - 0.5 * deltaQ[s::vyy]) / (h(1) * h(1));
    const double vxy = (Q[s::vxy] - 0.5 * deltaQ[s::vxy]) / (4.0 * h(0) * h(1));

    switch (normal) {
    case 0: // x
      BTimesDeltaQ[s::h]      = -(h(0) * 0.0);
      BTimesDeltaQ[s::hu + 0] = -(
        h(0) * Nu * std::sqrt(Q[s::h]) * (1.5 * hx * ux + Q[s::h] * uxx)
      );
      BTimesDeltaQ[s::hu + 1] = -(
        h(0) * 0.5 * Nu * std::sqrt(Q[s::h])
        * (1.5 * hx * (uy + vx) + Q[s::h] * (uxy + vxx))
      );
      break;
    case 1: // y
      BTimesDeltaQ[s::h]      = -(h(1) * 0.0);
      BTimesDeltaQ[s::hu + 0] = -(
        h(1) * 0.5 * Nu * sqrt(Q[s::h])
        * (1.5 * hy * (uy + vx) + Q[s::h] * (uyy + vxy))
      );
      BTimesDeltaQ[s::hu + 1] = -(
        h(1) * Nu * sqrt(Q[s::h]) * (1.5 * hy * vy + Q[s::h] * vyy)
      );
      break;
    }
  } else {
    switch (normal) {
    case 0: // x
      BTimesDeltaQ[s::h]      = 0.0;
      BTimesDeltaQ[s::hu + 0] = 0.0;
      BTimesDeltaQ[s::hu + 1] = 0.0;
      break;
    case 1: // y
      BTimesDeltaQ[s::h]      = 0.0;
      BTimesDeltaQ[s::hu + 0] = 0.0;
      BTimesDeltaQ[s::hu + 1] = 0.0;
      break;
    }
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::landslide::sourceTerm(
  const double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  double* __restrict__ S
) {
  // Kappa in the friction law is assumed to be 1 and is just initialised as a
  // const double here using Kappa. The user can apply std::pow((Fr/BetaStar),
  // Kappa) for evaluating T here in case the value of Kappa is different
  // from 1.
  if (tarch::la::greater(Q[s::h], 0.0)) {
    const double ih   = 1.0 / Q[s::h];
    const double u    = ih * Q[s::hu + 0];
    const double v    = ih * Q[s::hu + 1];
    const double ubar = std::sqrt(
                          Q[s::hu + 0] * Q[s::hu + 0]
                          + Q[s::hu + 1] * Q[s::hu + 1]
                        )
                        * ih;

    const double     Fr    = ubar / std::sqrt(g * Q[s::h] * std::cos(Zeta));
    constexpr double Hstar = (BetaStar + Gamma) * Hstop / Beta;
    double           Mu    = 1.0;

    if (Fr > BetaStar) {
      const double vstop  = (Q[s::h] * Beta) / (Fr + Gamma);
      const double Mustop = Mu1
                            + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      Mu = Mustop;
    } else if (Fr > 0.0 && Fr <= BetaStar) {
      const double vstart  = Q[s::h];
      const double vstop   = Q[s::h] * Beta / (BetaStar + Gamma);
      const double Mustart = Mu3
                             + (Mu1 - Mu2
                               ) / (1.0 + vstart / FrictionLengthScale);
      const double Mustop = Mu1
                            + (Mu1 - Mu2) / (1.0 + vstop / FrictionLengthScale);
      const double T = Mustart
                       + (Mustop - Mustart) * std::pow((Fr / BetaStar), Kappa);
      Mu = T;
    } else if (tarch::la::equals(Fr, 0.0)) {
      const double vstart  = Q[s::h];
      const double Mustart = Mu3
                             + (Mu1 - Mu2
                               ) / (1.0 + vstart / FrictionLengthScale);
      Mu = Mustart;
    }

    if (tarch::la::equals(ubar, 0.0)) {
      const double hx = Q[s::hx] / h(0);
      const double hy = Q[s::hy] / h(1);
      const tarch::la::Vector<Dimensions, double>
                   frictionVector          = {std::tan(Zeta) - hx, -hy};
      const double frictionVectorMagnitude = tarch::la::norm2(frictionVector);
      if (tarch::la::greater(Mu, frictionVectorMagnitude)) {
        Mu = frictionVectorMagnitude;
      }

      if (tarch::la::equals(Mu, 0.0)) {
        S[s::h]      = 0.0;
        S[s::hu + 0] = Q[s::h] * g * std::sin(Zeta);
        S[s::hu + 1] = 0.0;
      } else {
        S[s::h]      = 0.0;
        S[s::hu + 0] = Q[s::h] * g * std::sin(Zeta)
                       - Mu * (frictionVector(0) / frictionVectorMagnitude)
                           * Q[s::h] * g * std::cos(Zeta);
        S[s::hu + 1] = -Mu * (frictionVector(1) / frictionVectorMagnitude)
                       * Q[s::h] * g * std::cos(Zeta);
      }
    } else if (tarch::la::greater(ubar, 0.0)) {
      S[s::h]      = 0.0;
      S[s::hu + 0] = Q[s::h] * g * std::sin(Zeta)
                     - Mu * (u / ubar) * Q[s::h] * g * std::cos(Zeta);
      S[s::hu + 1] = -Mu * (v / ubar) * Q[s::h] * g * std::cos(Zeta);
    }
  } else {
    S[s::h]      = 0.0;
    S[s::hu + 0] = 0.0;
    S[s::hu + 1] = 0.0;
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
