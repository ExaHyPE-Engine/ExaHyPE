# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2

class SolveVolumeIntegral(AbstractRungeKuttaDGActionSet):
  """
  
  Solve volume integral
  
  Solve the volumetric part of the DG method. This includes the evaluation of the
  volume integral of the operator multiplied with a trial function, plus the 
  subsequent multiplication with an inverted (lumped) mass matrix. So, eventually,
  we do an explicit Euler step subject to a relaxed time step size from the Butcher
  tableau. The input is already properly computed (see LinearCombinationOfEstimates()).
  
  The volume integral definition is an underdetermined action set, i.e. you still
  have to ensure that a number of macros are actually defined when you use it:
  
  - {{COMPUTE_TIME_STEP_SIZE}} has to specify a double timeStepSize. It may be const if 
    you want. I don't alter it.
  
  In return, the following variables will be determined by the action set:
  
  - timeStamp: double This is the time stamp of the original solution, i.e. not the
    one we are currently evaluating due to the Runge-Kutta scheme.
  -  
  
    
  """
  
  TemplateCellKernel = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value;
    #if Dimensions==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif
    
    ::exahype2::dg::explicitEulerOnCell_FluxNCPSource_EvaluateTermsRedundantly(
      QIn,  
      QOut,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(),
      marker.h()(0), //assume square/cubic cells
      timeStamp,
      timeStepSize,
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  dQdx,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, dQdx, x, t, normal, F);
          {% endif %}          
        }, //ncp
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double * __restrict__                        S
      ) -> void {
        {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q,x,t,S);
        {% endif %}
      }, //source
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
      repositories::{{SOLVER_INSTANCE}}.MassMatrix,
      repositories::{{SOLVER_INSTANCE}}.StiffnessOperator,
      repositories::{{SOLVER_INSTANCE}}.DerivativeOperator,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }}, //useNCP
      {{ "true" if SOURCE_TERM_IMPLEMENTATION!="<none>" else "false" }} //useSource
    );    

    /*
    #if Dimensions==2
    const int facePosition = {{PREDICATE_NO}}*{{NUMBER_OF_DOFS_PER_FACE_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    const int facePosition = {{PREDICATE_NO}}*{{NUMBER_OF_DOFS_PER_FACE_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif

    #if Dimensions==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value + facePosition, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).value + facePosition, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).value + facePosition, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(3).value + facePosition  //up
    );
    #elif Dimensions==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value + facePosition, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(3).value + facePosition, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).value + facePosition, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(4).value + facePosition, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).value + facePosition, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(5).value + facePosition  //back
    );
    #endif
    */
    
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
    super(SolveVolumeIntegral,self).__init__(solver)
    self.guards               = []
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
      result = jinja2.Template(self.TemplateCellKernel).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    
    return super(SolveVolumeIntegral,self).get_includes() + """
    #include "exahype2/dg/Euler.h"
    """