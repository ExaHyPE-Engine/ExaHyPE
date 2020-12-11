# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSize import GenericRusanovFixedTimeStepSize 


class GenericRusanovFixedTimeStepSizeWithEnclaves( FV ):
  TemplateUpdateCell = """
  if (marker.isSkeletonCell()) {
    {{LOOP_OVER_PATCH_FUNCTION_CALL}}(
      [&](
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        ::exahype2::fv::splitRusanov1d(
          {{RUSANOV_ON_FACE}},
          {{EIGENVALUES}},
          QL, QR, x, dx, t, dt, normal,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          FL,FR
        );
      },
      marker.x(),
      marker.h(),
      {{TIME_STAMP}},
      {{TIME_STEP_SIZE}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
    );
    
    {{FREE_SKELETON_MEMORY}}
  }
  else { // is an enclave cell
    auto perCellFunctor = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {
      ::exahype2::fv::copyPatch(
        reconstructedPatch,
        originalPatch,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        1 // halo size
      );
      {{LOOP_OVER_PATCH_FUNCTION_CALL}}(
        [&](
          double                                       QL[],
          double                                       QR[],
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       dx,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double                                       FL[],
          double                                       FR[]
        ) -> void {
          ::exahype2::fv::splitRusanov1d(
            {{RUSANOV_ON_FACE}},
            {{EIGENVALUES}},
            QL, QR, x, dx, t, dt, normal,
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            FL,FR
          );
        },
        marker.x(),
        marker.h(),
        {{TIME_STAMP}},
        {{TIME_STEP_SIZE}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        reconstructedPatch,
        originalPatch
      );
    };

    ::exahype2::EnclaveTask* newEnclaveTask = new ::exahype2::EnclaveTask(
      marker,
      reconstructedPatch,
      #if Dimensions==2
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D}},
      #else
      {{NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D}},
      #endif
      perCellFunctor
    );
      
    fineGridCell{{SEMAPHORE_LABEL}}.setSemaphoreNumber( newEnclaveTask->getTaskId() );
    peano4::parallel::Tasks spawn( 
      newEnclaveTask,
      peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
      peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFixedTimeStepSizeWithEnclaves" )
    );      
  }
  """      


  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, kernel_implementation = FV.CellUpdateImplementation_NestedLoop, memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete):
    """
    
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

    # @todo Ein Haufen der Logik kann raus
    
    self._time_step_size = time_step_size
    
    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._kernel_implementation               = None
    self._rusanov_call                        = None
    self._reconstructed_array_memory_location = None

    self.set_implementation(flux=flux,ncp=ncp)
    self.set_update_cell_implementation(kernel_implementation=kernel_implementation,memory_location=memory_location)

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


  def _construct_template_update_cell(self):
    self._template_update_cell      = jinja2.Template( self._wrap_update_cell_template( 
      self._get_template_update_cell( self._rusanov_call + """
          QL, QR, x, dx, t, dt, normal, """ + 
      str(self._unknowns) + """, """ + str(self._auxiliary_variables) + """, FL, FR
        );
"""   )))
    
    
  def set_implementation(self,flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
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
      self._flux_implementation        = flux
    if ncp!=None:
      self._ncp_implementation         = ncp
    if eigenvalues!=None:    
      self._eigenvalues_implementation = eigenvalues
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    
    if self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation==PDETerms.None_Implementation:
      self._rusanov_call = GenericRusanovFixedTimeStepSize.RusanovCallWithFlux
    elif self._flux_implementation==PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
      self._rusanov_call = GenericRusanovFixedTimeStepSize.RusanovCallWithNCP
    else:
      raise Exception("ERROR: Combination of fluxes/operators not supported. flux: {} ncp: {}".format(flux, ncp))

    self._construct_template_update_cell()


  def set_update_cell_implementation(self,
    kernel_implementation   = FV.CellUpdateImplementation_NestedLoop,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete
  ):
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack:
      raise Exception( "Non-heap allocation chosen" )
        
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Heap:
      memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete  
      print( "Warning: Reset memory mode to mode without implicit delete, as enclave tasks free memory themselves" )
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch:
      memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete  
      print( "Warning: Reset memory mode to mode without implicit delete, as enclave tasks free memory themselves" )
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Accelerator:
      memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete  
      print( "Warning: Reset memory mode to mode without implicit delete, as enclave tasks free memory themselves" )

    self._reconstructed_array_memory_location = memory_location
    self._kernel_implementation               = kernel_implementation
    self._construct_template_update_cell()

  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

#include "peano4/parallel/Tasks.h"
"""    

  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STEP_SIZE" ] = self._time_step_size
    
    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_VOLUMES_PER_AXIS"] * (d["NUMBER_OF_UNKNOWNS"] + d["NUMBER_OF_AUXILIARY_VARIABLES"])
    
    d[ "SEMAPHORE_LABEL" ]      = exahype2.grid.EnclaveLabels.get_attribute_name(self._name)


    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "delete[] reconstructedPatch;"
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "::tarch::multicore::freeMemory(reconstructedPatch, ::tarch::multicore::MemoryLocation::Heap);"
    if self._reconstructed_array_memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      d[ "FREE_SKELETON_MEMORY" ] = "::tarch::multicore::freeMemory(reconstructedPatch, ::tarch::multicore::MemoryLocation::Accelerator);"
    pass


  def _construct_template_update_cell(self):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d[ "LOOP_OVER_PATCH_FUNCTION_CALL" ] = self._kernel_implementation
    d[ "TIME_STAMP" ]                   = "{{SOLVER_INSTANCE}}.getMinTimeStamp()"
    d[ "RUSANOV_ON_FACE"]               = self._rusanov_call
    d[ "EIGENVALUES"]                   = GenericRusanovFixedTimeStepSize.EigenvaluesCall
      
    temp = jinja2.Template( self.TemplateUpdateCell ).render(d);
    self._template_update_cell      = jinja2.Template( temp ); 


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    FV.add_actions_to_create_grid(self,step,evaluate_refinement_criterion)
    step.add_action_set( exahype2.grid.EnclaveLabels( self._name ) )


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

    #
    # @todo The template below still holds a lot of manually constructed
    #       expressions. I should replace more of them with proper Jinja 
    # expressions.
    #
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    template = jinja2.Template(
      """
      const int taskNumber = fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.getSemaphoreNumber();
      if ( taskNumber>=0 ) {
        ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, patchData );
      }
      fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
      
      ::exahype2::fv::validatePatch(
        patchData,
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{NUMBER_OF_VOLUMES_PER_AXIS}},
        0,
        std::string(__FILE__) + ": " + std::to_string(__LINE__)
      );
      """)
    
    roll_over_enclave_task_results = peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,
      template.render(**d),
      "marker.isEnclaveCell() and not marker.isRefined() and " + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + """::SolverState::Secondary""",
      self._get_default_includes() + self.get_user_includes()
    )

    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    step.add_action_set( roll_over_enclave_task_results )
    

  def add_to_Peano4_datamodel( self, datamodel ):
    FV.add_to_Peano4_datamodel(self,datamodel)
    datamodel.add_cell(self._cell_sempahore_label)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    FV.add_use_data_statements_to_Peano4_solver_step(self,step)
    step.use_cell(self._cell_sempahore_label)
