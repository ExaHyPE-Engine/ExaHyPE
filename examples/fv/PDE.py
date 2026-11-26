# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q.rho;
  const auto u0 = Q.rhoU[0] * irho;
  const auto u1 = Q.rhoU[1] * irho;
#if DIMENSIONS == 3
  const auto u2 = Q.rhoU[2] * irho;
#endif

#if DIMENSIONS == 3
  const auto uSq = tarch::la::sumOfSquaresInOrderOfMagnitude(u0, u1, u2);
  const auto u_n = (normal == 0) ? u0 : (normal == 1) ? u1 : u2;
#else
  const auto squareOfU0 = tarch::la::productRoundedOnce(u0, u0);
  const auto squareOfU1 = tarch::la::productRoundedOnce(u1, u1);
  const auto uSq        = squareOfU0 + squareOfU1;
  const auto u_n = (normal == 0) ? u0 : u1;
#endif

  const auto internalE = Q.rhoE - 0.5 * Q.rho * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = sqrt(GAMMA * p * irho);
  auto result = fmax(0.0, fabs(u_n - speedOfSound));
  result = fmax(result, fabs(u_n + speedOfSound));
  return result;
""".format(compute_primitive_variables=compute_primitive_variables)

flux = r"""
  {compute_primitive_variables}

  F.rho  = Q.rhoU[normal];
  F.rhoU = Q.rhoU * u_n;
  F.rhoU[normal] += p;
  F.rhoE = (Q.rhoE + p) * u_n;
""".format(compute_primitive_variables=compute_primitive_variables)

source_term = r"""
  S         = {};
  S.rhoU[1] = -Q.rho * GRAVITY;
  S.rhoE    = -Q.rhoU[1] * GRAVITY;
"""
