# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

import peano4.solversteps
import jinja2
#TODO: duplicate the getQuadraturePoint function into aderdg folder?
class UpdateCell(AbstractAderDGActionSet):
  TemplateUpdateCell = """
  
        const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // Set the variable
      // double timeStepSize
      {{COMPUTE_TIME_STEPSIZE}}
   
      {{PREPROCESS_RECONSTRUCTED_PATCH}}

      assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
      assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );
      
      // Set the variable
      // double newTimeStepSize
      {{COMPUTE_NEW_TIME_STEP_SIZE}}
    
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    
      repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );

  
  """
  
  def __init__(self,solver,guard):
    super(UpdateCell,self).__init__(solver)
    self.guard               = guard

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = self.guard
      result = jinja2.Template(self.TemplateUpdateCell).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")