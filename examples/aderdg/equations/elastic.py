# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2

from .equation import Equation


class Elastic(Equation):
    def __init__(self, dimensions):
        self.dimensions = dimensions
        self.num_unknowns = {
            "v": dimensions,
            "sigma": exahype2.SymmetricMatrix(dimensions),
        }
        self.num_auxiliary_variables = {"rho": 1, "cp": 1, "cs": 1}
        self.is_linear = True

    def eigenvalues(self):
        return """
  return std::fmax(
    std::abs(Q.cp),
    std::abs(Q.cs)
  );
"""

    def flux(self):
        return """
  // Lamé parameters
  const auto iRho   = 1/Q.rho;
  const auto Mu     = Q.rho * Q.cs * Q.cs;
  const auto Lambda = Q.rho * Q.cp * Q.cp - 2 * Mu;

  // The flux of the velocity is the stress column of the normal axis.
  F = {};
  for (int i = 0; i < DIMENSIONS; i++) {
    F.v(i)        = -iRho * Q.sigma(i, normal);
    F.sigma(i, i) = -Lambda * Q.v(normal);
    if (i != normal) {
      F.sigma(i, normal) = -Mu * Q.v(i);
    }
  }
  F.sigma(normal, normal) = -(2 * Mu + Lambda) * Q.v(normal);
"""
