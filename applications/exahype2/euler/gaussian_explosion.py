import sys
sys.path.append('../../../tutorials/exahype2/euler')

import pde
import sympy

from exahype2.solvers.fv.PDETerms import PDETerms

class GaussianExplosion(pde.PDE):
    def __init__(self, unknowns, auxiliary_variables, dimensions):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions)
        self.substitute_expression(self.gamma, 1.4)


    def implementation_of_initial_conditions(self):
        return """
        // Manual offset to make the wave originate slightly to the left of the center --- helps
        // to detect if wave is moving to the left or right.
        #if Dimensions==2
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.3};
        #else
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.18, 0.3, 0.6};
        #endif

        const double peakDeviation = MaxAdmissibleCellH;
        double distance = tarch::la::norm2(volumeCentre - circleCentre);
        double exponent = - (distance * distance) / 2.0 / peakDeviation / peakDeviation;

        Q[0] = 0.1; // rho
        Q[1] = 0; // u
        Q[2] = 0; // v

        #if Dimensions == 2
        Q[3] = std::exp(exponent); // e
        #else
        Q[3] = 0; // w
        Q[4] = std::exp(exponent); // e
        #endif
        """


    def implementation_of_boundary_conditions(self):
        return self.implementation_of_homogeneous_Neumann_BC()


    def implementation_of_sources(self):
        return PDETerms.None_Implementation


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        #if Dimensions==2
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.3};
        #else
        const tarch::la::Vector<Dimensions, double> circleCentre = {0.18, 0.3, 0.6};
        #endif

        if (tarch::la::equals(t, 0.0)) {
          bool isInTheCentre = (tarch::la::norm2(volumeCentre - circleCentre) < 0.05);
          if (isInTheCentre) {
            result = ::exahype2::RefinementCommand::Refine;
          }
        }

        return result;
        """
