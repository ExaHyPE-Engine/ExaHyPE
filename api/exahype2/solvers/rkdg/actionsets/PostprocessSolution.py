# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2


class EmptyPostprocessSolution(AbstractRungeKuttaDGActionSet):
    """
    Postprocessing variant that emits no code.

    RungeKuttaDG.create_action_sets() creates it when no other variant is set,
    and a user replaces it through solver._action_set_postprocess_solution.
    """

    def get_body_of_operation(self, operation_name):
        return ""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


class CellWisePostprocessSolution(AbstractRungeKuttaDGActionSet):
    """
    Postprocessing variant that runs a snippet once per cell in touchCellLastTime().
    """

    def __init__(self, solver):
        super(CellWisePostprocessSolution, self).__init__(solver)
        self.guard = "true"
        self._compute_kernel = ""

    def add_postprocessing_kernel(self, operation_per_cell):
        """
        Adds the C++ snippet operation_per_cell, which reads marker, timeStamp
        and timeStepSize.
        """
        self._compute_kernel += (
            """
{
  [[maybe_unused]] const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
  [[maybe_unused]] const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {
    """
            + operation_per_cell
            + """
  }
}
"""
        )

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"] = jinja2.Template(
                self.guard, undefined=jinja2.DebugUndefined
            ).render(**d)
            result = jinja2.Template(
                self._compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
