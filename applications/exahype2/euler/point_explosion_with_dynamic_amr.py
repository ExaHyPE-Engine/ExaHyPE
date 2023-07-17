import sys
sys.path.append('../../../tutorials/exahype2/euler')

import pde
import sympy

from exahype2.solvers.fv.PDETerms import PDETerms

class PointExplosionWithDynamicAMR(pde.PDE):
    def __init__(self, unknowns, auxiliary_variables, dimensions):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)

        self.initial_values[0] = 0.1 # rho
        self.initial_values[1] = 0 # u
        self.initial_values[2] = 0 # v

        if dimensions == 2:
          volume_centre = sympy.sqrt((0.5 - self.x[0])**2 + (0.5 - self.x[1])**2)
          self.initial_values[3] = sympy.Piecewise((1.0, volume_centre < 0.05), (0, True)) # e
        else: # 3D
          volume_centre = sympy.sqrt((0.5 - self.x[0])**2 + (0.5 - self.x[1])**2 + (0.5 - self.x[2])**2)
          self.initial_values[3] = 0 # w
          self.initial_values[4] = sympy.Piecewise((1.0, volume_centre < 0.05), (0, True)) # e

        self.substitute_expression(self.gamma, 1.4)


    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()


    def implementation_of_sources(self):
        return PDETerms.None_Implementation


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        #if Dimensions==3
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
        #else
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
        #endif

        // Same as for static AMR. This is the initial pattern.
        if (tarch::la::equals(t, 0.0)) {
          bool isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.05);
          if (isInTheCentre) {
            result = ::exahype2::RefinementCommand::Refine;
          }
        }

        #if Dimensions == 2
        const double energy = Q[3];
        #else
        const double energy = Q[4];
        #endif

        // This is an example how to implement dynamic AMR, as the
        // AMR instruction depends on the actual solution (which is
        // not directly available throughout the grid construction).
        // If you remove this part, you get static AMR.
        if (tarch::la::greater(t, 0.0)) {
          if (energy > 0.4) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if (energy < 0.2) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }

        return result;
        """
