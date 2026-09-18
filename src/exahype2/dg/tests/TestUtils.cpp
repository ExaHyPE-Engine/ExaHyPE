// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TestUtils.h"

/** Writes the compressible Euler flux along normal into F. */
void exahype2::dg::tests::eulerFlux(
  const double* NOALIAS const                                   Q,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& x,
  [[maybe_unused]] const double                                 t,
  [[maybe_unused]] const double                                 dt,
  const int                                                     normal,
  double* NOALIAS const                                         F
) {
  assertion(Q[0] > 0.0, Q[0], Q[1], Q[2], Q[3], Q[4], x, t, normal);
  assertion(normal >= 0 and normal < 3, normal);

  constexpr double Gamma          = 1.4;
  const double     inverseDensity = 1.0 / Q[0];
  const double     pressure = (Gamma - 1) * (Q[4] - 0.5 * inverseDensity * (Q[1] * Q[1] + Q[2] * Q[2] + Q[3] * Q[3]));

  const double velocity = inverseDensity * Q[normal + 1];
  F[0]                  = velocity * Q[0];
  F[1]                  = velocity * Q[1];
  F[2]                  = velocity * Q[2];
  F[3]                  = velocity * Q[3];
  F[normal + 1] += pressure;
  F[4] = velocity * (Q[4] + pressure);
}
