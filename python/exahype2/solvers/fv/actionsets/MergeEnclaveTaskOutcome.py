# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

import peano4.solversteps
import exahype2.grid

from .AbstractFVActionSet import AbstractFVActionSet

class MergeEnclaveTaskOutcome(AbstractFVActionSet):
    Template = """
  if (
    not marker.hasBeenRefined()
    and
    {{GUARD}}
    and
    repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::Secondary
  ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if (marker.hasBeenEnclaveCell() and taskNumber >= 0) {
      double maxEigenvalue = 0.0;
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver(taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value, maxEigenvalue);

      #if PeanoDebug > 0
      ::exahype2::fv::validatePatch(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0, // Halo size
        std::string(__FILE__) + "(" + std::to_string(__LINE__) + ")"
      ); // Outcome has to be valid
      #endif

      {{COMPUTE_NEW_TIME_STEP_SIZE}}

      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber(::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber);
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    }

    if (fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()) {
      double* newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

      {{POSTPROCESS_UPDATED_PATCH}}

      repositories::{{SOLVER_INSTANCE}}.update(
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    } else {
      repositories::{{SOLVER_INSTANCE}}.update(
        0.0,
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
  }
"""

    def __init__(self, solver):
        super(MergeEnclaveTaskOutcome, self).__init__(solver)
        self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)
        self.guard = "true"
        self.descend_invocation_order = solver._baseline_action_set_descend_invocation_order

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["LABEL_NAME"] = self.label_name
            d["GUARD"] = self.guard
            result = jinja2.Template(self.Template).render(**d)
            pass
        return result

    def get_includes(self):
        return """
#include "exahype2/fv/PatchUtils.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
