# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2


class InitialCondition(AbstractRungeKuttaDGActionSet):
    TemplateInitialCondition = """
  if ({{PREDICATE}}) { 
    int linearisedIndex = 0;
    peano4::dForEach({{DG_ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
      repositories::{{SOLVER_INSTANCE}}.initialCondition(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data() + linearisedIndex,
        ::exahype2::dg::getQuadraturePoint(
          marker.getX(), marker.getH(), index, repositories::{{SOLVER_INSTANCE}}.DGOrder, repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
        ),
        marker.getH()
      );
      linearisedIndex += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    });

    // relevant for tracing et al which kicks in if and only if cell has been
    // updated
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  } 
"""

    def __init__(self, solver, guard):
        super(InitialCondition, self).__init__(solver)
        self.guard = guard

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = self.guard
            result = jinja2.Template(self.TemplateInitialCondition).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
