# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4.solversteps


class EmptyPostprocessSolution(AbstractADERDGActionSet):
    """
    Emits the cell kernel in touchCellLastTime() and the face kernel in
    touchFaceLastTime(); both are empty until a user appends a snippet to
    _cell_compute_kernel or _face_compute_kernel.

    ADERDG.create_action_sets() creates it when no other variant is set, and a
    user replaces it through solver._action_set_postprocess_solution.
    """

    def __init__(self, solver):
        super(EmptyPostprocessSolution, self).__init__(solver)
        self._cell_compute_kernel = ""
        self._face_compute_kernel = ""
        self.descend_invocation_order = 1

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
            and self._cell_compute_kernel != ""
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(
                self._cell_compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
            and self._face_compute_kernel != ""
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            result = jinja2.Template(
                self._face_compute_kernel, undefined=jinja2.DebugUndefined
            ).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
