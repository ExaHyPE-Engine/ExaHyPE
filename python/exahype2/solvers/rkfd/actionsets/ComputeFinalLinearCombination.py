# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRKFDActionSet import AbstractRKFDActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class ComputeFinalLinearCombination(AbstractRKFDActionSet):
  """!
  
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
  
  In the FD4 world, we can integrate the ComputeFinalLinearCombination
  action set directly into the very last mesh sweep. At this point, all
  the right-hand side estimates are in place.
  
  
  ## Invocation order
  
  It is important to study this action set in combination with ProjectPatchOntoFaces.
  Both plug into touchCellLastTime(), but the projection has to come before the
  computation of the final solution. So it has to have a higher order as we invert
  the order throughout the steps up the tree.
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
    {{COMPUTE_TIME_STEP_SIZE}}

    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1,            {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
    dfor( dof, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
      for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
        {% for WEIGHT_NO in range(0,WEIGHTS|length) %}
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] += 
          timeStepSize * {{WEIGHTS[WEIGHT_NO]}} * fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},dof,unknown) ];
        {% endfor %}
       }
    }

    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    double* newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    
    {{POSTPROCESS_UPDATED_PATCH}}

    ::exahype2::fd::validatePatch(
      newQ,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      0,  // we do not have an overlap of {{OVERLAP}} here within the cell data, only within reconstructed values
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // outcome has to be valid

    {% if COMPUTE_MAX_EIGENVALUE %}
    ::exahype2::CellData  patchData( 
      nullptr,      // QIn
      marker.x(), 
      marker.h(), 
      timeStamp, 
      timeStepSize, 
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value 
    );

    ::exahype2::fd::reduceMaxEigenvalue_patchwise_functors(
      patchData,
      {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
      0,                            // only reconstructed data have overlap of {{OVERLAP}}, not the real data
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  gridCellCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
      )->double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, gridCellCentre, gridCellH, t, dt, normal );
      }
    );

    const double maxEigenvalue = patchData.maxEigenvalue[0];
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
#include "exahype2/fd/LoopBody.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
"""
