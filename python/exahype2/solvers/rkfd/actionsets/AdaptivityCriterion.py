# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRKFDActionSet import AbstractRKFDActionSet

import peano4
import jinja2

class AdaptivityCriterion(AbstractRKFDActionSet):
  """!
  
  The action set to realise AMR
  
  AMR is a multistep process in ExaHyPE. Most of the relevant documentation 
  on it is documented in the class exahype2::RefinementControl.
  
  There are different phases in ExaHyPE: grid generation (either with 
  refinement or without), initialisation, plotting and time stepping. It 
  depends on your solver in which time stepping to use AMR, but the there
  are some things to take into account that are in common for all schemes:
  
  - The grid creation does not use any actual solver data, so we should not
    evalutae the adaptivity criterion here.
  - InitGrid has volumetric data and might want to evaluate the criterion.
    This is implicitly done by the t=0 check below.
  - The plotting does not alter the solution. It thus makes no sense to 
    evaluate the criterion here.
  
  Please consult EnclaveTasking.create_action_sets for details regarding
  the handling of AMR within the enclave tasking concept.
  
  """
  
  
  TemplateAMR = """  
  logTraceInWith2Arguments( "touchCellFirstTime(...)", marker.willBeRefined(), marker.hasBeenRefined() );

  ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

  {% if GRID_CONSTRUCTION %}
  if ( 
    not marker.willBeRefined() 
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MaxAdmissiblePatchH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( 
    not marker.willBeRefined() 
    and
    tarch::la::greaterEquals( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MinAdmissiblePatchH )
    and
    not fineGridCell{{CELL_LABEL_NAME}}.getAMDCriterionEvaluatedThroughoutGridConstruction()
  ) { 
    dfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
        refinementCriterion = refinementCriterion and repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
          nullptr,
          ::exahype2::fd::getGridCellCentre( marker.x(), marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, volume), 
          ::exahype2::fd::getGridCellSize( marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ),
          0.0
        );
    }
    fineGridCell{{CELL_LABEL_NAME}}.setAMDCriterionEvaluatedThroughoutGridConstruction(true);
  }  
  else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% else %}
  if ( 
    {{PREDICATE}}
    and
    not marker.willBeRefined() 
    and
    tarch::la::greater( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MaxAdmissiblePatchH) 
  ) {
    refinementCriterion = ::exahype2::RefinementCommand::Refine;
  } 
  else if ( 
    {{PREDICATE}}
    and
    fineGridCell{{CELL_LABEL_NAME}}.getHasUpdated()
  ) { 
    int index = 0;
    dfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
        refinementCriterion = refinementCriterion and repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
          ::exahype2::fd::getGridCellCentre( marker.x(), marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}}, volume), 
          ::exahype2::fd::getGridCellSize( marker.h(), {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ),
          repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
        );
        index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
     
    if (
      refinementCriterion==::exahype2::RefinementCommand::Refine 
      and 
      tarch::la::smallerEquals( tarch::la::max( marker.h() ), repositories::{{SOLVER_INSTANCE}}.MinAdmissiblePatchH )
    ) {
      logDebug( "touchCellFirstTime(...)", "drop refine instructions, as mesh would be too small" );
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
    else if (refinementCriterion==::exahype2::RefinementCommand::Erase and 3.0* tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.MaxAdmissiblePatchH ) {
      refinementCriterion = ::exahype2::RefinementCommand::Keep;
    } 
  }
  else {
    refinementCriterion = ::exahype2::RefinementCommand::Keep;
  }
  {% endif %}
    
  _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, {{EVENT_LIFETIME}} );
  logTraceOutWith1Argument( "touchCellFirstTime(...)", toString(refinementCriterion) );
  """
  
    
  def __init__(self, 
               solver, 
               guard, 
               build_up_new_refinement_instructions, 
               implement_previous_refinement_instructions,
               called_by_grid_construction, 
               event_lifetime=2,
               ):
    """
    
    :: Attributes
    
    _implement_previous_refinement_instructions: Boolean
      This name might be misleading. Consult exahype2::RefinementControl for
      a description of the control flow. This flag controls if instructions
      are picked up from the RefinementControl database.
      
    
    :: Arguments
    
    guard: C++ expression which evaluates to true or false
      A per cell decision whether we should study a cell or not.
    
    build_up_new_refinement_instructions: Boolean
      See remarks on multistep realisation of AMR in C++ class 
      exahype2::RefinementControl.
        
    implement_previous_refinement_instructions: Boolean
      See remarks on multistep realisation of AMR in C++ class 
      exahype2::RefinementControl.
      
    event_lifetime: Int
      See setter below
    
    """
    AbstractRKFDActionSet.__init__(self,solver)
    self.guard                                       = guard
    self._build_up_new_refinement_instructions       = build_up_new_refinement_instructions
    self._implement_previous_refinement_instructions = implement_previous_refinement_instructions
    self._event_lifetime                             = event_lifetime
    self._called_by_grid_construction                = called_by_grid_construction

  
  def get_body_of_getGridControlEvents(self):
    if self._implement_previous_refinement_instructions:
      return """
    return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();
""" 
    else:
      return """
    return std::vector< peano4::grid::GridControlEvent >();
"""      


  def get_body_of_operation(self,operation_name):
    result = ""
    if self._build_up_new_refinement_instructions and operation_name==peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  _localRefinementControl.clear();
"""

    if self._build_up_new_refinement_instructions and operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = """
  ::exahype2::RefinementControlService::getInstance().merge( _localRefinementControl );
"""
    
    if self._build_up_new_refinement_instructions and operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      if self._solver._patch.dim[0] != self._solver._patch.dim[1]:
        raise Exception( "Error: Can only handle square patches." )
      
      d[ "CELL_LABEL_NAME" ]    = self._solver._cell_label.name
      d[ "UNKNOWNS" ]           = str(self._solver._patch.no_of_unknowns)
      d[ "DOFS_PER_AXIS" ]      = str(self._solver._patch.dim[0])
      d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0])
      d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ] = str(self._solver._patch.no_of_unknowns * self._solver._patch.dim[0] * self._solver._patch.dim[0] * self._solver._patch.dim[0])
      d[ "CELL_ACCESSOR" ]                                = "fineGridCell" + self._solver._patch.name
      d[ "PREDICATE" ]          = "not marker.willBeRefined() and not marker.hasBeenRefined() and " + self.guard
      d[ "EVENT_LIFETIME"]      = self._event_lifetime
      d[ "GRID_CONSTRUCTION" ]  = self._called_by_grid_construction

      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = jinja2.Template( self.TemplateAMR ).render(**d)

    return result

  
  @property 
  def event_lifetime(self):
    return self._event_lifetime


  @event_lifetime.setter
  def event_lifetime(self,value):
    """
    
     By default, a refinement/coarsening event is only alive for one grid sweep. 
     After that one, the set of refine/coarsen commands is reset and we start
     again. If you work with local time stepping, subcycling, multiphysics codes,
     you might want to keep an event for more steps. In this case, you have to 
     invoke this setter.
     
    """
    self._event_lifetime = value


  def get_attributes(self):
    return """
    ::exahype2::RefinementControl         _localRefinementControl;
"""


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super( AdaptivityCriterion, self ).get_includes() + """
#include "exahype2/RefinementControlService.h"
"""
    
