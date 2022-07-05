# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class AddVolumeAndFaceSolution(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );

    #if Dimensions==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * {{NUMBER_OF_UNKNOWNS}};
    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * {{NUMBER_OF_UNKNOWNS}};
    #endif
/*
    #if Dimensions==2
    ::exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).value, //up
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d,
      QOut
    );
    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    
    ::exahype2::dg::integrateOverRiemannSolutionsAndAddToVolume_GaussLegendre(
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(3).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(4).value, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(2).value, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(5).value, //back
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.h(),
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d,
      QOut
    );
    #endif
*/

    ::exahype2::CellData cellData(
      nullptr,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      QOut
    );

    ::exahype2::dg::multiplyWithInvertedMassMatrixAndClearAuxiliaryVariables_GaussLegendre(
      cellData,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.MassMatrixDiagonal1d
    );
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
    super(AddVolumeAndFaceSolution,self).__init__(solver)
    self.guards           = []
    self._butcher_tableau = ButcherTableau(self._solver._rk_order)
    

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
      d[ "PREDICATES" ]              = self._guards
      d[ "BUTCHER_TABLEAU_WEIGHTS" ] = self._butcher_tableau.weight_matrix()      
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
