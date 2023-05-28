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
  """!
  
  Update cell in primary sweep
  
  This action set is used in the primary sweeps only. In the secondary sweep, 
  its counterpart, the action set UpdateCell, is active and works in all data
  computed by tasks which have been spawned here.
  
  We extend the superclass ReconstructPatchAndApplyFunctor and hence have 
  access to the reconstructed data including a halo layer of one. Furthermore,
  the superclass provides us with a guard which we should use, as this guard
  ensures that we reconstruct the patch plus halo if and only if certain 
  conditions are met. By default, we compuate only on unrefined octants.
  
  Our condition whether to spawn a task or to compute the new time step 
  data immediately depends on peano4::datamanagement::CellMarker::willBeSkeletonCell().
  If this predicate holds, we compute stuff straightaway. Otherwise, we 
  span a new enclave task. While this check does the job in almost all
  cases, there are special situations where you might want to label more
  cells as skeleton cells. @ref benchmarks_exahype2_ccz4_single_black_hole "Peano's single black hole test" provides
  an example. If so, you 
  
  
  ## Modify templates
  
  You can alter the template. Typical codes augment _Template_TouchCellFirstTime_Preamble
  for example. However, there are two things to consider:
  
  - _Template_TouchCellFirstTime_Preamble is a member of the class and 
    initialised in the constructor. 
  - The constructor is used to create an object in the create_action_sets()
    of the using class.
  
  If you want to alter the preamble, you thus should specialise 
  create_action_sets() and invoke the supertype's create_action_sets(). After
  that, alter self._action_set_update_cell._Template_TouchCellFirstTime_Preamble.
  I recommend to add an entry and not to replace the preamble, as the 
  preamble already consists meaningful code.
  
  """

  
  TemplateUpdateCell = jinja2.Template( """
  double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  // Set the following two paramters
  // double timeStepSize
  {{COMPUTE_TIME_STEP_SIZE}}
    
  {{PREPROCESS_RECONSTRUCTED_PATCH}}
    
  assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStepSize, timeStamp );
  assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStepSize, timeStamp );

  ::exahype2::fv::validatePatch(
      oldQWithHalo,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  ); // previous time step has to be valid

  if ( marker.willBeSkeletonCell() ) {
    const double maxEigenvalue = tasks::{{SOLVER_NAME}}EnclaveTask::applyKernelToCell(
      marker,
      timeStamp,
      timeStepSize,
      oldQWithHalo,
      newQ
    );
    
    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::SkeletonTask );
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
  }
  else { // is an enclave cell
    assertion( marker.willBeEnclaveCell() );
    assertion( not marker.willBeRefined() );
    auto newEnclaveTask = new tasks::{{SOLVER_NAME}}EnclaveTask(
      marker,
      timeStamp,
      timeStepSize,
      oldQWithHalo
    );
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);

    tarch::multicore::spawnTask( newEnclaveTask );
  }
  """)


  def __init__(self,solver):
    ReconstructPatchAndApplyFunctor.__init__(self,
      patch = solver._patch,
      # todo hier muessen beide rein, denn ich muss ja interpolieren -> machen die anderen Codes dann
      patch_overlap = solver._patch_overlap_new,
      functor_implementation = "<not yet set - will do this later>",
      reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete,
      # todo Dokumentieren, dass net willBeRefined(), weil wir ja das brauchen wenn wir runtergehen
      guard  = "not marker.hasBeenRefined() and (" + \
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
#include "tarch/multicore/Tasks.h"
#include "repositories/SolverRepository.h"
#include "tasks/""" + self._solver._name + """EnclaveTask.h"
""" + self._solver._get_default_includes() + self._solver.user_action_set_includes


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def _add_action_set_entries_to_dictionary(self,d):
    """
    
    First ask the solver to add its symbols, and then re-construct the 
    functor which should not contain any symbols after that anymore.
    Next, we call the superclass routine which supplements all those
    instructions that any reconstruction wants to have.
    
    """

    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
 
    self.functor_implementation = self.TemplateUpdateCell.render(**d)

    super( UpdateCell, self )._add_action_set_entries_to_dictionary(d)


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
  Template = """
  if ( 
    not marker.hasBeenRefined()
    and 
    {{GUARD}}
    and
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary 
  ) {
    const int taskNumber = fineGridCell{{LABEL_NAME}}.getSemaphoreNumber();
    if ( marker.hasBeenEnclaveCell() and taskNumber>=0 ) {
      double maxEigenvalue;
      ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, fineGridCell{{UNKNOWN_IDENTIFIER}}.value, maxEigenvalue );

      ::exahype2::fv::validatePatch(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0,
        std::string(__FILE__) + ": " + std::to_string(__LINE__) + "; marker=" + marker.toString()
      );
      
      {{COMPUTE_NEW_TIME_STEP_SIZE}}

      fineGridCell{{LABEL_NAME}}.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    }
    
    if ( fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated() ) {
      double*  newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
      
      {{POSTPROCESS_UPDATED_PATCH}}

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
    super( MergeEnclaveTaskOutcome, self ).__init__(solver)
    self.label_name = exahype2.grid.UpdateCellLabel.get_attribute_name(solver._name)
    self.guard      = "true"
    self.priority   = -1


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
  def __init__(self,
               name,
               patch_size,
               overlap,
               unknowns,
               auxiliary_variables,
               min_volume_h,
               max_volume_h,
               plot_grid_properties,
               pde_terms_without_state: bool,
               kernel_namespace):
    """
     Not so nice. I have to store this field as I later rely on get_name_of_global_instance()
     which uses this field.
    """
    self._name = name

    self._fused_compute_kernel_call_cpu       = "#error Not yet defined. Set self._fused_Riemann_solver_call in your Python solver class."
    self._fused_compute_kernel_call_gpu       = "#error Not yet defined. Set self._fused_Riemann_solver_call in your Python solver class."
    self._pde_terms_without_state             = pde_terms_without_state

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
 
    super(EnclaveTasking, self).__init__(name,
                                         patch_size,
                                         overlap,
                                         unknowns,
                                         auxiliary_variables,
                                         min_volume_h,
                                         max_volume_h,
                                         plot_grid_properties,
                                         kernel_namespace)
    self._solver_template_file_class_name = "EnclaveTasking"

    additional_includes= """
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
"""

    self.add_user_action_set_includes( additional_includes )


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
    """!
    
    Make storage and loading more restrictive such that enclave data are not held in-between primary and secondary sweep
    
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
    
    @todo There's no equivalent routine for FD4 and RKDG yet
    
    """
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_guard() + " and (" + \
      self._secondary_sweep_or_grid_initialisation_or_plot_guard + " or marker.willBeSkeletonCell())"
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_guard() + " and (" + \
      self._primary_sweep_or_plot_guard + " or marker.hasBeenSkeletonCell())"


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

    AMR and adjust cell have to be there always, i.e. also throughout 
    the grid construction. But the criterion is something that we only
    evaluate in the secondary sweep. That's when we have an updated/changed time step.
    If we identify coarsening and refinement instructions in the secondary sweep, the 
    next primary one will actually see them and trigger the update. That is, the 
    subsequent secondary switch will actually implement the grid changes, and we can 
    evaluate the criteria again.

    For dynamic AMR, this implies that we have to ensure that all changed grid parts
    are labelled as skeleton cells. This way, we can implement the AMR properly, we 
    ensure that all the enclaves run in parallel, and we know that all data is held
    persistently on the stacks.
    """
    super(EnclaveTasking, self).create_action_sets()

    self._action_set_update_cell                = UpdateCell(self)
    self._action_set_merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)                                                                                 

    self._action_set_initial_conditions.guard                       = self._action_set_initial_conditions.guard
    self._action_set_initial_conditions_for_grid_construction.guard = self._action_set_initial_conditions_for_grid_construction.guard
    self._action_set_AMR.guard                                      = self._secondary_sweep_guard
    self._action_set_AMR_commit_without_further_analysis.guard      = self._secondary_sweep_guard
    # We do not set the guard of the secondary sweep
    # self._action_set_postprocess_solution.guard                     = self._secondary_sweep_guard

    self._action_set_handle_boundary.guard                          = self._store_face_data_default_guard() + " and " + self._primary_or_initialisation_sweep_guard
    self._action_set_project_patch_onto_faces.guard                 = self._store_cell_data_default_guard() + " and (" + \
         "(repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.willBeSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.willBeSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.willBeEnclaveCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"
    self._action_set_roll_over_update_of_faces.guard = self._store_face_data_default_guard() + " and " + self._secondary_sweep_or_grid_initialisation_guard
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guard = self._store_cell_data_default_guard() + " and " + self._secondary_sweep_or_grid_initialisation_guard


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
    
    if refinement_criterion==exahype2.solvers.fv.PDETerms.None_Implementation:
      assert False, "refinement criterion cannot be none"
    if initial_conditions==exahype2.solvers.fv.PDETerms.None_Implementation:
      assert False, "initial conditions cannot be none"

    if memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete and \
       memory_location!=None:
      raise Exception( "only valid memory mode for enclave tasking is heap without a delete, as enclave tasks delete memory themselves through the tarch. Selected mode=" + str(solver._reconstructed_array_memory_location) )

    self.create_action_sets()


  def add_entries_to_text_replacement_dictionary(self,d):
    """
     d: Dictionary of string to string
        in/out argument
    """
    d[ "FUSED_COMPUTE_KERNEL_CALL_CPU"] = jinja2.Template(self._fused_compute_kernel_call_cpu, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FUSED_COMPUTE_KERNEL_CALL_GPU"] = jinja2.Template(self._fused_compute_kernel_call_gpu, undefined=jinja2.DebugUndefined).render( **d )

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])

    d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.UpdateCellLabel.get_attribute_name(self._name)
    d[ "STATELESS_PDE_TERMS" ]  = self._pde_terms_without_state


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    super(EnclaveTasking, self).add_actions_to_create_grid(step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.UpdateCellLabel( self._name ) )


  def add_actions_to_init_grid(self, step):
    super(EnclaveTasking, self).add_actions_to_init_grid(step)
    step.add_action_set( exahype2.grid.UpdateCellLabel( self._name ) )


  def add_actions_to_perform_time_step(self, step):
    """!
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


  def add_implementation_files_to_project(self,namespace,output, dimensions):
    """!
    
      Add the enclave task for the GPU
      
    """

    super(EnclaveTasking, self).add_implementation_files_to_project(namespace, output, dimensions)
    templatefile_prefix = os.path.join(os.path.dirname(os.path.realpath(__file__)), "EnclaveTasking.EnclaveTask.template")

    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)
    
    # Some includes might logically belong into the action sets, but now they are 
    # 'outsourced' into the enclave task. So we manually add it here.
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.user_solver_includes
    implementationDictionary[ "SOLVER_INCLUDES" ] += self.user_action_set_includes

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
    output.makefile.add_cpp_file( "tasks/" + task_name + ".cpp", generated=True )

    if self._pde_terms_without_state:
        # We need to explicitly link objects in gpu mode, since PEANO_SOURCE_DIR is gone,
        # this hack has to suffice
        peanodir = templatefile_prefix.split("python/exahype2",1)[0]
        output.makefile.d["PEANODIR"] = peanodir
        exahype2.gpu.add_exahype_objects(output.makefile)


  def _enclave_task_name(self): return "{}EnclaveTask".format(self._name)


  def set_pde_terms_without_state(self, value):
    self._pde_terms_without_state = value
