# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class ComputeFinalLinearCombination(AbstractRungeKuttaDGActionSet):
  """
  
  We solve the ODE
  
  @f$ \partial Q = F(Q) @f$
  
  where we have multiple guesses for F according to the Butcher tableau. The final 
  solution can be written down as linear combination over these guesses:
  
  
  @f$ Q^{new} = Q^{old} + a_1 \cdot dt \cdot F_1(Q) + a_2 \cdot dt \cdot F_2(Q) + \dots @f$
  
  We observe the following simple facts:
  
  - The data in fineGridCell{{UNKNOWN_IDENTIFIER}}.value holds @f$ Q^{old} @f$
  - The data in fineGridCell{{UNKNOWN_IDENTIFIER}}.value shall hold @f$ Q^{new} @f$ eventually.
    We thus can simply add the scaled estimated to it.
  - The estimates do not alter the auxiliary variables.
  - We can thus just keep the previous auxiliary variables implicitly.  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
    {{COMPUTE_TIME_STEP_SIZE}}

    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1,            {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
    dfor( dof, {{ORDER}}+1 ) {
      for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
        {% for WEIGHT_NO in range(0,WEIGHTS|length) %}
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] += 
          timeStepSize * {{WEIGHTS[WEIGHT_NO]}} * fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},dof,unknown) ];
        {% endfor %}
      }
    }
    

    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    double* targetPatch = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    
    {{POSTPROCESS_UPDATED_CELL}}

    {% if COMPUTE_MAX_EIGENVALUE %}
    ::exahype2::CellData  cellData( nullptr, marker.x(), marker.h(), timeStamp, timeStepSize, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );

    ::exahype2::dg::reduceMaxEigenvalue_patchwise_functors(
      cellData,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal
      )->double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, x, t, dt, normal );
      },
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
    );

    const double maxEigenvalue = cellData.maxEigenvalue[0];
    {% endif %}
        
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
    super(ComputeFinalLinearCombination,self).__init__(solver)
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


  def get_includes(self):
    return super(ComputeFinalLinearCombination,self).get_includes() + """
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
"""    
