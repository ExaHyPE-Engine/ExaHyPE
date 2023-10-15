import sys

from exahype2.solvers.PDETerms import PDETerms

from point_explosion import PointExplosion


class PointExplosionWithDynamicAMR(PointExplosion):
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions, use_symbolic_description=use_symbolic_description)


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;
        if (tarch::la::greater(t, 0.0)) {
          const double stress = Q[4];
          if (stress >= 5) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if (stress < 5) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }
        return result;
        """
