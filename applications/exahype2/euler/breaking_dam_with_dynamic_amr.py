import sys

from exahype2.solvers.fv.PDETerms import PDETerms

from breaking_dam import BreakingDam


class BreakingDamWithDynamicAMR(BreakingDam):
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions, use_symbolic_description=use_symbolic_description)


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        if (tarch::la::equals(t, 0.0) and tarch::la::equals(volumeCentre(0), 1.0/3.0)) {
          #if Dimensions == 2
          const double energy = Q[3];
          #else
          const double energy = Q[4];
          #endif

          result = ::exahype2::RefinementCommand::Refine;
        } else if (t > 0.0 and energy > 0.4) {
          result = ::exahype2::RefinementCommand::Refine;
        } else if (t > 0.0 and energy < 0.2) {
          result = ::exahype2::RefinementCommand::Erase;
        }

        return result;
        """
