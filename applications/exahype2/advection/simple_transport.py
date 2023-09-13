import sys

from exahype2.solvers.fv.PDETerms import PDETerms


class SimpleTransport:
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        self.use_symbolic_description = use_symbolic_description
        if use_symbolic_description:
          sys.path.append('../../../tutorials/exahype2/advection')
          import sympy
          from pde import PDE
          self.pde = PDE(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

          self.pde.initial_values[0] = self.pde.x[0]
          self.pde.initial_values[1] = self.pde.x[1]
          if dimensions == 3:
            self.pde.initial_values[2] = self.pde.x[2]

          self.pde.substitute_expression(self.pde.c, 1)


    def implementation_of_initial_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_initial_conditions()
        else:
            return """
            for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
              Q[i] = volumeCentre[i];
            }
            """


    def implementation_of_boundary_conditions(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_homogeneous_Neumann_BC()
        else:
            return """
            ::exahype2::advection::neumannBoundaryConditions(Qinside, Qoutside, faceCentre, t, normal, NumberOfUnknowns, NumberOfAuxiliaryVariables);
            """


    def implementation_of_flux(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_flux()
        else:
            return """
            ::exahype2::advection::flux(Q, faceCentre, t, dt, normal, F);
            """


    def implementation_of_max_eigenvalue(self):
        if self.use_symbolic_description:
            return self.pde.implementation_of_max_eigenvalue()
        else:
            return """
            return ::exahype2::advection::maxEigenvalue(Q, faceCentre, t, dt, normal, 1.0);
            """


    def implementation_of_sources(self):
        return PDETerms.None_Implementation


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        if (volumeCentre(0) > 0.5) {
          result = ::exahype2::RefinementCommand::Refine;
        } else {
          result = ::exahype2::RefinementCommand::Erase;
        }

        return result;
        """
