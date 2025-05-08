# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

max_eigenvalue = r"""
  const auto irho = 1.0 / Q[s::rho];
  const auto p = (GAMMA - 1.0) * (Q[s::rhoE] - 0.5 * irho * (Q[s::rhoU + 0] * Q[s::rhoU + 0] + Q[s::rhoU + 1] * Q[s::rhoU + 1]
#if Dimensions == 3
    + Q[s::rhoU + 2] * Q[s::rhoU + 2]
#endif
  ));

  const auto speedOfSound = sqrt(GAMMA * p * irho);
  const auto u_n          = Q[normal + 1] * irho;
  const auto result       = std::fmax(std::fabs(u_n - speedOfSound), std::fabs(u_n + speedOfSound));

  return result;
"""

flux = r"""
  const auto irho = 1.0 / Q[s::rho];
  const auto p = (GAMMA - 1.0) * (Q[s::rhoE] - 0.5 * irho * (Q[s::rhoU + 0] * Q[s::rhoU + 0] + Q[s::rhoU + 1] * Q[s::rhoU + 1]
#if Dimensions == 3
    + Q[s::rhoU + 2] * Q[s::rhoU + 2]
#endif
  ));

  F[s::rho]         = Q[normal + 1];
  F[s::rhoU + 0]    = Q[normal + 1] * Q[s::rhoU + 0] * irho;
  F[s::rhoU + 1]    = Q[normal + 1] * Q[s::rhoU + 1] * irho;
#if Dimensions == 3
  F[s::rhoU + 2]    = Q[normal + 1] * Q[s::rhoU + 2] * irho;
#endif
  F[s::rhoE]        = Q[normal + 1] * (Q[s::rhoE] + p) * irho;

  F[normal + 1] += p;
"""
