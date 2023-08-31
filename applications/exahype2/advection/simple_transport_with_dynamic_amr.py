import sys

from exahype2.solvers.fv.PDETerms import PDETerms

from simple_transport import SimpleTransport


class SimpleTransportWithDynamicAMR(SimpleTransport):
    def __init__(self, unknowns, auxiliary_variables, dimensions, use_symbolic_description):
        super().__init__(unknowns=unknowns, auxiliary_variables=auxiliary_variables, dimensions=dimensions, use_symbolic_description=use_symbolic_description)


    def implementation_of_refinement_criterion(self):
        return """
        auto result = ::exahype2::RefinementCommand::Keep;

        if (tarch::la::greater(t, 0.0)) {
          const double u = Q[0];

          if (u >= 0.5) {
            result = ::exahype2::RefinementCommand::Refine;
          }
          if (u < 0.5) {
            result = ::exahype2::RefinementCommand::Erase;
          }
        }

        return result;
        """
