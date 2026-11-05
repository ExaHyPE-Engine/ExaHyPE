# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class Euler(Equation):
    def __init__(self, dimensions, gamma=1.4):
        self.dimensions = dimensions
        self.num_unknowns = {"rho": 1, "E": 1, "v": dimensions}
        self.num_auxiliary_variables = 0
        self.gamma = gamma

    def _pressure(self):
        """Returns the pressure. The squared momentum has equal bits for every order of the axes."""
        return (
            """
  const auto irho = 1 / Q.rho;
  constexpr auto Gamma = """
            + str(self.gamma)
            + """;
  const auto p = (Gamma - 1) * (Q.E - 0.5 * irho * tarch::la::norm2Squared(Q.v));
"""
        )

    def eigenvalues(self):
        return self._pressure() + """
  const auto c = std::sqrt(Gamma * p * irho);
  const auto u = Q.v(normal) * irho;

  return std::fmax(std::abs(u-c), std::abs(u+c));
"""

    def flux(self):
        return self._pressure() + """
  F.rho = Q.v(normal);
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    F.v(axis) = Q.v(normal) * Q.v(axis) * irho;
  }
  F.E = Q.v(normal) * irho * (Q.E + p);

  F.v(normal) += p;
"""
