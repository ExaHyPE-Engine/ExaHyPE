# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from exahype2.solvers.rkdg.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class LinearCombinationOfEstimates(AbstractRungeKuttaDGActionSet):
  """
  
  
  
  """
  
  TemplateLinearCombination = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    #if Dimensions==2
    for (int dof=0; dof<{{NUMBER_OF_DOFS_PER_CELL_2D}}; dof++)
    for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      const int index = dof*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+unknown;
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index)  = *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index);
      {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO]|length) %}
      {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]==0 %}
      // *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index) += {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}}* *(fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + index + {{WEIGHT_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}});
      {% else %}
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index) += {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}}* *(fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + index + {{WEIGHT_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}});
      {% endif %}
      {% endfor %}
    }
    #endif

    #if Dimensions==3
    for (int dof=0; dof<{{NUMBER_OF_DOFS_PER_CELL_3D}}; dof++)
    for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
      const int index = dof*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})+unknown;
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index)  = *(fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index);
      {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO]|length) %}
      {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]==0 %}
      // *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index) += {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}}* *(fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + index + {{WEIGHT_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}});
      {% else %}
      *(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value + index) += {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}}* *(fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + index + {{WEIGHT_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}});
      {% endif %}
      {% endfor %}
    }
    #endif
  }
  {% endfor %} 
"""
  
  def __init__(self,solver):
    """
    
    guard: [String]  (C++ code)
      Sequence of predicates which determine when we compute the respective RK 
      linear combination. Can be empty.
    
    """
    super(LinearCombinationOfEstimates,self).__init__(solver)
    self._guards              = []
    self._butcher_tableau     = ButcherTableau(self._solver._rk_order)
    

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
      d[ "PREDICATES" ]                 = self.guards
      d[ "BUTCHER_TABLEAU_WEIGHTS" ]    = self._butcher_tableau.weight_matrix()      
      result = jinja2.Template(self.TemplateLinearCombination).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
