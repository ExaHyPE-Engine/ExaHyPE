# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Equation:
    """A term hook returns ``True`` for a function the project defines, a C++ body or ``None``."""

    dimensions = 2
    num_unknowns = 0
    num_auxiliary_variables = 0
    is_linear = False
    use_diffusive_flux = False

    @staticmethod
    def eigenvalues():
        return True

    @staticmethod
    def flux():
        return None

    @staticmethod
    def ncp():
        return None

    @staticmethod
    def source():
        return None

    @staticmethod
    def riemann_solver():
        return None
