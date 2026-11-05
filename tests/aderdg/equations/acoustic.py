# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class Acoustic(Equation):
    def __init__(self, dimensions, rho=1.0, K0=4.0):
        self.dimensions = dimensions
        self.num_unknowns = {"p": 1, "v": dimensions}
        self.num_auxiliary_variables = 0
        self.rho = rho
        self.K0 = K0
        self.is_linear = True

    def eigenvalues(self):
        return (
            """
  const auto c = std::sqrt("""
            + str(self.K0)
            + "/"
            + str(self.rho)
            + """);
  return c;
"""
        )

    def flux(self):
        return (
            """
  constexpr auto K0      = """
            + str(self.K0)
            + """;
  constexpr auto inv_rho = 1 / """
            + str(self.rho)
            + """;

  F.p         = K0 * Q.v(normal);
  F.v         = 0.0;
  F.v(normal) = inv_rho * Q.p;
"""
        )
