import sys

from exahype2.solvers.fv.PDETerms import PDETerms

from gaussian_explosion import GaussianExplosion


class GaussianExplosionWithDynamicAMR(GaussianExplosion):
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions, use_symbolic_description=use_symbolic_description)


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

        #if Dimensions == 2
        const double energy = Q[3];
        #else
        const double energy = Q[4];
        #endif

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
