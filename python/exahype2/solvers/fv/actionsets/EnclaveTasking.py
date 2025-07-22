# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import jinja2


class EnclaveTasking(AbstractFVActionSet):
    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if ({{GUARD}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    {{COMPUTE_TIME_STEP_SIZE}}
    assertion2(tarch::la::greaterEquals(timeStamp,    0.0), timeStamp, timeStepSize);
    assertion2(tarch::la::greaterEquals(timeStepSize, 0.0), timeStamp, timeStepSize);

    auto* /*const*/ QIn = {{OLD_ACCESSOR}}.value;
    auto* /*const*/ QOut = {{NEW_ACCESSOR}}.value;

    {{STORAGE_PRECISION}}* faces[1][TwoTimesD];
    for (int d = 0; d < TwoTimesD; ++d) {
      faces[0][d] = {{FACE_ACCESSOR}}(d).value;
    }

    {% if CAN_FUSE %}
    ::exahype2::EnclaveBookkeeping::getInstance().bookmarkTask(new tasks::{{ENCLAVE_TASK_TYPE}}(
      marker,
      timeStamp,
      timeStepSize,
      QIn,
      faces,
      QOut
    ));
    {% else %}
    tarch::multicore::spawnTask(new tasks::{{ENCLAVE_TASK_TYPE}}(
      marker,
      timeStamp,
      timeStepSize,
      QIn,
      faces,
      QOut
    ));
    {% endif %}
  }
"""
    )

    def __init__(self, solver):
        super(EnclaveTasking, self).__init__(solver)
        self.guard = "not marker.willBeRefined() and not marker.hasBeenRefined() /*and marker.willBeEnclaveCell()*/"
        self.new_accessor = "fineGridCell" + solver._patch.name
        self.old_accessor = "fineGridCell" + solver._patch_reconstructed.name
        self.face_accessor = "fineGridFaces" + solver._patch_overlap_new.name

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard
        d["NEW_ACCESSOR"] = self.new_accessor
        d["OLD_ACCESSOR"] = self.old_accessor
        d["FACE_ACCESSOR"] = self.face_accessor

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            self._add_action_set_entries_to_dictionary(d)
            result = self._Template_TouchCellFirstTime.render(**d)
        return result

    def get_body_of_unprepareTraversal(self):
        result = ""
        if self._solver._can_fuse:
            result += (
                "::exahype2::EnclaveBookkeeping::getInstance().fusePendingTasks();"
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(EnclaveTasking, self).get_includes()
        result += (
            """
#include "tasks/"""
            + self._solver._name
            + """EnclaveTask.h"
#include "exahype2/EnclaveBookkeeping.h"
"""
        )
        return result
