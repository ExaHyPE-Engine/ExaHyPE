# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV       import FV
from .PDETerms import PDETerms
 
import peano4
import exahype2

import jinja2
import os

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor
from exahype2.solvers.fv.actionsets.AbstractFVActionSet import AbstractFVActionSet


class UpdateCell(ReconstructPatchAndApplyFunctor):
  TemplateUpdateCell = jinja2.Template( """
  double cellTimeStepSize = -1.0;
  double cellTimeStamp    = -1.0;
     
  {{PREPROCESS_RECONSTRUCTED_PATCH_THROUGHOUT_SWEEP}}
  {{COMPUTE_TIME_STEPSIZE}}
    
  assertion2( tarch::la::greaterEquals( cellTimeStepSize, 0.0 ), cellTimeStepSize, cellTimeStamp );
  assertion2( tarch::la::greaterEquals( cellTimeStamp, 0.0 ), cellTimeStepSize, cellTimeStamp );

  const double usedTimeStepSize = cellTimeStepSize;
    
  ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid

  if (marker.isSkeletonCell()) {
    tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
      marker,
      cellTimeStamp,
      usedTimeStepSize,
      reconstructedPatch,
      targetPatch
    );

    ::exahype2::fv::validatePatch(
      targetPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      0, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // outcome has to be valid

    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::SkeletonTask );
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(cellTimeStamp + usedTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(usedTimeStepSize);
  }
  else { // is an enclave cell
    assertion( marker.isEnclaveCell() );
    auto newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
      marker,
      cellTimeStamp,
      usedTimeStepSize,
      reconstructedPatch
    );
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

    peano4::parallel::Tasks spawn(
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );
  }
  
  fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(cellTimeStamp + usedTimeStepSize);
  fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(usedTimeStepSize);
  """)

  def __init__(self,solver):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)

    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      # todo hier muessen beide rein, denn ich muss ja interpolieren
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = self.TemplateUpdateCell.render(**d),
      reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete,
      guard  = "not marker.isRefined() and not marker.hasBeenRefined() and (" + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")",
      add_assertions_to_halo_exchange = True
    )

    self._Template_TouchCellFirstTime_Preamble = """
  if (
    repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()==""" + solver._name + """::SolverState::Primary or 
    repositories::""" + solver.get_name_of_global_instance() + """.getSolverState()==""" + solver._name + """::SolverState::PrimaryAfterGridInitialisation
  ) {{
    fineGridCell""" + solver._name + """CellLabel.setHasUpdated(false);
  }}
""" + self._Template_TouchCellFirstTime_Preamble

    self._solver    = solver


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
""" + self._solver._get_default_includes() + self._solver.get_user_action_set_includes()


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
  Template = """
  if ( 
    not marker.isRefined() 
    and 
    {{GUARD}}
    and
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
  ) {
    if ( marker.isEnclaveCell() ) {
      const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();

      if ( taskNumber>=0 ) {
        ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );
        fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      }
      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      
      ::exahype2::fv::validatePatch(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0,
        std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
      );
    }
    
    if ( fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() ) {
      double* targetPatch = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
      
      {{POSTPROCESS_UPDATED_PATCH_THROUGHOUT_SWEEP}}
      
      repositories::{{SOLVER_INSTANCE}}.update(
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
    else {
      repositories::{{SOLVER_INSTANCE}}.update(
        0.0,
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp(),
        marker.h()(0)
      );
    }
  }
"""
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)
    self.guard      = "true"


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      d[ "GUARD" ]      = self.guard
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


class EnclaveTasking( FV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties, use_gpu):
    """
    
     Not so nice. I have to store this field as I later rely on get_name_of_global_instance()
     which uses this field. 
     
    """
    self._name = name
    
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._use_split_loop                      = False
    
    self._compute_time_step_size              = "#error Not yet defined"

    self._preprocess_reconstructed_patch_throughout_sweep      = ""
    self._postprocess_updated_patch_throughout_sweep           = ""
    self._preprocess_reconstructed_patch_in_enclave_task       = ""
    self._postprocess_updated_patch_in_enclave_task            = ""
    self._preprocess_reconstructed_patch_in_skeleton_cell      = ""
    self._postprocess_updated_patch_in_skeleton_cell           = ""
    
    self._source_term_call                = "#error Not yet defined. Set self._source_term_call in your Python solver class."
    self._Riemann_solver_call             = "#error Not yet defined. Set self._Riemann_solver_call in your Python solver class."
    self._fused_Riemann_solver_call       = "#error Not yet defined. Set self._fused_Riemann_solver_call in your Python solver class."
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""
    
    self._use_gpu = use_gpu
    
    self._constructor_implementation = ""
    
    self._initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
      
    self._first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition" + \
    ")"

    self._primary_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    self._primary_sweep_or_plot_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"

    self._primary_or_initialisation_sweep_guard= "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    self._primary_or_grid_construction_or_initialisation_sweep_guard= "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
      ")"

    self._secondary_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary" + \
      ")"

    self._secondary_sweep_or_grid_construction_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
      ")"

    self._secondary_sweep_or_grid_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"

    self._secondary_sweep_or_grid_initialisation_or_plot_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"
 
    super(EnclaveTasking, self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties)
     
    self._solver_template_file_class_name     = "EnclaveTasking"
      

  def create_data_structures(self):
    """
    
    This routine does not really add new data, but it heavily tailors when data are
    stored, exchanged, ... Each generator has some guard attributes, i.e. some guards,
    which control when data is stored, sent, received. The routine takes these guards
    and rewires them to the local guards of this object. If you alter these guards
    further, you have to alter them before you invoke this class' create_data_structures().
    
    """
    super(EnclaveTasking, self).create_data_structures()
    
    self._patch_overlap_new.generator.send_condition               = self._secondary_sweep_or_grid_initialisation_or_plot_guard
    self._patch_overlap_new.generator.receive_and_merge_condition  = self._primary_sweep_or_plot_guard

    self._patch_overlap_old.generator.send_condition               = self._initialisation_sweep_guard
    self._patch_overlap_old.generator.receive_and_merge_condition  = self._first_iteration_after_initialisation_guard


  def _optimise_patch_storage_for_global_time_stepping(self):
    """
  
    If you work with global time stepping, you know that each enclave cell will
    be updated per grid traversal duo. Consequently, every enclave cell's data
    doesn't have to be stored in-between two grid traversals - we know that it
    is currently outsourced to a task.
  
    Things are different if we use local time stepping, as there will always be
    cells that are currently processed, and then there are cells which are not 
    updated and which we consequently should keep.
    
    If you want to have this optimisation, you have to call this routine 
    explicitly in create_data_structures(). By default, we always store the 
    patches all the time.
  
    """
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_guard() + " and (" + \
      self._secondary_sweep_or_grid_initialisation_or_plot_guard + " or marker.isSkeletonCell())"
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_guard() + " and (" + \
      self._primary_sweep_or_plot_guard + " or marker.isSkeletonCell())"


  def create_action_sets(self):
    """
    
    Adaptive mesh handing
    
    Adaptive meshes require us to clear the patch overlaps and to restrict/interpolate. 
    Obviously, there's no need to do this for a refined faces. So we can eliminate these
    cases a priori. Furthermore, we clear faces only in the primary sweep. We know that
    either the primary sweep (for skeleton) or the secondary sweep (for enclaves) will
    write in proper data into anything that's cleared, and we know that restriction only
    has to happen after the primary sweep, as all cells next to an adaptivity boundary
    are skeleton cells. 
    
    As pointed out, both interpolation and restriction are to be active for the first
    sweep only. We interpolate into hanging faces, and we have to restrict immediately
    again as they are non-persistent. The projection onto the (hanging) faces is also 
    happening directly in the primary sweep, as the cells adjacent to the hanging 
    face are skeleton cells.
        
    """
    super(EnclaveTasking, self).create_action_sets()

    self._action_set_update_cell                = UpdateCell(self)
    self._action_set_merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)                                                                                 

    #
    # AMR and adjust cell have to be there always, i.e. also throughout 
    # the grid construction.
    #
    
    self._action_set_initial_conditions.guard                       = self._action_set_initial_conditions.guard
    self._action_set_initial_conditions_for_grid_construction.guard = self._action_set_initial_conditions_for_grid_construction.guard
    self._action_set_AMR.guard                                 = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_guard
    self._action_set_AMR_commit_without_further_analysis.guard = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_guard
    self._action_set_handle_boundary.guard                     = self._store_face_data_default_guard() + " and " + self._primary_or_initialisation_sweep_guard
    self._action_set_project_patch_onto_faces.guard            = self._store_cell_data_default_guard() + " and (" + \
         "(repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"
    self._action_set_roll_over_update_of_faces.guard = self._store_face_data_default_guard() + " and " + self._secondary_sweep_or_grid_initialisation_guard
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guard = self._store_cell_data_default_guard() + " and " + self._secondary_sweep_or_grid_initialisation_guard
   

  def get_user_action_set_includes(self):
    return super(EnclaveTasking, self).get_user_action_set_includes() + """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
"""    

    
  def set_implementation(self,
    boundary_conditions, refinement_criterion, initial_conditions,
    memory_location,
    use_split_loop,
    additional_action_set_includes,
    additional_user_includes
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the 
      set value so far won't be overwritten.
    """
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions
    if memory_location      is not None:  self._reconstructed_array_memory_location       = memory_location
    if use_split_loop                  :  self._use_split_loop                            = use_split_loop

    if memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete and \
       memory_location!=None:
      raise Exception( "only valid memory mode for enclave tasking is heap without a delete, as enclave tasks delete memory themselves through the tarch. Selected mode=" + str(solver._reconstructed_array_memory_location) )

    self.user_action_set_includes += additional_action_set_includes
    self.user_solver_includes     += additional_user_includes

    self.create_action_sets()
    

  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

    d[ "SOURCE_TERM_CALL"]                    = jinja2.Template(self._source_term_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "RIEMANN_SOLVER_CALL"]                 = jinja2.Template(self._Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "RIEMANN_SOLVER_CALL"]                 = jinja2.Template(self._Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FUSED_RIEMANN_SOLVER_CALL"]           = jinja2.Template(self._fused_Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    
    d[ "COMPUTE_TIME_STEPSIZE" ]                               = jinja2.Template(self._compute_time_step_size, undefined=jinja2.DebugUndefined).render( **d )
    d[ "PREPROCESS_RECONSTRUCTED_PATCH_THROUGHOUT_SWEEP" ]     = jinja2.Template(self._preprocess_reconstructed_patch_throughout_sweep, undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH_THROUGHOUT_SWEEP" ]          = jinja2.Template(self._postprocess_updated_patch_throughout_sweep,      undefined=jinja2.DebugUndefined).render( **d )
    d[ "PREPROCESS_RECONSTRUCTED_PATCH_IN_ENCLAVE_TASK" ]      = jinja2.Template(self._preprocess_reconstructed_patch_in_enclave_task,  undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH_IN_ENCLAVE_TASK" ]           = jinja2.Template(self._postprocess_updated_patch_in_enclave_task,       undefined=jinja2.DebugUndefined).render( **d )
    d[ "PREPROCESS_RECONSTRUCTED_PATCH_IN_SKELETON_CELL" ]     = jinja2.Template(self._preprocess_reconstructed_patch_in_skeleton_cell, undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH_IN_SKELETON_CELL" ]          = jinja2.Template(self._postprocess_updated_patch_in_skeleton_cell,      undefined=jinja2.DebugUndefined).render( **d )
    
    d[ "ABSTRACT_SOLVER_USER_DECLARATIONS" ]  = jinja2.Template(self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DEFINITIONS" ]   = jinja2.Template(self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DECLARATIONS" ]           = jinja2.Template(self._solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DEFINITIONS" ]            = jinja2.Template(self._solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "START_TIME_STEP_IMPLEMENTATION" ]     = jinja2.Template(self._start_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FINISH_TIME_STEP_IMPLEMENTATION" ]    = jinja2.Template(self._finish_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "CONSTRUCTOR_IMPLEMENTATION"]          = jinja2.Template(self._constructor_implementation, undefined=jinja2.DebugUndefined).render( **d )

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    
    d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.UpdateCellLabel.get_attribute_name(self._name)
    d[ "USE_GPU" ] = self._use_gpu
    
    
  def set_preprocess_reconstructed_patch_kernel(self,kernel, can_offload_into_enclave_task = True ):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    if can_offload_into_enclave_task:
      self._preprocess_reconstructed_patch_in_enclave_task  += kernel
      self._preprocess_reconstructed_patch_in_skeleton_cell += kernel
    else:
      self._preprocess_reconstructed_patch_throughout_sweep += kernel
      
    self.create_data_structures()
    self.create_action_sets()


  def set_postprocess_updated_patch_kernel(self, kernel, can_offload_into_enclave_task = True):
    """

    Define a postprocessing routine over the data

    The postprocessing kernel often looks similar to the following code:

  {
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      enforceCCZ4constraints( targetPatch+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }


    Within this kernel, you have at least the following variables available:

    - targetPatch This is a pointer to the whole data structure (one large
        array).
        The patch is not supplemented by a halo layer.
    - reconstructedPatch This is a pointer to the data snapshot before the
        actual update. This data is combined with the halo layer, i.e. if you
        work with 7x7 patches and a halo of 2, the pointer points to a 11x11
        patch.
    - marker

    Furthermore, you can use all the symbols (via Jinja2 syntax) from
    _init_dictionary_with_default_parameters().

    kernel: String
      C++ code that holds the postprocessing kernel

    """
    if can_offload_into_enclave_task:
      self._postprocess_updated_patch_throughout_sweep += kernel
    else:
      self._postprocess_updated_patch_in_enclave_task  += kernel
      self._postprocess_updated_patch_in_skeleton_cell += kernel

    self.create_data_structures()
    self.create_action_sets()


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    super(EnclaveTasking, self).add_actions_to_create_grid(step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.UpdateCellLabel( self._name ) )


  def add_actions_to_init_grid(self, step):
    super(EnclaveTasking, self).add_actions_to_init_grid(step)
    step.add_action_set( exahype2.grid.UpdateCellLabel( self._name ) )


  def add_actions_to_perform_time_step(self, step):
    """
      Add enclave aspect to time stepping
      
      There's a bunch of different things to do to extend my standard solver
      into an enclave solver. In this operation, we add the runtime logic,
      i.e. what happens at which point.
      
      We need additional action sets that are
      triggered throughout the traversal in every second time step. I call this
      one task_based_implementation_primary_iteration or secondary, 
      respectively. One wraps the implementation of _HandleCellTemplate into a 
      task, the other communicates with the task bookkeeping only. Both rely on
      additional labels within the cell. We therefore end up with three new 
      action sets: reconstruct_patch_and_apply_FV_kernel, exahype2.grid.UpdateCellLabel
      and roll_over_enclave_task_results.
 
    """
    super(EnclaveTasking, self).add_actions_to_perform_time_step(step)
    step.add_action_set( self._action_set_merge_enclave_task_outcome )
 

  def add_implementation_files_to_project(self,namespace,output):
    """
      Add the enclave task for the GPU
    """

    super(EnclaveTasking, self).add_implementation_files_to_project(namespace, output)
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "EnclaveTasking.EnclaveTask.template")

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)
    
    # Some includes might logically belong into the action sets, but now they are 
    # 'outsourced' into the enclave task. So we manually add it here.
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.get_user_action_set_includes()

    task_name = self._enclave_task_name()
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      "{}.h".format(templatefile_prefix),
      "{}.cpp".format(templatefile_prefix),
      task_name,
      namespace + ["tasks"],
      "tasks",
      implementationDictionary,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp" )

    if self._use_gpu:
        # We need to explicitly link objects in gpu mode, since PEANO_SOURCE_DIR is gone,
        # this hack has to suffice
        peanodir = templatefile_prefix.split("python/exahype2",1)[0]
        output.makefile.d["PEANODIR"] = peanodir
        exahype2.gpu.add_exahype_objects(output.makefile)


  def _enclave_task_name(self): return "{}EnclaveTask".format(self._name)
