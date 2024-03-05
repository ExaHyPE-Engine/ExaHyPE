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

        auto* luh = fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value;


        {% if USE_PAC %}
        isUnTroubled &= generated::kernels::limiter::isPhysicallyAdmissible(
          luh,
          repositories::{{SOLVER_INSTANCE}},
          marker.x(),
          marker.h(),
          timeStamp+timeStepSize
        );
        {% endif %}


        {% if USE_DMP %}
        constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
        double boundaryMinPerVariables[2*Dimensions*numberOfObservables];
        double boundaryMaxPerVariables[2*Dimensions*numberOfObservables];

        for(int d=0; d<Dimensions; d++){
            std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value,                                 numberOfObservables, &boundaryMinPerVariables[d*numberOfObservables]);
            std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+numberOfObservables,  numberOfObservables, &boundaryMinPerVariables[(d+Dimensions)*numberOfObservables]);

            std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+2*numberOfObservables,            numberOfObservables, &boundaryMaxPerVariables[d*numberOfObservables]);
            std::copy_n(fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+3*numberOfObservables, numberOfObservables, &boundaryMaxPerVariables[(d+Dimensions)*numberOfObservables]);
        }

        isUnTroubled &= generated::kernels::limiter::discreteMaximumPrincipleAndMinAndMaxSearch(
          luh,
          repositories::{{SOLVER_INSTANCE}},
          repositories::{{SOLVER_INSTANCE}}.RelaxationParameter, //const double relaxationParameter
          repositories::{{SOLVER_INSTANCE}}.DifferencesScaling, //const double differenceScaling
          boundaryMinPerVariables,
          boundaryMaxPerVariables
        );
        {% endif %}


        if(!isUnTroubled){
          repositories::{{SOLVER_INSTANCE}}.addTroubledCell();
          fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::TROUBLED);
          for(int d=0; d<2*Dimensions; d++){
            fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::TROUBLED);
          }
        }

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);

        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setHasUpdated(true);

  }

"""
  
  def __init__(self, solver, guard, use_PAC=False, use_DMP=False):
    super(VerifyTroubledness,self).__init__(solver)
    self.guard = guard

    self.use_PAC                    = use_PAC
    self.use_DMP                    = use_DMP

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d["PREDICATE" ]           = self.guard
      d["USE_PAC"]              = self.use_PAC
      d["USE_DMP"]              = self.use_DMP
      result = jinja2.Template(self.TemplateVerifyTroubledness).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
      return (
          super(VerifyTroubledness, self).get_includes()
          + """
#include "../generated/kernels/limiter/fv/Kernels.h"
    """
      )