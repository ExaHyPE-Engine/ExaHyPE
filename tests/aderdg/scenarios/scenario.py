# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org


class Scenario:
    """A term hook returns ``True`` for a function the project defines, a C++ body or ``None``."""

    _equation = None
    _dimensions = 2
    _end_time = 1.0
    _offset = 0.0
    _domain_size = 1.0
    _periodic_bc = False

    def initial_conditions(self):
        return True

    def boundary_conditions(self):
        return "assertion(false);" if self._periodic_bc else True

    def refinement_criterion(self):
        return None

    def analytical_solution(self):
        return None
