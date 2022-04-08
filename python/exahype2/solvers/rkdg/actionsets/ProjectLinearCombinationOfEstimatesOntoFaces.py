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
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value;
    
    #if Dimensions==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(3).value  //up
    );
    #elif Dimensions==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(3).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(4).value, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).value, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(5).value  //back
    );
    #endif

  }
  {% endfor %}
"""
  
  def __init__(self,solver):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(ProjectLinearCombinationOfEstimatesOntoFaces,self).__init__(solver)
    self.guards            = []

  @property
  def guards(self):
    if self._guards==[]:
      raise Exception( "Guards are not initialised" )
    return self._guards
      
    
  @guards.setter
  def guards(self,new_guards):
    if new_guards!=[] and len(new_guards)!=self._solver.number_of_Runge_Kutta_steps():
      raise Exception( "Expect one guard per Runge Kutta step. Have {} steps but got guards {}".format(solver.number_of_Runge_Kutta_steps(),guards) )
    self._guards = new_guards
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]                               = self.guards
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
