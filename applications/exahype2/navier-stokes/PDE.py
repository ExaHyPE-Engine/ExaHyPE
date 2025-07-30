# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[Shortcuts::rho];

  const auto u0 = Q[Shortcuts::rhoU + 0] * irho;
  const auto u1 = Q[Shortcuts::rhoU + 1] * irho;
#if Dimensions == 3
  const auto u2 = Q[Shortcuts::rhoU + 2] * irho;
  const auto u2sq = u2 * u2;
#else
  const auto u2sq = 0.0;
#endif

#if Dimensions == 3
  const auto u_n = (normal == 0) ? u0 : (normal == 1) ? u1 : u2;
#else
  const auto u_n = (normal == 0) ? u0 : u1;
#endif

  const auto uSq = u0 * u0 + u1 * u1 + u2sq;
  const auto internalE = Q[Shortcuts::rhoE] - 0.5 * Q[Shortcuts::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  const auto result = std::fmax(std::fabs(u_n - speedOfSound), std::fabs(u_n + speedOfSound));
  return result;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

flux = r"""
  {compute_primitive_variables}

  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];

  F[Shortcuts::rhoU + 0] = Q[Shortcuts::rhoU + 0] * u_n;
  F[Shortcuts::rhoU + 1] = Q[Shortcuts::rhoU + 1] * u_n;
#if Dimensions == 3
  F[Shortcuts::rhoU + 2] = Q[Shortcuts::rhoU + 2] * u_n;
#endif

  F[Shortcuts::rhoU + normal] += p;

  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + p) * u_n;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

source_term = r"""
  S[Shortcuts::rho]       = 0.0;
  S[Shortcuts::rhoU + 0]  = 0.0;
  S[Shortcuts::rhoU + 1]  = -Q[Shortcuts::rho] * GRAVITY;
#if Dimensions == 3
  S[Shortcuts::rhoU + 2]  = 0.0;
#endif
  S[Shortcuts::rhoE]      = -Q[Shortcuts::rhoU + 1] * GRAVITY;
"""
