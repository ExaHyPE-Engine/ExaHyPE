# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2

class SolveRiemannProblem(AbstractRungeKuttaDGActionSet):
  """
  
  Solve the actual Riemann problem and add stuff back to the solution.


  See the discussion of RungeKuttaDG for a discussion regarding the individual 
  steps. 
  
  """
  
  TemplateProject = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    // no support for local time stepping or subcycling 
    
    // @todo net klar, ob wir richtigen verwenden
    
    assertionNumericalEquals(
      {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(0),
      {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(1)
    );
    const double timeStampOldSolution = {{FACE_METADATA_ACCESSOR}}.getNewTimeStamp(0);
  
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );
    
    //::exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ(
    #if Dimensions==2
    const double* __restrict__ QIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    double* __restrict__       QOut = fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value    + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    const double* __restrict__ QIn = fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif


    ::exahype2::dg::rusanov::solveRiemannProblem_pointwise_in_situ(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  deltaQ,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, deltaQ, x, t, dt, normal, F);
          {% endif %}          
      }, //ncp
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q, x, t, dt, normal);
      }, // maxEigenvalue
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }},   //useNCP
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.value
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
    super(SolveRiemannProblem,self).__init__(solver)
    self.guards            = []
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]            = self.guards
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      d[ "FACE_METADATA_ACCESSOR" ] = "fineGridFace"  + self._solver._face_label.name
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
  
  
  def get_includes(self):
    return super(SolveRiemannProblem,self).get_includes() + """
#include "exahype2/dg/rusanov/Riemann.h"
    """  
