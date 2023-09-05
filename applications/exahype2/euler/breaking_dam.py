import sys

from exahype2.solvers.fv.PDETerms import PDETerms


class BreakingDam:
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        self.use_symbolic_description = use_symbolic_description
        if use_symbolic_description:
          sys.path.append('../../../tutorials/exahype2/euler')
          import sympy
          from pde import PDE
          self.pde = PDE(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

          self.pde.initial_values[0] = 0.1 # rho
          self.pde.initial_values[1] = 0 # u
          self.pde.initial_values[2] = 0 # v

          if dimensions == 2:
            self.pde.initial_values[3] = sympy.Piecewise((1.0, self.pde.x[0] < 0.5), (0, True)) # e
          else:
            self.pde.initial_values[3] = 0 # w
            self.pde.initial_values[4] = sympy.Piecewise((1.0, self.pde.x[0] < 0.5), (0, True)) # e

          self.pde.substitute_expression(self.pde.gamma, 1.4)


    def implementation_of_initial_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_initial_conditions()
        else:
            return """
            Q[0] = 0.1;
            Q[1] = 0;
            Q[2] = 0;
            #if Dimensions == 2
            Q[3] = ((volumeCentre(0) < 0.5) ? (1.0) : (0));
            #else
            Q[3] = 0;
            Q[4] = ((volumeCentre(0) < 0.5) ? (1.0) : (0));
            #endif
            """


    def implementation_of_boundary_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_homogeneous_Neumann_BC()
        else:
            return """
            ::exahype2::euler::neumannBoundaryConditions(Qinside, Qoutside, faceCentre, t, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
            """


    def implementation_of_flux(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_flux()
        else:
            return """
            ::exahype2::euler::flux(Q, faceCentre, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables, F);
            """


    def implementation_of_max_eigenvalue(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_max_eigenvalue()
        else:
            return """
            return ::exahype2::euler::maxEigenvalue(Q, faceCentre, t, dt, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
            """


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
