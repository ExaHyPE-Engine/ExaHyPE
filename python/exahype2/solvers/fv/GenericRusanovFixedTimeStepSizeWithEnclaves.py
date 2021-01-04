# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import *
 
from .PDETerms import PDETerms

import peano4
import exahype2

import jinja2

from .GenericRusanovFixedTimeStepSize import GenericRusanovFixedTimeStepSize 
from .GenericRusanovFixedTimeStepSize import UpdateCell 

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor

import peano4.output.Jinja2TemplatedHeaderImplementationFilePair


class MergeEnclaveTaskOutcome(AbstractFVActionSet):
  Template = """
  if ( marker.isEnclaveCell() and not marker.isRefined() and {{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::Secondary ) {
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
  


class UpdateCellWithEnclaves(ReconstructPatchAndApplyFunctor):
  TemplateUpdateCell = """
    ::exahype2::fv::validatePatch(
      reconstructedPatch,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      1, // halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    ); // previous time step has to be valid
  
  if (marker.isSkeletonCell()) {
   
    {% if USE_SPLIT_LOOP %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
    #endif
    {% else %}
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
    {% endif %}
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
          [] (
           double * __restrict__ Q,
           const tarch::la::Vector<Dimensions,double>&  faceCentre,
           const tarch::la::Vector<Dimensions,double>&  volumeH,
           double                                       t,
           double                                       dt,
           int                                          normal,
           double * __restrict__                        F
          ) -> void {
            {% if FLUX_IMPLEMENTATION=="<none>" %}
            for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
            {% else %}
            {{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
            {% endif %}
          },
          {% if NCP_IMPLEMENTATION!="<none>" %}
          [] (
            double                                       Q[],
            double                                       gradQ[][Dimensions],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {{SOLVER_INSTANCE}}.nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          },
          {% endif %}
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          },
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

      {% if USE_SPLIT_LOOP %}
      #if Dimensions==2
      ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
      #else
      ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
      #endif
      {% else %}
      #if Dimensions==2
      ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
      #else
      ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
      #endif
      {% endif %}
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
          [] (
           double * __restrict__ Q,
           const tarch::la::Vector<Dimensions,double>&  faceCentre,
           const tarch::la::Vector<Dimensions,double>&  volumeH,
           double                                       t,
           double                                       dt,
           int                                          normal,
           double * __restrict__                        F
          ) -> void {
            {% if FLUX_IMPLEMENTATION=="<none>" %}
            for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) F[i] = 0.0;
            {% else %}
            {{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
            {% endif %}
          },
          {% if NCP_IMPLEMENTATION!="<none>" %}
          [] (
            double                                       Q[],
            double                                       gradQ[][Dimensions],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {{SOLVER_INSTANCE}}.nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          },
          {% endif %}
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          },
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
  
  
  def __init__(self,solver,use_split_loop=False):
    d = {}
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["USE_SPLIT_LOOP"] = use_split_loop      
    
    ReconstructPatchAndApplyFunctor.__init__(self,
      solver._patch,
      solver._patch_overlap,
      jinja2.Template( self.TemplateUpdateCell ).render(**d),
      solver._reconstructed_array_memory_location,
      "not marker.isRefined() and (" + \
      "observers::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::Primary or " + \
      "observers::" + solver.get_name_of_global_instance() + ".getSolverState()==" + solver._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"
    )
    self.label_name = exahype2.grid.EnclaveLabels.get_attribute_name(solver._name)


  def get_includes(self):
    return ReconstructPatchAndApplyFunctor.get_includes(self) + """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "peano4/parallel/Tasks.h"
#include "SolverRepository.h"
"""


class GenericRusanovFixedTimeStepSizeWithEnclaves( FV ):
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False):
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

    self._time_step_size = time_step_size
    
    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    self._use_split_loop                      = False

    initialisation_sweep_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
      
    first_iteration_after_initialisation_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition" + \
    ")"

    primary_sweep_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    primary_sweep_or_plot_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"

    primary_or_initialisation_sweep_predicate= "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation" + \
      ")"

    secondary_sweep_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary" + \
      ")"

    secondary_sweep_or_grid_construction_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction" + \
      ")"

    secondary_sweep_or_grid_initialisation_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"

    secondary_sweep_or_grid_initialisation_or_plot_predicate = "(" + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Plotting " + \
      ")"

    self._patch_overlap.generator.store_persistent_condition   = secondary_sweep_or_grid_initialisation_or_plot_predicate
    self._patch_overlap.generator.load_persistent_condition    = primary_sweep_or_plot_predicate

    self._patch_overlap.generator.send_condition               = "not marker.isRefined() and " + initialisation_sweep_predicate
    self._patch_overlap.generator.receive_and_merge_condition  = "not marker.isRefined() and " + first_iteration_after_initialisation_predicate

    self._patch_overlap_new.generator.store_persistent_condition   = "not marker.isRefined() and " + primary_sweep_predicate
    self._patch_overlap_new.generator.load_persistent_condition    = "not marker.isRefined() and " + secondary_sweep_predicate

    self._patch_overlap_new.generator.send_condition               = "true"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "true"

    self._patch_overlap.generator.includes  += """
#include "../observers/SolverRepository.h"
"""    
    self._patch_overlap_new.generator.includes  += """
#include "../observers/SolverRepository.h"
"""    

    self._action_set_adjust_cell                         = AdjustPatch(self, "not marker.isRefined() and " + primary_or_initialisation_sweep_predicate)
    self._action_set_AMR                                 = AMROnPatch(self, "not marker.isRefined() and " + secondary_sweep_or_grid_construction_predicate)
    # @todo Da ist einmal marker zu viel, weil steckt ja schon im Template drin
    self._action_set_handle_boundary                     = HandleBoundary(self, "not marker.isRefined() and " + primary_or_initialisation_sweep_predicate)
    self._action_set_project_patch_onto_faces            = ProjectPatchOntoFaces(self, 
      "not marker.isRefined() and (" + \
         "(" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
      "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )" + \
      ")"
    )
    self._action_set_copy_new_patch_overlap_into_overlap = CopyNewPatchOverlapIntoCurrentOverlap(self, "not marker.isRefined() and " + secondary_sweep_or_grid_initialisation_predicate)
    self._action_set_update_cell              = None
   
    self._cell_sempahore_label = exahype2.grid.create_enclave_cell_label( self._name )

    self.set_implementation(flux=flux,ncp=ncp)
  


  def set_implementation(self,
    flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None,
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

    if memory_location!=None:
      self._reconstructed_array_memory_location = memory_location
    if use_split_loop!=None:
      self._use_split_loop = use_split_loop

    if self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete and \
       self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete and \
       self._reconstructed_array_memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without immedidate (call stack) free chosen. This will lead into a segmentation fault" )

    self._action_set_update_cell = UpdateCellWithEnclaves(self)

  
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
    d[ "TIME_STAMP" ]     = d[ "SOLVER_INSTANCE" ] + ".getMinTimeStamp()"
    
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
    step.add_action_set( MergeEnclaveTaskOutcome(self) )
    

  def add_to_Peano4_datamodel( self, datamodel ):
    FV.add_to_Peano4_datamodel(self,datamodel)
    datamodel.add_cell(self._cell_sempahore_label)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    FV.add_use_data_statements_to_Peano4_solver_step(self,step)
    step.use_cell(self._cell_sempahore_label)
