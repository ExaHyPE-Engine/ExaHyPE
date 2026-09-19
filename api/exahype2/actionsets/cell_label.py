# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

import exahype2.datamodel

from .action_set import ActionSet


class UpdateCellLabel(ActionSet):
    def get_attribute_name(solver_name):
        return solver_name + "CellLabel"

    def __init__(self, solver_name):
        super(UpdateCellLabel, self).__init__(descend_invocation_order=1)
        self._solver_name = solver_name

    _TEMPLATE_CREATE_CELL = """
  fineGridCell{{CELL_LABEL_NAME}}.setAMRCriterionEvaluatedThroughoutGridConstruction(false);
  fineGridCell{{CELL_LABEL_NAME}}.setRequiresCriterionEvaluation(false);
  if (tarch::la::equals(repositories::getMinTimeStamp(), 0.0)) {
    fineGridCell{{CELL_LABEL_NAME}}.setTimeStamp(0.0);
    fineGridCell{{CELL_LABEL_NAME}}.setTimeStepSize(0.0);
  } else {
    fineGridCell{{CELL_LABEL_NAME}}.setTimeStamp(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
    fineGridCell{{CELL_LABEL_NAME}}.setTimeStepSize(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStepSize());
  }
  fineGridCell{{CELL_LABEL_NAME}}.setHasUpdated(false);
"""

    def operation_body(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_CREATE_CELL:
            result += jinja2.Template(self._TEMPLATE_CREATE_CELL).render(
                CELL_LABEL_NAME=UpdateCellLabel.get_attribute_name(self._solver_name)
            )
        return result

    def attributes(self):
        return ""

    def includes(self):
        return ["repositories/SolverRepository.h"]


def create_cell_label(solver_name):
    result = exahype2.datamodel.MetaData(
        UpdateCellLabel.get_attribute_name(solver_name)
    )

    result.add_attribute(exahype2.datamodel.Double("timeStamp"))
    result.add_attribute(exahype2.datamodel.Double("timeStepSize"))
    result.add_attribute(exahype2.datamodel.Boolean("hasUpdated"))
    result.add_attribute(
        exahype2.datamodel.Boolean("AMRCriterionEvaluatedThroughoutGridConstruction")
    )
    # Realisation sweeps evaluate the criterion on cells created by a topology change.
    result.add_attribute(exahype2.datamodel.Boolean("requiresCriterionEvaluation"))

    result.merge_method_definition = """
  switch (context) {
  case ::peano4::grid::SendReceiveContext::InterRank:
    assertion(false, "Cells are never subject to an inter-rank exchange");
    break;
  default:
    assertion(false, "Not implemented");
  }
"""

    return result
