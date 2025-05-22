# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

eigenvalues = r"""
  L[s::h]  = 0.0;
  L[s::hu] = 0.0;
  L[s::hv] = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return 0.0;
  }

  const double u = Q[s::hu + normal] / Q[s::h];
  const double c = std::sqrt(GRAV * Q[s::h]);
  L[s::h]        = u;
  L[s::hu]       = u + c;
  L[s::hv]       = u - c;
"""

max_eigenvalue = r"""
  double L[3];
  {eigenvalues}
  return std::fmax(std::fabs(L[0]), std::fmax(std::fabs(L[1]), std::fabs(L[2])));
""".format(eigenvalues=eigenvalues)

flux = r"""
  F[s::h]  = 0.0;
  F[s::hu] = 0.0;
  F[s::hv] = 0.0;
  F[s::b] = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return;
  }

  const double ih = 1.0 / Q[s::h];

  F[s::h]  = Q[s::hu + normal];
  F[s::hu] = Q[s::hu + normal] * Q[s::hu] * ih;
  F[s::hv] = Q[s::hu + normal] * Q[s::hv] * ih;
  F[s::hu + normal] += 0.5 * GRAV * Q[s::h] * Q[s::h];
"""

nonconservative_product = r"""
  BTimesDeltaQ[s::h] = 0.0;
  BTimesDeltaQ[s::hu] = 0.0;
  BTimesDeltaQ[s::hv] = 0.0;
  BTimesDeltaQ[s::b] = 0.0;

  constexpr double DRY_TOL = std::numeric_limits<double>::epsilon();
  if (Q[s::h] <= DRY_TOL) {
    return;
  }

  BTimesDeltaQ[s::hu + normal] = GRAV * Q[s::h] * (deltaQ[s::h] + deltaQ[s::b]);
"""
