# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2

class ProjectLinearCombinationOfEstimatesOntoFaces(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
    #if Dimensions==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(0).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(1).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(2).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(3).value
    );
    #elif Dimensions==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(0).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(1).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(2).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(3).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(4).value,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}(5).value
    );
    #endif
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
    super(ProjectLinearCombinationOfEstimatesOntoFaces,self).__init__(solver)
    self.guard            = guard
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]                 = self.guard
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
