# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms


class Scenario:
    _equation = None
    _dimensions = 2
    _end_time = 1.0
    _offset = 0.0
    _domain_size = 1.0
    _periodic_bc = False

    def initial_conditions(self):
        return PDETerms.User_Defined_Implementation

    def boundary_conditions(self):
        return (
            "assert(false);"
            if self._periodic_bc
            else PDETerms.User_Defined_Implementation
        )

    def refinement_criterion(self):
        return PDETerms.None_Implementation

    def analytical_solution(self):
        return PDETerms.None_Implementation
