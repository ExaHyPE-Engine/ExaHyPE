# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.rkdg.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class ComputeFinalLinearCombinationAndProjectSolutionOntoFaces(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
    #if Dimensions==2
    for (int dof=0; dof<{{NUMBER_OF_DOFS_PER_CELL_2D}}; dof++)
    for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      const int destIndex = dof*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+unknown;
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + destIndex) = 0.0;

      int srcIndex = destIndex;
      {% for WEIGHT in WEIGHTS %}
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + destIndex) += {{WEIGHT}} * *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + srcIndex);
      srcIndex += {{NUMBER_OF_DOFS_PER_CELL_2D}};
      {% endfor %}
    }
    #elif Dimensions==3
    for (int dof=0; dof<{{NUMBER_OF_DOFS_PER_CELL_3D}}; dof++)
    for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      const int destIndex = dof*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+unknown;
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + destIndex) = 0.0;

      const int srcIndex = destIndex;
      {% for WEIGHT in WEIGHTS %}
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + destIndex) += {{WEIGHT}} * *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + srcIndex);
      srcIndex += {{NUMBER_OF_DOFS_PER_CELL_3D}};
      {% endfor %}
    }
    #endif


    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    // Set the variable
    // double newTimeStepSize
    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
  }
"""
  
  def __init__(self,solver,guard):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(ComputeFinalLinearCombinationAndProjectSolutionOntoFaces,self).__init__(solver)
    self.guard            = guard
    self._butcher_tableau = ButcherTableau(self._solver._rk_order)
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]         = self.guard
      d[ "WEIGHTS" ]           = self._butcher_tableau.final_estimate_weights()
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    # @todo Projektion auf die Raender und umschalten der Zeiten fehlt noch. Aus das Flag und das Berechnen der Folgezeitschrittweite (falls gewollt)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
