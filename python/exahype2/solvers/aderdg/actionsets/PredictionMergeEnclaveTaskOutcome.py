# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

import jinja2
import peano4
import exahype2


class PredictionMergeEnclaveTaskOutcome(AbstractADERDGActionSet):
    Template = """
  if (
    not marker.hasBeenRefined()
    and
    marker.hasBeenEnclaveCell()
    and
    {{GUARD}}
  ) {
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    {{COMPUTE_TIME_STEP_SIZE}}

    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if (taskNumber >= 0) {
      double maxEigenvalue = 0.0; // Not used here. TODO: Use in correction.
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver(taskNumber, QOut, maxEigenvalue);
      repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0));
    }
  }

  // Projected solution estimations
  //std::copy_n(&lQhbnd[2*d*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsPerFace);
  //std::copy_n(&lQhbnd[(2*d+1)*basisElementsPerFace], basisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);

  // Projected flux estimations
  //std::copy_n(&lFhbnd[2*d*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace);
  //std::copy_n(&lFhbnd[(2*d+1)*fluxElementsPerFace], fluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);            

  // Timestamp information for boundary handling
  //fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
  //fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).setNewTimeStamp(0, timeStamp);
"""

    def __init__(self, solver):
        super(PredictionMergeEnclaveTaskOutcome, self).__init__(solver)
        self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)
        self.guard = "true"


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


    def get_action_set_name(self):
        return (
            __name__.replace(".py", "").replace(".", "_")
        )


    def get_includes(self):
        return (
            super(PredictionMergeEnclaveTaskOutcome, self).get_includes()
            + """
#include "exahype2/EnclaveBookkeeping.h"
"""
        )