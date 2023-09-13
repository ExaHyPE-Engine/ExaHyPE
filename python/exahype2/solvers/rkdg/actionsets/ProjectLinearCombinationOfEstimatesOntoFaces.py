# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2

from exahype2.solvers.ButcherTableau                import ButcherTableau

from enum import Enum

class FaceProjections(Enum):
  """
  Different data can be projected onto the faces. In the simplest variant,
  only the solution is projected on the faces and the Riemann solver then
  reconstructs the flux from the arising jump, i.e. the solution left and
  right of the face. There are more sophisticated variants however which
  exploit the solution left and right plus additional data such as the
  gradient along the normal.
  """
  Solution = 1,
  SolutionAndGradient = 2,
  SolutionAndFluxExtrapolation = 3

def compute_number_of_face_projection_quantities(face_projections: FaceProjections):
  """
  Translate the information what is projected onto the faces into a number
  how many quantities are to be held per face.
  """
  if face_projections   == FaceProjections.Solution:
    return 1
  elif face_projections == FaceProjections.SolutionAndGradient:
    return 2
  elif face_projections == FaceProjections.SolutionAndFluxExtrapolation:
    return 2
  else: 
    assert False, "variant {} not handled".format( face_projections )
    return -1

class ProjectLinearCombinationOfEstimatesOntoFaces(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge-Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  Please consult the documentation of RungeKuttaDG for an explanation of the
  role of the two different boundary data fields.

  
  ## Time step sizes and time stamps

  Besides the actual projection, I also set the following fields on the 
  face:

  - Updated    Is set after each projection.
  
  - UpdatedTimeStamp   Is set after each projection with the time stamp 
               of the current Runge-Kutta trial. 
               
  We do not update the new and old time stamp on the face label. These
  values are properly set in the ComputeFinalLinearCombination action set.


  ## Guards

  We can set separate guards for each individual step (which we have to do,
  as the different steps depend on the role of the individual Runge-Kutta
  phases). The action set does not initialise the guards with a dummy. 
  Instead, I expect that the person who creates the action set explicitly 
  sets the guard list before the action set is handed over to the code 
  generation phase.  
    
  """
 
  
  TemplateProjectLinearCombinationSolution = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value;
    
    #if Dimensions==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).value  //up
    );
    #elif Dimensions==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(4).value, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).value, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(5).value  //back
    );
    #endif

      
    // Set the variable
    // double timeStepSize
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    {{COMPUTE_TIME_STEP_SIZE}}    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    for (int d=0; d<Dimensions; d++) {
      {{FACE_METADATA_ACCESSOR}}(d).setUpdated(           1,true);
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdated(0,true);

      {{FACE_METADATA_ACCESSOR}}(d).setUpdatedTimeStamp(           1, timeStamp );
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdatedTimeStamp(0, timeStamp );

      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << d << ")(1)" );
      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << (d+Dimensions) << ")(0)" );
    }
  }
  {% endfor %}
"""
  

  def __init__(self,
               solver,
               face_projections: FaceProjections):
    """
    
    
    """
    super(ProjectLinearCombinationOfEstimatesOntoFaces,self).__init__(solver)
    self.guards            = []
    self.face_projections  = face_projections
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
    d = {}
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    d[ "PREDICATES" ]                               = self.guards
    d[ "FACE_METADATA_ACCESSOR" ]   = "fineGridFaces"  + self._solver._face_label.name
    d[ "CELL_METADATA_ACCESSOR" ]   = "fineGridCell""" + self._solver._cell_label.name
    d[ "WEIGHTS" ]                  = self._butcher_tableau.final_estimate_weights()
    d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME and self.face_projections==FaceProjections.Solution:
      result = jinja2.Template(self.TemplateProjectLinearCombinationSolution).render(**d)
      pass 
    elif operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      assert False, "face projection variant {} not supported yet ".format( self.face_projections)
      
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
