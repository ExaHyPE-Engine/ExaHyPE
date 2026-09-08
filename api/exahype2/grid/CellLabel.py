# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.datamodel


class UpdateCellLabel(ActionSet):
    def get_attribute_name(solver_name):
        return solver_name + "CellLabel"

    def __init__(self, solver_name):
        super(UpdateCellLabel, self).__init__(descend_invocation_order=1)
        self._solver_name = solver_name

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_CREATE_CELL:
            result += (
                """
  fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setAMRCriterionEvaluatedThroughoutGridConstruction( false );
  fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setRequiresCriterionEvaluation( false );
  if ( tarch::la::equals(repositories::getMinTimeStamp(),0.0) ) {
    fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setTimeStamp(  0.0 );
    fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setTimeStepSize( 0.0 );
  }
  else {
    fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setTimeStamp(  coarseGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.getTimeStamp() );
    fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setTimeStepSize( coarseGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.getTimeStepSize() );
  }
  fineGridCell"""
                + UpdateCellLabel.get_attribute_name(self._solver_name)
                + """.setHasUpdated( false );
"""
            )
        return result

    def get_attributes(self):
        return ""

    def get_includes(self):
        return """
#include "Constants.h"
#include "repositories/SolverRepository.h"
"""


def create_cell_label(solver_name):
    result = peano4.datamodel.MetaData(UpdateCellLabel.get_attribute_name(solver_name))

    result.add_attribute(peano4.datamodel.Double("timeStamp"))
    result.add_attribute(peano4.datamodel.Double("timeStepSize"))
    result.add_attribute(peano4.datamodel.Boolean("hasUpdated"))
    result.add_attribute(
        peano4.datamodel.Boolean("AMRCriterionEvaluatedThroughoutGridConstruction")
    )
    # Marks a cell a topology change has just created, so the realisation sweeps
    # evaluate the refinement criterion only there.
    result.add_attribute(peano4.datamodel.Boolean("requiresCriterionEvaluation"))

    result.merge_method_definition = """
  switch (context) {
    case ::peano4::stacks::SendReceiveContext::InterRank:
      assertion(false, "Cells are never subject to an inter-rank exchange");
      break;
    default:
      assertion(false, "Not implemented yet");
  }
"""

    return result
