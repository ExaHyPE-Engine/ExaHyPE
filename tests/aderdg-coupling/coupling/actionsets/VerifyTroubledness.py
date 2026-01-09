# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet
from exahype2.solvers.PDETerms import PDETerms

import peano4.solversteps
import jinja2

from .kernels import check_troubledness

class VerifyTroubledness(AbstractLimiterActionSet):
  TemplateVerifyTroubledness = """

    if ( {{PREDICATE}} ) {


        const double timeStamp     = fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp();
        const double timeStepSize  = fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStepSize();

        bool isUnTroubled = true;

        auto* luh = fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.data();


        {% if USE_PAC %}

        int linearisedIndex = 0;
        dfor( index, {{ORDER}}+1 ) {
          
          isUnTroubled &= repositories::{{SOLVER_INSTANCE}}.isPhysicallyAdmissible(
            luh + linearisedIndex,
            ::exahype2::dg::getQuadraturePoint(
              marker.getX(), marker.getH(), index, repositories::{{REGULAR_SOLVER_INSTANCE}}.Order+1,
              kernels::{{REGULAR_SOLVER_NAME}}::Quadrature<{{REGULAR_SOLVER_STORAGE_PRECISION}}>::nodes
            ),
            marker.getH(),
            timeStamp+timeStepSize
          );

          linearisedIndex += repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfUnknowns + repositories::{{REGULAR_SOLVER_INSTANCE}}.NumberOfAuxiliaryVariables;
        }
       
        {% endif %}


        if(!isUnTroubled){
          repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
          fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
          for(int d=0; d<2*DIMENSIONS; d++){
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
          }
        }

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setHasUpdated(true);

  }

"""
  
  def __init__(self, solver, guard, use_PAC=False):
    super(VerifyTroubledness,self).__init__(solver)
    self.guard = guard

    self.use_PAC                    = use_PAC

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d["PREDICATE" ]           = self.guard
      d["USE_PAC"]              = self.use_PAC

      result = jinja2.Template(self.TemplateVerifyTroubledness).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return ( super(VerifyTroubledness, self).get_includes() + """
#include "kernels/"""
            + self._solver._regular_solver._name
            + """/Quadrature.h"
#include "exahype2/dg/DGUtils.h"
  """)