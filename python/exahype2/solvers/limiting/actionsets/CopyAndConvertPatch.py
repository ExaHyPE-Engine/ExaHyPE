# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class CopyAndConvertPatch(AbstractLimiterActionSet):
  TemplateCopyAndConvertDataFromRegularToLimiterAndProjectToFaces = """
  if ({{CONVERT_FROM_REGULAR_TO_LIMITER_AND_SEND_PREDICATE}}) {
    // We need to send the projection of TROUBLED cells, their neighbors LIMITER_TO_REGULAR and the second neighbors.
    // - FV recalculate step for TROUBLED, LIMITER_TO_REGULAR cells based on all three types of cell we sent.
    // - Note that the seconds neighbours won't be recomputed on the FV step, but will be used to recompute LIMITER_TO_REGULAR cells.
    bool sendFaceProjection = false;
    celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker ownStatus = fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker();
    facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker faceStatus = facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED;
    sendFaceProjection  = (ownStatus == celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
    sendFaceProjection |= (ownStatus == celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::LIMITER_TO_REGULAR);
    for (int d = 0; d < 2 * Dimensions; d++) {
      faceStatus = fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker();
      sendFaceProjection |= (faceStatus == facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
      sendFaceProjection |= (faceStatus == facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::LIMITER_TO_REGULAR);
    }

    // If there's no need to send anything, skip this step.
    if (sendFaceProjection) {
      // Convert old (untroubled) regular solver solution to the limiter solver layer
      kernels::{{SOLVER_NAME}}::projectOnFVLimiterSpaceWithoutHalo(
        fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}_old.value,
        fineGridCell{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}.value
      );

      {% if LIMITER_USE_ADJUST_SOLUTION %}
      // Adjust the projected solution
      // - That is done to get rid off disturbances that come from the projection, if needed.
#if Dimensions == 2
      constexpr int xyzvarSize = {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}};
#elif Dimensions == 3
      constexpr int xyzvarSize = {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}} * {{NUMBER_OF_DOF_LIMITER}};
#endif

      for (int i = 0; i < xyzvarSize; i++) {
        repositories::{{LIMITER_SOLVER_INSTANCE}}.adjustSolution(
          marker.x(),
          marker.h(),
          fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp(),
          fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStepSize(),
          fineGridCell{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}.value + i * {{NUMBER_OF_DATA_PADDED}}
        );
      }
      {% endif %}
    }
  }
"""

  TemplateCopyAndConvertDataFromLimiterToRegular = """
  if ({{CONVERT_FROM_LIMITER_TO_REGULAR_PREDICATE}}) {
    // On the FV layer the troubled cells and their neighbors LIMITER_TO_REGULAR were recalculated.
    // Therefore we project limited values on the regular solver layer.
    kernels::{{SOLVER_NAME}}::projectOnDGSpaceFromFVWithoutHalo(
      fineGridCell{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}.value,
      fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value
    );
  }
"""

  def __init__(self, solver, regularToLimiterGuard, limiterToRegularGuard):
    super(CopyAndConvertPatch, self).__init__(solver)
    self.regularToLimiterGuard = regularToLimiterGuard
    self.limiterToRegularGuard = limiterToRegularGuard

  def get_body_of_operation(self, operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d["CONVERT_FROM_REGULAR_TO_LIMITER_AND_SEND_PREDICATE"] = self.regularToLimiterGuard
      result = jinja2.Template(self.TemplateCopyAndConvertDataFromRegularToLimiterAndProjectToFaces).render(**d)
    elif operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d["CONVERT_FROM_LIMITER_TO_REGULAR_PREDICATE"] = self.limiterToRegularGuard
      result = jinja2.Template(self.TemplateCopyAndConvertDataFromLimiterToRegular).render(**d)
    return result

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return (
          super(CopyAndConvertPatch, self).get_includes()
          + """
#include "kernels/"""  + self._solver._name + """/Limiter.h"
    """
      )
