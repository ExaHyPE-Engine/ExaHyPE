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

    auto* /*const*/ QIn = {{RECONSTRUCTED_ACCESSOR}}.value;
    auto* /*const*/ QOut = {{CELL_ACCESSOR}}.value;

    {{STORAGE_PRECISION}}* faces[3][TWO_TIMES_D];
    bool                   isBoundary[TWO_TIMES_D];
    for (int d = 0; d < TWO_TIMES_D; ++d) {
      faces[0][d]   = {{FACES_ACCESSOR_OLD}}(d).value;
      faces[1][d]   = {{FACES_ACCESSOR_UPDATE}}(d).value;
      faces[2][d]   = {{FACE_ACCESSOR_NEW}}(d).value;
      isBoundary[d] = fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getBoundary() and (not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[d%DIMENSIONS]);
    }

    {% if CAN_FUSE %}
    ::exahype2::EnclaveBookkeeping::getInstance().bookmarkTask(new tasks::{{ENCLAVE_TASK_TYPE}}(
      marker,
      timeStamp,
      timeStepSize,
      QIn,
      faces,
      isBoundary,
      QOut
    ));
    {% else %}
    tarch::multicore::spawnTask(new tasks::{{ENCLAVE_TASK_TYPE}}(
      marker,
      timeStamp,
      timeStepSize,
      QIn,
      faces,
      isBoundary,
      QOut
    ));
    {% endif %}
  }
"""
    )

    def __init__(self, solver):
        super(EnclaveTasking, self).__init__(solver)
        self.guard = "not marker.willBeRefined() and not marker.hasBeenRefined() and marker.willBeEnclaveCell()"
        self.cell_accessor = "fineGridCell" + solver._patch.name
        self.reconstructed_accessor = "fineGridCell" + solver._patch_reconstructed.name
        self.face_accessor_new = "fineGridFaces" + solver._patch_overlap_new.name
        self.face_accessor_update = "fineGridFaces" + solver._patch_overlap_update.name
        self.face_accessor_old = "fineGridFaces" + solver._patch_overlap_old.name

    def _add_action_set_entries_to_dictionary(self, d):
        d["GUARD"] = self.guard
        d["CELL_ACCESSOR"] = self.cell_accessor
        d["RECONSTRUCTED_ACCESSOR"] = self.reconstructed_accessor
        d["FACE_ACCESSOR_NEW"] = self.face_accessor_new
        d["FACES_ACCESSOR_UPDATE"] = self.face_accessor_update
        d["FACES_ACCESSOR_OLD"] = self.face_accessor_old

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
