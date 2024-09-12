// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SWE.h"

#include "Constants.h"
#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "VariableShortcuts.h"

using s = applications::exahype2::swe::VariableShortcuts;

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::swe::eigenvalues(
  const double* __restrict__ Q,
  const ::tarch::la::Vector<Dimensions, double>& x,
  const ::tarch::la::Vector<Dimensions, double>& h,
  double                                         t,
  double                                         dt,
  int                                            normal,
  double* __restrict__ L
) {
#ifdef GPUOffloadingOff
  assertion(normal >= 0);
  assertion(normal < Dimensions);
  assertion(!std::isnan(Q[s::h]));
  assertion(!std::isnan(Q[s::hu + 0]));
  assertion(!std::isnan(Q[s::hu + 1]));
  assertion(!std::isnan(Q[s::b]));
  nonCriticalAssertion(Q[s::h] == Q[s::h]);
  nonCriticalAssertion(Q[s::hu + 0] == Q[s::hu + 0]);
  nonCriticalAssertion(Q[s::hu + 1] == Q[s::hu + 1]);
  nonCriticalAssertion(Q[s::b] == Q[s::b]);
#endif

  const double u = Q[s::h] > 0.0 ? Q[s::hu + normal] / Q[s::h] : 0.0;
  const double c = Q[s::h] > 0.0 ? std::sqrt(g * Q[s::h]) : 0.0;
  L[0]           = u;
  L[1]           = u + c;
  L[2]           = u - c;
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::swe::flux(
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
  assertion(!std::isnan(Q[s::h]));
  assertion(!std::isnan(Q[s::hu + 0]));
  assertion(!std::isnan(Q[s::hu + 1]));
  assertion(!std::isnan(Q[s::b]));
  nonCriticalAssertion(Q[s::h] == Q[s::h]);
  nonCriticalAssertion(Q[s::hu + 0] == Q[s::hu + 0]);
  nonCriticalAssertion(Q[s::hu + 1] == Q[s::hu + 1]);
  nonCriticalAssertion(Q[s::b] == Q[s::b]);
#endif

  const double ih = Q[s::h] > 0.0 ? 1.0 / Q[s::h] : 0.0;
  F[s::h]         = Q[s::hu + normal];
  F[s::hu + 0]    = ih * Q[s::hu + normal] * Q[s::hu + 0];
  F[s::hu + 1]    = ih * Q[s::hu + normal] * Q[s::hu + 1];
  F[s::hu + normal] += 0.5 * 9.81 * Q[s::h] * Q[s::h]; // TODO: Should be
                                                       // removed as ncp takes
                                                       // care of that.
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
void applications::exahype2::swe::nonconservativeProduct(
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

  BTimesDeltaQ[s::h] = 0.0;

  switch (normal) {
  case 0: // x
    BTimesDeltaQ[s::hu + 0] = g * Q[s::h] * deltaQ[s::b];
    BTimesDeltaQ[s::hu + 1] = 0.0;
    break;
  case 1: // y
    BTimesDeltaQ[s::hu + 0] = 0.0;
    BTimesDeltaQ[s::hu + 1] = g * Q[s::h] * deltaQ[s::b];
    break;
  }
}
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
