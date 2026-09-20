# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from exahype2.codegen.helper import KeepUndefined
from exahype2.codegen.helper import render_jinja2_template

from .action_set import ActionSet
from .solver_action_set import SolverActionSet


class InitialCondition(SolverActionSet):
    """Evaluates the solver's initial condition on every degree of freedom of a cell the guard
    admits and marks the cell as updated; ``epilogue`` is C++ the family runs afterwards.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{PREDICATE}}) {
    {{INITIALISATION | indent(4)}}
{%- if EPILOGUE %}
    {{EPILOGUE | indent(4)}}
{%- endif %}
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  }
"""

    def __init__(self, solver, guard, epilogue=""):
        super().__init__(solver)
        self.guard = guard
        self._epilogue = epilogue

    def operation_body(self, operation_name):
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return ""
        layout = self._solver.layout
        if layout.initialises_patches:
            initialisation = (
                "{{FULL_QUALIFIED_SOLVER_NAME}}::initialCondition("
                + layout.solution()
                + ".data(), marker.getX(), marker.getH());"
            )
        else:
            initialisation = layout.for_each_dof(
                "{{FULL_QUALIFIED_SOLVER_NAME}}::initialCondition(value, x, h);"
            )
        template = render_jinja2_template(
            self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            {
                "INITIALISATION": textwrap.dedent(initialisation).strip(),
                "EPILOGUE": textwrap.dedent(self._epilogue).strip(),
            },
            KeepUndefined,
        )
        return self.render(template, PREDICATE=self.guard)
