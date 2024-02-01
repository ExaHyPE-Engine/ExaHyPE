# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class SpreadLimiterStatus(AbstractLimiterActionSet):
  TemplateSpreadLimiterStatus = """

  if ( {{PREDICATE}} ) {

      if(fineGridCell{{SOLVER_NAME}}CellLabel.getTroubled_Marker()==celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED) return;

      bool hasTroubledNeighbour = false;
      for(int d=0; d<2*Dimensions; d++){
        hasTroubledNeighbour |= fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker()==facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED;
      }

      if(hasTroubledNeighbour){
        fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::LIMITER_TO_REGULAR);
        for(int d=0; d<2*Dimensions; d++){
          fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(
            std::max(fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getTroubled_Marker(),
            facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::LIMITER_TO_REGULAR
            )
          );
        }
      }

  }

"""
  
  def __init__(self,solver,guard):
    super(SpreadLimiterStatus,self).__init__(solver)
    self.guard               = guard

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard
      result = jinja2.Template(self.TemplateSpreadLimiterStatus).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
