# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms


class Equation:
    dimensions = 2
    num_unknowns = 0
    num_auxiliary_variables = 0
    is_linear = False

    @staticmethod
    def eigenvalues():
        return PDETerms.User_Defined_Implementation

    @staticmethod
    def flux():
        return PDETerms.None_Implementation

    @staticmethod
    def ncp():
        return PDETerms.None_Implementation
