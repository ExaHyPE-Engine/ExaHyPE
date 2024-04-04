# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractLimiterActionSet import AbstractLimiterActionSet

import peano4.solversteps
import jinja2
class SaveNewCellData(AbstractLimiterActionSet):
  TemplateSaveNewCellData = """

  if ( {{PREDICATE}} ) {


    {% if COPY_OLD_VALUES %}
      //copying old values
      std::copy_n(
        fineGridCell{{REGULAR_SOLVER_UNKNOWN_IDENTIFIER}}.value,
#if Dimensions==2
        {{NUMBER_OF_DOFS_PER_CELL_2D}},
#else
        {{NUMBER_OF_DOFS_PER_CELL_3D}},
#endif
        fineGridCell{{UNKNOWN_IDENTIFIER}}_regularOld.value
      );
    {% endif %}


    {% if USE_DMP %}
      //resetting local min and max
      constexpr int numberOfObservables = repositories::{{SOLVER_INSTANCE}}.NumberOfDMPObservables;
      double localMinPerVariables[numberOfObservables], localMaxPerVariables[numberOfObservables];

      generated::kernels::limiter::findCellLocalMinAndMax(
        fineGridCell{{UNKNOWN_IDENTIFIER}}_regularOld.value,
        repositories::{{SOLVER_INSTANCE}},
        localMinPerVariables, localMaxPerVariables
      );

      for(int d=0; d<Dimensions; d++){
          //Projection of cell local min and max
          std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+numberOfObservables);
          std::copy_n(localMinPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value);
          std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d).value+3*numberOfObservables);
          std::copy_n(localMaxPerVariables, numberOfObservables, fineGridFaces{{SOLVER_NAME}}Q_min_and_max(d+Dimensions).value+2*numberOfObservables);
      }
    {% endif %}


    {% if RESET_TROUBLED_MARKERS %}
      //resetting troubled markers
      fineGridCell{{SOLVER_NAME}}CellLabel.setTroubled_Marker(celldata::{{SOLVER_NAME}}CellLabel::Troubled_Marker::REGULAR);
      for(int d=0; d<2*Dimensions; d++){
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setTroubled_Marker(facedata::{{SOLVER_NAME}}FaceLabel::Troubled_Marker::REGULAR);
      }
    {% endif %}

    {% if TRAMSMIT_CELL_TIMESTAMPS %}
      //set timestamp for both cells to be the max of the two possible timestamps
      const double timeStamp = std::max(
        fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.getTimeStamp(),
        fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.getTimeStamp()
      );
      fineGridCell{{LIMITER_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
      fineGridCell{{REGULAR_SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp);
    {% endif %}

  }

"""
  
  def __init__(self, solver, guard, copy_old_values=False, use_dmp=False, reset_troubled_markers=False, transmit_cell_timestamps=False):
    super(SaveNewCellData,self).__init__(solver)
    self.guard                      = guard
    self._copy_old_values           = copy_old_values
    self._use_dmp                   = use_dmp
    self._reset_troubled_markers    = reset_troubled_markers
    self._transmit_cell_timestamps  = transmit_cell_timestamps

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]              = self.guard
      d["COPY_OLD_VALUES"]          = self._copy_old_values
      d["USE_DMP"]                  = self._use_dmp
      d["RESET_TROUBLED_MARKERS"]   = self._reset_troubled_markers
      d["TRAMSMIT_CELL_TIMESTAMPS"] = self._transmit_cell_timestamps
      result = jinja2.Template(self.TemplateSaveNewCellData).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
  
  def get_includes(self):
      return (
          super(SaveNewCellData, self).get_includes()
          + """
#include "../generated/kernels/limiter/fv/Kernels.h"
    """
      )
