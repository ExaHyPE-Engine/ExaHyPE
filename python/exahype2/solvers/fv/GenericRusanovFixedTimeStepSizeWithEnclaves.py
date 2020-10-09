# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
from .AbstractAoSWithOverlap1  import AbstractAoSWithOverlap1
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSize import GenericRusanovFixedTimeStepSize 


class GenericRusanovFixedTimeStepSizeWithEnclaves( FV, AbstractAoSWithOverlap1 ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.None_Implementation, ncp=PDETerms.None_Implementation, plot_grid_properties=False):
    """
    
      Instantiate a generic FV scheme with an overlap of 1.
      
    A fixed time stepping scheme with enclave tasking
    
    This is a simple implementation of a FV scheme using a generic 
    Rusanov solver. It applies the concept of enclave tasking and 
    thus exhibits a higher concurrency level than a plain FV 
    counterpart. The price to pay is a higher memory pressure and 
    further admin overhead.
    
    Algorithmic workflow:
    
    The enclave tasking variant here is simpler than the original
    enclave tasking as proposed by Charrier et al. The reason that 
    we keep it simpler is that the baseline code scales better.
    Therefore, it is reasonable to keep the enclave complexity and
    overhead down more aggressively.
    
    The basic idea behind enclave tasking is that each time step 
    consists of two grid sweeps and that we distinguish enclave 
    tasks from skeleton cells. Skeleton cells are cells that are
    adjacent to a resolution transition or adjacent to a domain 
    boundary. The FV steps are distributed among these two sweeps
    as follows:
    
    image:: GenericRusanovFixedTimeStepSizeWithEnclaves_state-transitions.svg
    
    The variant of enclave tasking as it is discussed here has nothing
    in common with the fused tasking as discussed by Charrier and 
    Weinzierl.
    
    
    
    Methods:
  
    We extend the superclass' add_actions_to_perform_time_step(), 
    add_to_Peano4_datamodel() and add_use_data_statements_to_Peano4_solver_step().
    For the actions, I add a further action which administers the task
    spawning over the enclaves. I plug into the data model routines to 
    add the marker to the cell which holds the semaphore/cell number.      
    
    """
    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    AbstractAoSWithOverlap1.__init__(self, flux, ncp)

    # @todo Ein Haufen der Logik kann raus
    
    self._time_step_size = time_step_size
    
    primary_sweep_predicate_for_guard = "(" + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    primary_or_initialisation_sweep_predicate_for_guard = "(" + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"
    
    #
    # The roll over usually happens in the secondary iteration, as the NewQ 
    # then has arrived from other trees. As the roll-over is the last thing 
    # that we do, this means that the roll-over happens towards the end of 
    # the secondary sweep. 
    #
    # We need all projected data at the boundary to impose boundary conditions.
    # Therefore, it is important that we also swap over as last action in the 
    # initialisation. Otherwise, the boundary initialisation in the first 
    # primary sweep has no valid input data.
    #
    # After the initialisation, we do not have to roll stuff over, as we 
    # exchange the real Q (not the new Q) after the initialisation step.
    # Again, this works only as we roll over the new Q in the end of the 
    # initialisation. This roll-over precedes the boundary data exchange.
    #
    self._guard_copy_new_face_data_into_face_data = self._predicate_face_carrying_data() + " and (" + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation " + \
      ")"
    
    self._guard_adjust_cell = self._predicate_cell_carrying_data() + " and " + primary_or_initialisation_sweep_predicate_for_guard

    self._guard_AMR = self._predicate_cell_carrying_data() + " and (" + \
        "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction or " \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary " + \
      ")"
      
    self._guard_project_patch_onto_faces = self._predicate_cell_carrying_data() + " and (" + \
      "   (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"

    self._guard_update_cell     = self._predicate_cell_carrying_data() + " and " + primary_sweep_predicate_for_guard
    
    self._guard_handle_boundary = self._predicate_boundary_face_carrying_data() + " and " + primary_or_initialisation_sweep_predicate_for_guard
    
    #
    # Exchange new patch data
    #
    # See above: in the enclave version, we do exchange the new time step's information before
    # the new time step approximation is rolled over. So here, we send out data in the primary
    # sweep and then receive it in the secondary.
    #
    self._patch_overlap_new.generator.send_condition               = self._predicate_face_carrying_data() + " and (" \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation " \
      + ")"
    self._patch_overlap_new.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and (" \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary" \
      + ")"

    #
    # Exchange patch overlaps
    #
    # Throughout the actual time stepping, there's no need to exchange the current time step 
    # information at all. This is different to the baseline version where the current time 
    # step information is exchanged (after we've used the time step updates to obtain the new 
    # step's solution). The reason for this is that we exchange the new patch data, i.e. the 
    # updates, rather than the real data. This information then is merged in the secondary
    # sweep and consequently rolled over.
    #
    # Throughout the grid construction we do not exchange any data. Neither do we exchange
    # data when we plot stuff.
    #
    # The tricky part is the grid initialisation: Here, we have to exchange the real Q and 
    # consequently receive it in the very first follow-up sweep. I tried to use the new Q to
    # transfer data, but that does not work et all as I have to roll over the new Q into the 
    # real Q before I apply any boundary conditions. The roll overs however happen towards 
    # the end of a sweep. So it becomes all bloody complicated. I therefore exchange the 
    # real Q throughout the initialisation. That means, in the very first real sweep (which 
    # either is the plot or the primary sweep of the very first time step), the real Q 
    # counterpart along a partition domain is available, we can merge it, and then directly
    # apply any patch reconstruction or boundary data routines.  
    # 
    # See self._patch_overlap_new for further details.
    #
    self._patch_overlap.generator.send_condition               = self._predicate_face_carrying_data() + " and (" \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" \
      + ")"
    self._patch_overlap.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and (" \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " \
      + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" \
      + ")"
    
    self._template_adjust_cell      = self._get_template_adjust_cell()
    self._template_AMR              = self._get_template_AMR()
    self._template_handle_boundary  = self._get_template_handle_boundary()
    
    self._cell_sempahore_label = exahype2.grid.create_enclave_cell_label( self._name )
   
    #
    # In the plain version, all cell data are always streamed in and out of the persistent
    # stacks. For the enclave version, we can be picky: Only skeleton data is stored 
    # persistently after the primary sweep. All enclave data is backed up (as reconstructed
    # patch data) in tasks. So no need to stream it to the output stack.
    # 
    # In the secondary sweep, only data from skeelton cells is to be streamed in. However, 
    # we stream out both skeleton and enclave tasks: After any secondary sweep, all data 
    # resides on the output/input stream. In the subsequent primary sweep, we thus have
    # to stream in all data again.
    #

    # @todo Wieder zurueckintegrieren
    #self._patch.generator.store_persistent_condition  = "marker.isSkeletonCell() or observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::Primary"
    #self._patch.generator.load_persistent_condition   = "marker.isSkeletonCell() or observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::Secondary"
    self._patch.generator.includes                   += """ #include "observers/SolverRepository.h" """

    self._rusanov_call = ""
    if   flux!=PDETerms.None_Implementation and ncp==PDETerms.None_Implementation:
      self._rusanov_call = GenericRusanovFixedTimeStepSize.RusanovCallWithFluxAndEigenvalues
    elif flux==PDETerms.None_Implementation and ncp!=PDETerms.None_Implementation:
      self._rusanov_call = GenericRusanovFixedTimeStepSize.RusanovCallWithNCPAndEigenvalues
    else:
      raise Exception("ERROR: Combination of fluxes/operators not supported. flux: {} ncp: {}".format(flux, ncp))

    self.set_implementation(flux,ncp)
    self.set_update_cell_implementation()

    pass


  def _wrap_update_cell_template(self, update_cell_template, memory_location):
    free_memory_call_for_skeleton_cells = ""
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete:
      free_memory_call_for_skeleton_cells = "tarch::multicore::freeMemory(reconstructedPatch,::tarch::multicore::MemoryLocation::Heap);"
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      print( "on-accelerator storage not yet implemented!")
      free_memory_call_for_skeleton_cells = "tarch::multicore::freeMemory(reconstructedPatch,::tarch::multicore::MemoryLocation::Accelerator);"

    result = """if (marker.isSkeletonCell()) {
""" + update_cell_template + """
      """ + free_memory_call_for_skeleton_cells + """
    }
    else {
      ::exahype2::EnclaveTask* newEnclaveTask = new ::exahype2::EnclaveTask(
        marker,
        reconstructedPatch,
        #if Dimensions==2
        {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
        #else
        {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
        #endif
        [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
           ::exahype2::fv::copyPatch(
             reconstructedPatch,
             originalPatch,
             {{NUMBER_OF_UNKNOWNS}},
             {{NUMBER_OF_AUXILIARY_VARIABLES}},
             {{NUMBER_OF_VOLUMES_PER_AXIS}},
             {{HALO_SIZE}}
           );
           """ + update_cell_template + """
        }
      );
      
      fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( newEnclaveTask->getTaskId() );
      peano4::parallel::Tasks spawn( 
        newEnclaveTask,
        peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
        peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFV" )
      );      
    }
"""
    return result
    
    
  
  def set_update_cell_implementation(self,
    function_call   = AbstractAoSWithOverlap1.CellUpdateImplementation_NestedLoop,
    memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete
  ):
    if memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      print( "WARNING: Selected memory allocation which does delete allocated memory. Enclave tasking has to have a mode which does not automatically free the memory and allocation has to run through tarch. Code will likely produce invalid memory accesses!" )

    self._template_update_cell      = jinja2.Template( self._wrap_update_cell_template( 
      self._get_template_update_cell( self._rusanov_call + """
          QL, QR, x, dx, t, dt, normal, """ + 
      str(self._unknowns) + """, """ + str(self._auxiliary_variables) + """, FL, FR
        );
"""   ), memory_location))
    self._reconstructed_array_memory_location = memory_location
        
  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    self._add_generic_Rusanov_FV_entries_to_text_replacement_dictionary(d)

    d[ "TIME_STEP_SIZE" ] = self._time_step_size
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    
    pass
  
  
  # @todo Das sollte man komplett auslagern koennen
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "exahype2/EnclaveOpenMPGPUTask.h"

#include "peano4/parallel/Tasks.h"
"""    
  

  # @todo Das sollte man komplett auslagern koennen
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    FV.add_actions_to_create_grid(self,step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.EnclaveLabels( self._name ) )


  # @todo Das sollte man komplett auslagern koennen
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

    #reconstruct_patch_and_apply_FV_kernel = peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(
    #  self._patch,
    #  self._patch_overlap,
    #  task_based_implementation_primary_iteration,
    #  "",
    #  "marker.isEnclaveCell() and not marker.isRefined() and " + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + """::SolverState::Primary""",
    #  "false",
    #  self._get_default_includes() + self.get_user_includes(),
    #  memory_allocation_mode
    #)

    roll_over_enclave_task_results = peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,
      """
      const int taskNumber = fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.getSemaphoreNumber();
      if ( taskNumber>=0 ) {
        ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, patchData );
      }
      fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      """,
      "marker.isEnclaveCell() and not marker.isRefined() and " + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + """::SolverState::Secondary""",
      self._get_default_includes() + self.get_user_includes()
    )

    #step.add_action_set( reconstruct_patch_and_apply_FV_kernel )
    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    step.add_action_set( roll_over_enclave_task_results )
    

  def add_to_Peano4_datamodel( self, datamodel ):
    FV.add_to_Peano4_datamodel(self,datamodel)
    datamodel.add_cell(self._cell_sempahore_label)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    FV.add_use_data_statements_to_Peano4_solver_step(self,step)
    step.use_cell(self._cell_sempahore_label)


