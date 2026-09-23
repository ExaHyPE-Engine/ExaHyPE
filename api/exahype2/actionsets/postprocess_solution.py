# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

from exahype2.codegen.helper import KeepUndefined
from exahype2.codegen.helper import render_jinja2_template

from .action_set import ActionSet
from .solver_action_set import SolverActionSet


class PostprocessSolution(SolverActionSet):
    """Runs user code once per cell, once per degree of freedom or once per face after the
    solver left the cell or face for the last time in a sweep.

    A degree-of-freedom kernel reads ``value``, ``x``, ``h``, ``weight`` and ``index``.
    """

    _TEMPLATE_GUARDED_KERNEL = """
  if ({{PREDICATE}}) {
    {{KERNEL | indent(4)}}
  }
"""

    def __init__(self, solver):
        super().__init__(solver)
        self.guard = "true"
        self._cell_kernels = []
        self._dof_kernels = []
        self._face_kernels = []

    def add_cell_kernel(self, code):
        self._cell_kernels.append(code)
        return self

    def add_dof_kernel(self, code):
        self._dof_kernels.append(code)
        return self

    def add_face_kernel(self, code):
        self._face_kernels.append(code)
        return self

    def has_kernels(self):
        return bool(
            self._cell_kernels
            or self._dof_kernels
            or self._face_kernels
            or self._solver.error_measurement_kernels()
        )

    def _guarded(self, kernels):
        """Returns the kernels one after the other, each without its own indentation."""
        kernels = [textwrap.dedent(kernel).strip() for kernel in kernels]
        if self.guard == "true":
            return "\n".join(self.render(kernel) for kernel in kernels)
        return "\n".join(
            textwrap.dedent(
                self.render(
                    render_jinja2_template(
                        self._TEMPLATE_GUARDED_KERNEL, {"KERNEL": kernel}, KeepUndefined
                    ),
                    PREDICATE=self.guard,
                )
            ).strip()
            for kernel in kernels
        )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            layout = self._solver.layout
            kernels = (
                list(self._cell_kernels) + self._solver.error_measurement_kernels()
            )
            kernels += [layout.for_each_dof(kernel) for kernel in self._dof_kernels]
            return self._guarded(kernels)
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
            return self._guarded(self._face_kernels)
        return ""
