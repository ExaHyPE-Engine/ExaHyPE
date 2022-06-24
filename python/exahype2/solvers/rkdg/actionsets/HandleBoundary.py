# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2

class HandleBoundary(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  ## Time step/time stamp handling
  
  We rely on ProjectLinearCombinationOfEstimatesOntoFaces to project the 
  current solution onto the faces.
  
  
  """
  
  TemplateProject = """
    /*
    In Peano the faces are ordered by direction, then dimension, so in 3D it would be the face in negative direction in dimension 0, then 1, then 2
      followed by the the face in positive direction of dimensions 0, then 1, then 2. The kernels expect the faces to be ordered by dimension, then
      direction, so first left face in dimension0, then right face in dimension 0, and so on for the other dimensions.
      Therefore when the Hull is arranged, the order of these faces are shuffled correspondingly.    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if (
    {{PREDICATES[PREDICATE_NO]}}
    and
    not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%Dimensions]
    and
    not marker.hasBeenRefined() 
    and 
    fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
  ) {
    // see action set ProjectLinearCombinationOfEstimatesOntoFaces and docu of action set
    const double timeStampOldSolution    = std::max(
      fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp()(0),
      fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp()(1)
    );
      
  
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );
      
    ::exahype2::dg::applyBoundaryConditions(
      [&](
        const double * __restrict__                  Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> void {
        repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, Qoutside, x, t, normal );
      },  
      marker.x(),
      marker.h(),
      timeStamp,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value
    );
  }
  {% endfor %}
"""
  
  def __init__(self,solver,guard):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(HandleBoundary,self).__init__(solver)
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
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
