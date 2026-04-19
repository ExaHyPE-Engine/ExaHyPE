# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .equation import Equation


class Advection(Equation):
    def __init__(self, dimensions, adv_speed=1.0):
        self.dimensions = dimensions
        self.num_unknowns = {"v": dimensions}
        self.num_auxiliary_variables = 0
        self.adv_speed = adv_speed
        self.is_linear = True

    def eigenvalues(self):
        return (
            """
  constexpr double v = """
            + str(self.adv_speed)
            + """;
  return v;
"""
        )

    def flux(self):
        return (
            """
    F[Shortcuts::v + 0] = 0;
    F[Shortcuts::v + 1] = 0;
#if DIMENSIONS == 3
    F[Shortcuts::v + 2] = 0;
#endif

    F[Shortcuts::v + normal] = """
            + str(self.adv_speed)
            + """ * Q[normal];
"""
        )
