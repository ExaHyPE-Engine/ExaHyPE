import sys
sys.path.append('../../../tutorials/exahype2/euler')

import pde
import sympy

from exahype2.solvers.fv.PDETerms import PDETerms

class BreakingDam(pde.PDE):
    def __init__(self, unknowns, auxiliary_variables, dimensions):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

        self.initial_values[0] = 0.1 # rho
        self.initial_values[1] = 0 # u
        self.initial_values[2] = 0 # v

        if dimensions == 2:
          self.initial_values[3] = sympy.Piecewise((1.0, self.x[0] < 0.5), (0, True)) # e
        else: # 3D
          self.initial_values[3] = 0 # w
          self.initial_values[4] = sympy.Piecewise((1.0, self.x[0] < 0.5), (0, True)) # e

        self.substitute_expression(self.gamma, 1.4)


    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()


    def implementation_of_sources(self):
        return PDETerms.None_Implementation


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        if (volumeCentre(0) < 0.5) {
          result = ::exahype2::RefinementCommand::Refine;
        } else {
          result = ::exahype2::RefinementCommand::Erase;
        }

        return result;
        """
