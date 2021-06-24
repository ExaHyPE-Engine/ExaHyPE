# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
import peano4
import exahype2

import jinja2

from .PDETerms import PDETerms


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
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)

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


class EnclaveTaskingFV( FV ):
  def __init__(self, name, patch_size, overlap, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._source_term_implementation          = PDETerms.Empty_Implementation
    
    self._name = name

    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False

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
      
    FV.__init__(self, name, patch_size, overlap, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)


  def create_data_structures(self):
    """
    
    This routine does not really add new data, but it heavily tailors when data are
    stored, exchanged, ... Each generator has some guard attributes, i.e. some predicates,
    which control when data is stored, sent, received. The routine takes these guards
    and rewires them to the local predicates of this object. If you alter these predicates
    further, you have to alter them before you invoke this class' create_data_structures().
    
    """
    FV.create_data_structures(self)
    self._cell_sempahore_label = exahype2.grid.create_enclave_cell_label( self._name )

    self._patch.generator.store_persistent_condition = self._store_cell_data_default_predicate() + " and (" + \
      self._secondary_sweep_or_grid_initialisation_or_plot_predicate + " or marker.isSkeletonCell())"
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_predicate() + " and (" + \
      self._primary_sweep_or_plot_predicate + " or marker.isSkeletonCell())"
    
    self._patch_overlap.generator.store_persistent_condition   = self._store_face_data_default_predicate() + " and " + self._secondary_sweep_or_grid_initialisation_or_plot_predicate
    self._patch_overlap.generator.load_persistent_condition    = self._load_face_data_default_predicate()  + " and " + self._primary_sweep_or_plot_predicate

    self._patch_overlap.generator.send_condition               = self._initialisation_sweep_predicate
    self._patch_overlap.generator.receive_and_merge_condition  = self._first_iteration_after_initialisation_predicate

    self._patch_overlap_new.generator.store_persistent_condition   = self._store_face_data_default_predicate() + " and " + self._primary_sweep_predicate
    self._patch_overlap_new.generator.load_persistent_condition    = self._load_face_data_default_predicate()  + " and " + self._secondary_sweep_predicate

    self._patch_overlap_new.generator.send_condition               = "true"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "true"

    self._patch_overlap.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap_new.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    FV.add_to_Peano4_datamodel(self,datamodel, verbose)
    datamodel.add_cell(self._cell_sempahore_label)

  
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

    #
    # AMR and adjust cell have to be there always, i.e. also throughout 
    # the grid construction.
    #
    self._action_set_AMR.predicate                                 = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_predicate
    self._action_set_AMR_commit_without_further_analysis.predicate = "not marker.isRefined() and " + self._secondary_sweep_or_grid_construction_predicate
    self._action_set_handle_boundary.predicate                     = self._store_face_data_default_predicate() + " and " + self._primary_or_initialisation_sweep_predicate
    
    self._action_set_project_patch_onto_faces            = ProjectPatchOntoFaces(self,
      self._store_cell_data_default_predicate() + " and (" + \
         "(repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
      "or (repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"
    )
    self._action_set_copy_new_patch_overlap_into_overlap = CopyNewPatchOverlapIntoCurrentOverlap(self, self._store_face_data_default_predicate() + " and " + self._secondary_sweep_or_grid_initialisation_predicate)

    self._merge_enclave_task_outcome = MergeEnclaveTaskOutcome(self)                                                                                 
    
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = DynamicAMR( 
      patch                       = self._patch,
      patch_overlap_interpolation = self._patch_overlap, 
      patch_overlap_restriction   = self._patch_overlap_new,
      interpolate_guard           = self._primary_or_initialisation_sweep_predicate,
      restrict_guard              = self._primary_or_initialisation_sweep_predicate,
      #clear_guard                 = self._primary_or_initialisation_sweep_predicate,
      #interpolate_guard           = "not marker.isRefined() and " + self._primary_or_initialisation_sweep_predicate,
      #restrict_guard              = "not marker.isRefined() and " + self._secondary_sweep_or_grid_initialisation_predicate,
      #clear_guard                 = "not fineGridFaceLabel.getBoundary() and " + self._primary_or_initialisation_sweep_predicate,
      clear_guard                 = "not marker.isRefined() and " + self._primary_or_initialisation_sweep_predicate,
      additional_includes         = """
#include "../repositories/SolverRepository.h"
"""      
    )
    
    
  def set_implementation(self,
    flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete,
    use_split_loop          = False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the 
      set value so far won't be overwritten.
      
      Please note that not all options are supported by all solvers. You 
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.
    """
    if flux!=None:
      self._flux_implementation                       = flux
    if ncp!=None:
      self._ncp_implementation                        = ncp
    if eigenvalues!=None:    
      self._eigenvalues_implementation                = eigenvalues
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    if source_term!=None:
      self._source_term_implementation                = source_term

    if memory_location!=None:
      self._reconstructed_array_memory_location = memory_location
    if use_split_loop!=None:
      self._use_split_loop = use_split_loop

    if self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete and \
       self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete and \
       self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without immedidate (call stack) free chosen. This will lead into a segmentation fault" )

    self.create_action_sets()
    

  def set_preprocess_reconstructed_patch_kernel(self,kernel):
    self._preprocess_reconstructed_patch = kernel
    self.create_action_sets()


  def set_postprocess_updated_patch_kernel(self,kernel):
    self._postprocess_updated_patch = kernel
    self.create_action_sets()

  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "peano4/parallel/Tasks.h"
"""    


  def add_entries_to_text_replacement_dictionary(self,d):
    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    d[ "SOURCE_TERM_IMPLEMENTATION"]          = self._source_term_implementation

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D" ] = (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_VOLUMES_PER_AXIS"]+2) * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    
    d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.EnclaveLabels.get_attribute_name(self._name)

    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "delete[] reconstructedPatch;"
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "::tarch::freeMemory(reconstructedPatch, ::tarch::MemoryLocation::Heap);"
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "::tarch::freeMemory(reconstructedPatch, ::tarch::MemoryLocation::ManagedAcceleratorMemory);"
    pass


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    FV.add_actions_to_create_grid(self,step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.EnclaveLabels( self._name ) )


  def add_actions_to_init_grid(self, step):
    FV.add_actions_to_init_grid(self,step)
    step.add_action_set( exahype2.grid.EnclaveLabels( self._name ) )
    step.add_action_set( ProjectPatchOntoFaces(self, self._store_cell_data_default_predicate(), False) )


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
      action sets: reconstruct_patch_and_apply_FV_kernel, exahype2.grid.EnclaveLabels
      and roll_over_enclave_task_results.
 
    """
    FV.add_actions_to_perform_time_step(self,step)
    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    step.add_action_set( self._merge_enclave_task_outcome )
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    FV.add_use_data_statements_to_Peano4_solver_step(self,step)
    step.use_cell(self._cell_sempahore_label)
