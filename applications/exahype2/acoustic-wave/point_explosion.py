import sys

from exahype2.solvers.fv.PDETerms import PDETerms


class PointExplosion:
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        self.use_symbolic_description = use_symbolic_description
        if use_symbolic_description:
          sys.path.append('../../../tutorials/exahype2/acoustic-wave')
          import sympy
          from pde import PDE
          self.pde = PDE(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

          self.pde.initial_values[0] = 0.1 # rho
          self.pde.initial_values[1] = 0 # u
          self.pde.initial_values[2] = 0 # v

          if dimensions == 2:
            volume_centre = sympy.sqrt((0.5 - self.pde.x[0])**2 + (0.5 - self.pde.x[1])**2)
            self.pde.initial_values[3] = sympy.Piecewise((1.0, volume_centre <= self.pde.h[0]), (0.05, True)) # e
          else:
            volume_centre = sympy.sqrt((0.5 - self.pde.x[0])**2 + (0.5 - self.pde.x[1])**2 + (0.5 - self.pde.x[2])**2)
            self.pde.initial_values[3] = 0 # w
            self.pde.initial_values[4] = sympy.Piecewise((1.0, volume_centre <= self.pde.h[0]), (0.05, True)) # e

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
            Q[3] = ((std::sqrt(std::pow(0.5 - volumeCentre(0), 2) + std::pow(0.5 - volumeCentre(1), 2)) <= volumeH[0]) ? (1.0) : (0.05));
            #else
            Q[3] = 0;
            Q[4] = ((std::sqrt(std::pow(0.5 - volumeCentre(0), 2) + std::pow(0.5 - volumeCentre(1), 2) + std::pow(0.5 - volumeCentre(2), 2)) <= volumeH[0]) ? (1.0) : (0.05));
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

        #if Dimensions==3
        tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
        #else
        tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
        #endif

        // This is an example how to implement a priori refinement.
        // If you only have this thing, then you work with static AMR,
        // as you never invoke erase.
        if (tarch::la::equals(t, 0.0)) {
          bool isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.2);
          if (isInTheCentre) {
            result = ::exahype2::RefinementCommand::Refine;
          }
        }

        return result;
        """
