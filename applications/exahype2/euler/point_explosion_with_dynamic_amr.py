import sys

from exahype2.solvers.fv.PDETerms import PDETerms

from point_explosion import PointExplosion


class PointExplosionWithDynamicAMR(PointExplosion):
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions, use_symbolic_description=use_symbolic_description)


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

        // This is an example how to implement dynamic AMR, as the
        // AMR instruction depends on the actual solution (which is
        // not directly available throughout the grid construction).
        // If you remove this part, you get static AMR.
        if (tarch::la::greater(t, 0.0)) {
          #if Dimensions == 2
          const double energy = Q[3];
          #else
          const double energy = Q[4];
          #endif

          if (energy >= 0.1) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if (energy < 0.1) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }

        return result;
        """
