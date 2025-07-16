# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[s::rho];

  const auto u0 = Q[s::rhoU + 0] * irho;
  const auto u1 = Q[s::rhoU + 1] * irho;
#if Dimensions == 3
  const auto u2 = Q[s::rhoU + 2] * irho;
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
  const auto internalE = Q[s::rhoE] - 0.5 * Q[s::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  const auto result = std::fmax(std::fabs(u_n - speedOfSound), std::fabs(u_n + speedOfSound));
  return result;
""".format(compute_primitive_variables=compute_primitive_variables)

flux = r"""
  {compute_primitive_variables}

  F[s::rho] = Q[s::rhoU + normal];

  F[s::rhoU + 0] = Q[s::rhoU + 0] * u_n;
  F[s::rhoU + 1] = Q[s::rhoU + 1] * u_n;
#if Dimensions == 3
  F[s::rhoU + 2] = Q[s::rhoU + 2] * u_n;
#endif

  F[s::rhoU + normal] += p;

  F[s::rhoE] = (Q[s::rhoE] + p) * u_n;
""".format(compute_primitive_variables=compute_primitive_variables)
