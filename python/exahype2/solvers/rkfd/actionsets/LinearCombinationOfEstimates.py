# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRKFDActionSet import AbstractRKFDActionSet
from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2


class LinearCombinationOfEstimates(AbstractRKFDActionSet):
  """
  
  Computes a linear combination of all of the estimates according to the
  Butcher Tableau.
  
  The linear combination is a volumetric representation which includes both
  the degrees of freedom and the auxiliary variables. However, the auxiliary
  variables are not developing over time. In Runge-Kutta, I have estimates 
  for the right-hand side, i.e. for the derivatives
  
  @f$ \partial _t Q = F(Q) @f$
  
  This is the stuff stored in RhsEstimates. It does not comprise any auxiliary
  variables. So I have to copy the auxiliary variables from the last valid
  time step every time I reconstruct a new guess.
  
  """
  
  TemplateLinearCombination = """
  // Set the variable
  // double timeStepSize
  {{COMPUTE_TIME_STEP_SIZE}}
    
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1,            {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
    dfor( dof, {{ORDER}}+1 ) {
      for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] = 
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ];      

        {% for WEIGHT_NO in range(0,BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO]|length) %}
        {% if BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]!=0 %}
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] += 
          {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize * {{BUTCHER_TABLEAU_WEIGHTS[PREDICATE_NO][WEIGHT_NO]}} * 
          fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},dof,unknown) ];
        {% endif %}
        {% endfor %}
      }      
      for (int unknown={{NUMBER_OF_UNKNOWNS}}; unknown<{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}; unknown++) {
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] = 
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ];      
      }
    }
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
      d[ "PREDICATES" ]                               = self.guards
      d[ "BUTCHER_TABLEAU_WEIGHTS" ]                  = self._butcher_tableau.weight_matrix()
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      result = jinja2.Template(self.TemplateLinearCombination).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(LinearCombinationOfEstimates,self).get_includes() + """
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
"""
