# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class Euler(Equation):
    def __init__(self, dimensions, gamma=1.4):
        self.dimensions = dimensions
        self.num_unknowns = {"rho": 1, "E": 1, "v": dimensions}
        self.num_auxiliary_variables = 0
        self.gamma = gamma

    def eigenvalues(self):
        return (
            """
  const auto irho = 1 / Q[Shortcuts::rho];
  constexpr auto Gamma = """
            + str(self.gamma)
            + """;
  const auto p = (Gamma - 1) * (Q[Shortcuts::E] - 0.5 * irho * """
            + (
                """(Q[Shortcuts::v+0]*Q[Shortcuts::v+0]+Q[Shortcuts::v+1]*Q[Shortcuts::v+1]+Q[Shortcuts::v+2]*Q[Shortcuts::v+2]));"""
                if self.dimensions == 3
                else """ (Q[Shortcuts::v+0]*Q[Shortcuts::v+0]+Q[Shortcuts::v+1]*Q[Shortcuts::v+1]));"""
            )
            + """

  const auto c = std::sqrt(Gamma * p * irho);
  const auto u = Q[Shortcuts::v + normal] * irho;

  return std::fmax(std::abs(u-c), std::abs(u+c));
"""
        )

    def flux(self):
        return (
            """
  const auto irho = 1 / Q[Shortcuts::rho];
  constexpr auto Gamma = """
            + str(self.gamma)
            + """;
  const auto p = (Gamma - 1) * (Q[Shortcuts::E] - 0.5 * irho * """
            + (
                """(Q[Shortcuts::v+0]*Q[Shortcuts::v+0]+Q[Shortcuts::v+1]*Q[Shortcuts::v+1]+Q[Shortcuts::v+2]*Q[Shortcuts::v+2]));"""
                if self.dimensions == 3
                else """ (Q[Shortcuts::v+0]*Q[Shortcuts::v+0]+Q[Shortcuts::v+1]*Q[Shortcuts::v+1]));"""
            )
            + """

  F[Shortcuts::rho] = Q[Shortcuts::v + normal];
  F[Shortcuts::v+0] = Q[Shortcuts::v + normal] * Q[Shortcuts::v+0] * irho;
  F[Shortcuts::v+1] = Q[Shortcuts::v + normal] * Q[Shortcuts::v+1] * irho;"""
            + (
                """
  F[Shortcuts::v+2] = Q[Shortcuts::v + normal] * Q[Shortcuts::v+2] * irho; """
                if self.dimensions == 3
                else ""
            )
            + """
  F[Shortcuts::E]     = Q[Shortcuts::v + normal] * irho * (Q[Shortcuts::E] + p);

  F[Shortcuts::v + normal] += p;
"""
        )
