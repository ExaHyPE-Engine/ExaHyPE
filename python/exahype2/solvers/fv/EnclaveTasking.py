# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV       import FV
from .PDETerms import PDETerms
 
import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor
from exahype2.solvers.fv.actionsets.AbstractFVActionSet import AbstractFVActionSet


class UpdateCell(ReconstructPatchAndApplyFunctor):
  TemplateUpdateCell = jinja2.Template( """
    double cellTimeStepSize = -1.0;
    double cellTimeStamp    = -1.0;
     
    {{PREPROCESS_RECONSTRUCTED_PATCH}}
    
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
      cellTimeStepSize,
      reconstructedPatch,
      targetPatch
    );
  }
  else { // is an enclave cell
    assertion( marker.isEnclaveCell() );
    auto newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
      marker,
      cellTimeStamp,
      cellTimeStepSize,
      reconstructedPatch
    );
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );

    peano4::parallel::Tasks spawn(
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );
  }
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
      reconstructed_array_memory_location = solver._reconstructed_array_memory_location,
      guard  = "not marker.isRefined() and (" + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "repositories::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")",
      add_assertions_to_halo_exchange = True
    )
    #self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)

    self._solver    = solver


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "peano4/parallel/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
""" + self._solver._get_default_includes() + self._solver.get_user_includes()


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
  Template = """
  if ( marker.isEnclaveCell() and not marker.isRefined() and repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();

    if ( taskNumber>=0 ) {
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );
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
"""
  def __init__(self,solver):
    AbstractFVActionSet.__init__(self,solver)
    self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "LABEL_NAME" ] = self.label_name      
      result = jinja2.Template(self.Template).render(**d)
      pass 
    return result


class EnclaveTasking( FV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
    """
    
     Not so nice. I have to store this field as I later rely on get_name_of_global_instance()
     which uses this field. 
     
    """
    self._name = name
    
    
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False
    
    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""
    
    self._source_term_call          = "#error Not yet defined"
    self._Riemann_solver_call       = "#error Not yet defined"
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""
    
    self._constructor_implementation = ""
    
    self._initialisation_sweep_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
      
    self._first_iteration_after_initialisation_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition" + \
    ")"

    self._primary_sweep_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    self._primary_sweep_or_plot_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"

    self._primary_or_initialisation_sweep_predicate= "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    self._primary_or_grid_construction_or_initialisation_sweep_predicate= "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
      ")"

    self._secondary_sweep_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary" + \
      ")"

    self._secondary_sweep_or_grid_construction_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
      ")"

    self._secondary_sweep_or_grid_initialisation_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"

    self._secondary_sweep_or_grid_initialisation_or_plot_predicate = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"
 
    super(EnclaveTasking, self).__init__(name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
     
    self._solver_template_file_class_name     = "EnclaveTasking"
      

  def create_data_structures(self):
    """
    
    This routine does not really add new data, but it heavily tailors when data are
    stored, exchanged, ... Each generator has some guard attributes, i.e. some predicates,
    which control when data is stored, sent, received. The routine takes these guards
    and rewires them to the local predicates of this object. If you alter these predicates
    further, you have to alter them before you invoke this class' create_data_structures().
    
    """
    FV.create_data_structures(self)
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_predicate() + " and (" + \
      self._secondary_sweep_or_grid_initialisation_or_plot_predicate + " or marker.isSkeletonCell())"
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_predicate() + " and (" + \
      self._primary_sweep_or_plot_predicate + " or marker.isSkeletonCell())"
    
    self._patch_overlap_old.generator.store_persistent_condition   = self._store_face_data_default_predicate() + " and " + self._secondary_sweep_or_grid_initialisation_or_plot_predicate
    self._patch_overlap_old.generator.load_persistent_condition    = self._load_face_data_default_predicate()  + " and " + self._primary_sweep_or_plot_predicate

    #self._patch_overlap_old.generator.send_condition               = self._initialisation_sweep_predicate
    #self._patch_overlap_old.generator.receive_and_merge_condition  = self._first_iteration_after_initialisation_predicate

    self._patch_overlap_new.generator.store_persistent_condition   = self._store_face_data_default_predicate() + " and " + self._primary_sweep_predicate
    self._patch_overlap_new.generator.load_persistent_condition    = self._load_face_data_default_predicate()  + " and " + self._secondary_sweep_predicate

    #self._patch_overlap_new.generator.send_condition               = "true"
    #self._patch_overlap_new.generator.receive_and_merge_condition  = "true"


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
    FV.create_action_sets(self)
    self._action_set_update_cell = UpdateCell(self)

    #
    # AMR and adjust cell have to be there always, i.e. also throughout 
    # the grid construction.
    #
    
    # @todo Not in SingleSweep yet
    #self._action_set_AMR.predicate                                 = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_predicate
    #self._action_set_AMR_commit_without_further_analysis.predicate = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_predicate
    #self._action_set_handle_boundary.predicate                     = self._store_face_data_default_predicate() + " and " + self._primary_or_initialisation_sweep_predicate
    
    #self._action_set_project_patch_onto_faces            = ProjectPatchOntoFaces(self,
    #  self._store_cell_data_default_predicate() + " and (" + \
    #     "(repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
    #  "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
    #  "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
    #  "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
    #  ")"
    #)
    #self._action_set_copy_new_patch_overlap_into_overlap = CopyNewPatchOverlapIntoCurrentOverlap(self, self._store_face_data_default_predicate() + " and " + self._secondary_sweep_or_grid_initialisation_predicate)

    self._merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)                                                                                 
    
    #self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = DynamicAMR( 
    #  patch                       = self._patch,
    #  patch_overlap_interpolation = self._patch_overlap, 
    #  patch_overlap_restriction   = self._patch_overlap_new,
    #  interpolate_guard           = self._primary_sweep_predicate,
    #  restrict_guard              = self._primary_or_initialisation_sweep_predicate,
      #clear_guard                 = self._primary_or_initialisation_sweep_predicate,
      #interpolate_guard           = "not marker.isRefined() and " + self._primary_or_initialisation_sweep_predicate,
      #restrict_guard              = "not marker.isRefined() and " + self._secondary_sweep_or_grid_initialisation_predicate,
      #clear_guard                 = "not fineGridFaceLabel.getBoundary() and " + self._primary_or_initialisation_sweep_predicate,
    #  clear_guard                 = "not marker.isRefined() and " + self._primary_or_initialisation_sweep_predicate,
    #  additional_includes         = """
#include "../repositories/SolverRepository.h"
#"""      
    #)

  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "peano4/parallel/Tasks.h"
#include "../repositories/SolverRepository.h"
"""    

    
  def set_implementation(self,
    boundary_conditions, refinement_criterion, initial_conditions,
    memory_location,
    use_split_loop
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

    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

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
    d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]     = jinja2.Template(self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH" ]          = jinja2.Template(self._postprocess_updated_patch, undefined=jinja2.DebugUndefined).render( **d )
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


  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    """
  
    Most subclasses will redefine/overwrite this operation as they have
    to incorporate the kernel into their generated stuff
  
    """
    self._preprocess_reconstructed_patch = kernel
    self.create_data_structures()
    self.create_action_sets()


  def set_postprocess_updated_patch_kernel(self, kernel):
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
    self._postprocess_updated_patch += kernel
    self.create_data_structures()
    self.create_action_sets()


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    FV.add_actions_to_create_grid(self,step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.UpdateCellLabel( self._name ) )


  def add_actions_to_init_grid(self, step):
    FV.add_actions_to_init_grid(self,step)
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
    FV.add_actions_to_perform_time_step(self,step)
    step.add_action_set( exahype2.grid.UpdateCellLabel(self._name) ) 
    step.add_action_set( self._merge_enclave_task_outcome )
 
