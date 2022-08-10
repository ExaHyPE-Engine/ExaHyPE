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
  
  Please consult the documentation of RungeKuttaDG for an explanation of the
  role of the two different boundary data fields.
  
  ## Time step sizes and time stamps

  Besides the actual projection, I also set the fields
    
      {{FACE_METADATA_ACCESSOR}}(d).setUpdated(1,true);
      {{FACE_METADATA_ACCESSOR}}(d).setUpdatedTimeStamp(1,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdated(0,true);
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdatedTimeStamp(0,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());

  on the face label. So the face now has a copy of the time stamp of the cell
  plus there's a marker that we are in the process of updating the adjacent
  cell. With the flag, we can determine whether a neighbouring cell runs a 
  Runge-Kutta step too or not.
  
  """
 
  
  TemplateProjectLinearCombination = """
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

    for (int d=0; d<Dimensions; d++) {
      {{FACE_METADATA_ACCESSOR}}(d).setUpdated(1,true);
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdated(0,true);
      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << d << ")(1)" );
      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << (d+Dimensions) << ")(0)" );
    }
  }
  {% endfor %}
"""
  

  TemplateProjectAsEpilogue = """
  if (
    repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
  ) {
    for (int d=0; d<Dimensions; d++) {
      {{FACE_METADATA_ACCESSOR}}(d).setUpdated(1,true);
      {{FACE_METADATA_ACCESSOR}}(d).setUpdatedTimeStamp(1,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());
      {{FACE_METADATA_ACCESSOR}}(d).setOldTimeStamp(    1,{{FACE_METADATA_ACCESSOR}}(d).getNewTimeStamp(1));
      {{FACE_METADATA_ACCESSOR}}(d).setNewTimeStamp(    1,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());

      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdated(0,true);
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setUpdatedTimeStamp(0,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setOldTimeStamp(    0,{{FACE_METADATA_ACCESSOR}}(d+Dimensions).getNewTimeStamp(0));
      {{FACE_METADATA_ACCESSOR}}(d+Dimensions).setNewTimeStamp(    0,{{CELL_METADATA_ACCESSOR}}.getTimeStamp());

      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << d << ")(1)" );
      logDebug( "touchCellLastTime(...)", "update {{FACE_METADATA_ACCESSOR}}(" << (d+Dimensions) << ")(0)" );
    }
  }
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
    d = {}
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    d[ "PREDICATES" ]                               = self.guards
    d[ "FACE_METADATA_ACCESSOR" ] = "fineGridFaces"  + self._solver._face_label.name
    d[ "CELL_METADATA_ACCESSOR" ] = "fineGridCell""" + self._solver._cell_label.name
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateProjectLinearCombination).render(**d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = jinja2.Template(self.TemplateProjectAsEpilogue).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
