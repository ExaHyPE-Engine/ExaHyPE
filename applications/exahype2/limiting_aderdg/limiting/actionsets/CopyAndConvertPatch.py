# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class CopyAndConvertPatch(AbstractLimiterActionSet):
  TemplateCopyAndConvertDataFromRegularToLimiterAndProjectToFaces = """

  if ( {{CONVERT_FROM_REGULAR_TO_LIMITER_AND_SEND_PREDICATE}} ) {

      celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker ownStatus = fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker();

      bool sendFaceProjection = ownStatus>=celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR_TO_LIMITER;
      for(int d=0; d<2*Dimensions; d++){
        sendFaceProjection |= fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()>= facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::LIMITER_TO_REGULAR;
      }

      //If there's no need to send anything, skip this step
      if(!sendFaceProjection) return;

      double* regularSolverOldValues = fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value;
      double* limiterSolverNewValues = fineGridCell{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}.value;

      //TODO: replace with more general conversion, probably from kernels
      //converts DG into fv
      generated::kernels::limiter::projectOnFVLimiterSpaceWithoutHalo(regularSolverOldValues, limiterSolverNewValues);

  }"""

  """
""",

  TemplateCopyAndConvertDataFromLimiterToRegular = """

  if ( {{CONVERT_FROM_LIMITER_TO_REGULAR_PREDICATE}} ) {

      //TODO: make this prettier and maybe put it in a kernel
      generated::kernels::limiter::projectOnDGSpaceFromFVWithoutHalo(
        fineGridCell{{LIMITER_SOLVER_UNKNOWN_IDENTIFIER}}.value,
        fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value
      );

  }"""
  
  def __init__(self,solver, regularToLimiterGuard, limiterToRegularGuard):
    super(CopyAndConvertPatch,self).__init__(solver)
    self.regularToLimiterGuard = regularToLimiterGuard
    self.limiterToRegularGuard = limiterToRegularGuard

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "CONVERT_FROM_REGULAR_TO_LIMITER_AND_SEND_PREDICATE" ] = self.regularToLimiterGuard
      result = jinja2.Template(self.TemplateCopyAndConvertDataFromRegularToLimiterAndProjectToFaces).render(**d)
      pass 
    elif operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "CONVERT_FROM_LIMITER_TO_REGULAR_PREDICATE"] = self.limiterToRegularGuard
      result = jinja2.Template(self.TemplateCopyAndConvertDataFromLimiterToRegular).render(**d)
      pass
    return result

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return (
          super(CopyAndConvertPatch, self).get_includes()
          + """
#include "../generated/kernels/limiter/fv/Kernels.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "exahype2/fv/rusanov/rusanov.h"
    """
      )