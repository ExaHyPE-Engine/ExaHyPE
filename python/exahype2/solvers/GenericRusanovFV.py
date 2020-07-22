# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair

import exahype2.grid.EnclaveLabels

from .FV import FV
from multiprocessing.heap import Heap


class AbstractGenericRusanovFV( FV ):
  __User_Defined = "<user-defined>"
  __None         = "<none>"

  def __init__(self, name, patch_size, unknowns, flux, ncp, plot_grid_properties):
    """
    
      flux: Bool
        Use a flux term
      
      ncp: Bool
        Use a non-conversative product    
        
    """
    super(AbstractGenericRusanovFV,self).__init__(name, patch_size, 1, unknowns, plot_grid_properties)
    
    self._flux  = flux
    self._ncp   = ncp
    
    self.HandleCellTemplate = self.construct_HandleCellTemplate()
         
    if flux:
      self._flux_implementation        = self.__User_Defined
    else:
      self._flux_implementation        = self.__None

    if ncp:
      self._ncp_implementation         = self.__User_Defined
    else:
      self._ncp_implementation         = self.__None
      
    self._eigenvalues_implementation                = self.__User_Defined
    self._boundary_conditions_implementation        = self.__User_Defined
    self._refinement_criterion_implementation       = self.__User_Defined
    self._initial_conditions_implementation         = self.__User_Defined


  def construct_HandleCellTemplate(self, namespace="::exahype2::fv::" ):
    """
    
      Analyses which PDE terms the user wants to use and returns the 
      corresponding cell template. This template is usually stored 
      within self.HandleCellTemplate but you might want to do other
      stuff with it
      
    """
    if self._flux and not self._ncp:
      return namespace + self.HandleCellTemplate_Flux
    #elif not flux and ncp:
    #  self.HandleCellTemplate = self.HandleCellTemplate_NCP
    elif self._flux and self._ncp:
      return namespace + self.HandleCellTemplate_Flux_NCP
    else:
      print( "ERROR: Combination of PDE terms not supported" )

  
  def set_implementation(self,flux=__None,ncp=__None,eigenvalues=__User_Defined,boundary_conditions=__User_Defined,refinement_criterion=__User_Defined,initial_conditions=__User_Defined):
    self._flux_implementation        = flux
    self._ncp_implementation         = ncp
    self._eigenvalues_implementation = eigenvalues
    self._boundary_conditions_implementation        = boundary_conditions
    self._refinement_criterion_implementation       = refinement_criterion
    self._initial_conditions_implementation         = initial_conditions
    pass
    

  def get_user_includes(self):
    return """
#include "exahype2/fv/Rusanov.h"
"""    


  HandleBoundaryTemplate = """ 
    logDebug( "touchFaceFirstTime(...)", "label=" << fineGridFaceLabel.toString() );
    ::exahype2::fv::applyBoundaryConditions(
      [&](
        double                                       Qinside[],
        double                                       Qoutside[],
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) -> void {{
        {SOLVER_INSTANCE}.boundaryConditions( Qinside, Qoutside, faceCentre, volumeH, t, normal );
      }},
      marker.x(),
      marker.h(),
      {SOLVER_INSTANCE}.getMinTimeStamp(),
      {TIME_STEP_SIZE},
      {NUMBER_OF_VOLUMES_PER_AXIS},
      {NUMBER_OF_UNKNOWNS},
      marker.getSelectedFaceNumber(),
      fineGridFace{UNKNOWN_IDENTIFIER}.value
    );
"""


  AdjustCellTemplate = """
  {{ 
    int index = 0;
    dfor( volume, {NUMBER_OF_VOLUMES_PER_AXIS} ) {{
      {SOLVER_INSTANCE}.adjustSolution(
        fineGridCell{UNKNOWN_IDENTIFIER}.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), {NUMBER_OF_VOLUMES_PER_AXIS}, volume), 
        ::exahype2::getVolumeSize( marker.h(), {NUMBER_OF_VOLUMES_PER_AXIS} ),
        {SOLVER_INSTANCE}.getMinTimeStamp()
      );
      index += {NUMBER_OF_UNKNOWNS};
    }}
  }} 
"""


  AMRTemplate = """
  {{ 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();
    int index = 0;
    dfor( volume, {NUMBER_OF_VOLUMES_PER_AXIS} ) {{
      refinementCriterion = refinementCriterion and {SOLVER_INSTANCE}.refinementCriterion(
        fineGridCell{UNKNOWN_IDENTIFIER}.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), {NUMBER_OF_VOLUMES_PER_AXIS}, volume), 
        ::exahype2::getVolumeSize( marker.h(), {NUMBER_OF_VOLUMES_PER_AXIS} ),
        {SOLVER_INSTANCE}.getMinTimeStamp()
      );
      index += {NUMBER_OF_UNKNOWNS};
    }}
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, {IS_GRID_CREATION} );
  }} 
"""


  HandleCellTemplate_Flux = """applyRusanovToPatch_FaceLoops(
    [&](
      double                                       Q[],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       F[]
    ) -> void {{
      {SOLVER_INSTANCE}.flux( Q, faceCentre, volumeH, t, normal, F );
    }},
    [&](
      double                                       Q[],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       lambdas[]
    ) -> void {{
      {SOLVER_INSTANCE}.eigenvalues( Q, faceCentre, volumeH, t, normal, lambdas );
    }},
    marker.x(),
    marker.h(),
    {SOLVER_INSTANCE}.getMinTimeStamp(),
    {TIME_STEP_SIZE}, 
    {NUMBER_OF_VOLUMES_PER_AXIS},
    {NUMBER_OF_UNKNOWNS},
    reconstructedPatch,
    originalPatch
  );
"""


  HandleCellTemplate_Flux_NCP = """applyRusanovToPatch_FaceLoops(
    [&](
      double                                       Q[],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       F[]
    ) -> void {{
      {SOLVER_INSTANCE}.flux( Q, faceCentre, volumeH, t, normal, F );
    }},
    [&](
      double Q[],
      double gradQ[][Dimensions],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double BgradQ[]
    ) -> void {{
      {SOLVER_INSTANCE}.nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
    }},
    [&](
      double                                       Q[],
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  volumeH,
      double                                       t,
      double                                       dt,
      int                                          normal,
      double                                       lambdas[]
    ) -> void {{
      {SOLVER_INSTANCE}.eigenvalues( Q, faceCentre, volumeH, t, normal, lambdas );
    }},
    marker.x(),
    marker.h(),
    {SOLVER_INSTANCE}.getMinTimeStamp(),
    {TIME_STEP_SIZE}, 
    {NUMBER_OF_VOLUMES_PER_AXIS},
    {NUMBER_OF_UNKNOWNS},
    reconstructedPatch,
    originalPatch
  );
"""

  def add_entries_to_text_replacement_dictionary(self,d):
    """
    
     This routine befills a dictionary d which is in turn passed to Jinja2 to 
     befill the template for the solver.
    
    """
    d[ "TIME_STEP_SIZE" ]                     = self._time_step_size
    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation





class GenericRusanovFVFixedTimeStepSize( AbstractGenericRusanovFV ):
  def __init__(self, name, patch_size, unknowns, time_step_size, flux=True, ncp=False, plot_grid_properties=False):
    """
      Instantiate a generic FV scheme with an overlap of 1.
    """
    super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, flux, ncp, plot_grid_properties)
    self._time_step_size              = time_step_size
    pass



   

class GenericRusanovFVFixedTimeStepSizeWithEnclaves( AbstractGenericRusanovFV ):
  """
    A fixed time stepping scheme with enclave tasking
    
    This is a simple implementation of a FV scheme using a generic 
    Rusanov solver. It applies the concept of enclave tasking and 
    thus exhibits a higher concurrency level than a plain FV 
    counterpart. The price to pay is a higher memory pressure and 
    further admin overhead.
    
    Algorithmic workflow
    --------------------
    
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
    
    image:: GenericRusanovFVFixedTimeStepSizeWithEnclaves_state-transitions.svg
    
    The variant of enclave tasking as it is discussed here has nothing
    in common with the fused tasking as discussed by Charrier and 
    Weinzierl.
    
    
    
    Attributes
    ----------
    
    _guard_copy_new_face_data_into_face_data: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      The routine triggers the roll-over, i.e. the actual commit of 
      a new time step. It is logically called after a time step and
      hence in touch -face-last-time in the secondary traversal.

    _guard_touch_face_first_time_in_time_step: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Updates the boundary. By definition, boundary cells in the domain
      are skeleton cells. Furthermore, the initialiation/setting of 
      boundary conditions only has to happen once per time step. We 
      hence trigger this routine for skeletons in the primary sweeps.

    _guard_project_patch_onto_faces: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      This routine guards the mapping of an updated cell content onto a
      cell's faces. The destination in QNew, i.e. is not "seen" by 
      others unless the faces' new data are committed. Therefore, I 
      trigger this routine in the primary sweep for skeleton cells and
      in the secondary  sweep for enclave cells. The primary calls 
      ensure that data are mapped onto the faces before faces are 
      exchanged with neighbour partitions.
      
    _guard_update_cell: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Actual FV kernel invocation.
      
    _patch_overlap.generator.send_condition: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Send out the faces in the grid initialisation. There's no need to 
      send out data in other iterations, as all data exchange is 
      realised through the QNew field. This is different to the plain
      FV realisation, where data exchange happens through Q: The enclave
      tasking exchanges QNew and then rolls data over from QNew into Q.
      The plain FV scheme rolls over QNew into Q and then exchanges the
      data.
    
    _patch_overlap.generator: string
      See documentation of _patch_overlap.generator.send_condition.
      
    _patch_overlap.generator.merge_method_definition: string
      See documentation of _patch_overlap.generator.send_condition.

    _guard_AMR: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      AMR is active throughout the grid construction. After that, it is
      only available for skeleton cells.
    
    _patch_overlap_new.generator.send_condition: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      See discussion of _patch_overlap.generator.send_condition for 
      details. As we effectively disable the data exchange for Q and
      instead ask for data exchange of QNew, we have to add merge 
      operations to QNew.
      
    _patch_overlap_new.generator.receive_and_merge_condition: string
      See _patch_overlap_new.generator.send_condition.
      
    _patch_overlap_new.generator.merge_method_definition: string
      See _patch_overlap_new.generator.send_condition.
      
    
    Methods
    -------
  
    We extend the superclass' add_actions_to_perform_time_step(), 
    add_to_Peano4_datamodel() and add_use_data_statements_to_Peano4_solver_step().
    For the actions, I add a further action which administers the task
    spawning over the enclaves. I plug into the data model routines to 
    add the marker to the cell which holds the semaphore/cell number.
    
  """
  def __init__(self, name, patch_size, unknowns, time_step_size, flux=True, ncp=False, plot_grid_properties=False, use_gpu=False):
    """
      Instantiate a generic FV scheme with an overlap of 1.
    """
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).__init__(name, patch_size, unknowns, flux, ncp, plot_grid_properties)

    self._time_step_size              = time_step_size

    #
    # Distribute the numerical activities over the two grid sweeps
    #
    self._guard_copy_new_face_data_into_face_data = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary"
    
    self._guard_touch_face_first_time_in_time_step = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary and fineGridFaceLabel.getBoundary() or " \
                                                   + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation and fineGridFaceLabel.getBoundary()"
    
    self._guard_project_patch_onto_faces = \
     "   (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )"

    self._guard_update_cell = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary and marker.isSkeletonCell() and not marker.isRefined()"

    #
    # Exchange patch overlaps
    #
    # Throughout the actual time stepping, there's no need to exchange the current time step 
    # information at all. This is different to the baseline version where the current time 
    # step information is exchanged (after we've used the time step updates to obtain the new 
    # step's solution). The reason for this is that we exchange the new patch data, i.e. the 
    # updates, rather than the real data. So consult self._patch_overlap_new for details.
    #
    # That leaves us with the fact that we have to exchange all data throughout the grid 
    # construction. Here, the new patch data is not used yet.
    #
    self._patch_overlap.generator.send_condition               = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" 
    self._patch_overlap.generator.receive_and_merge_condition  = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " \
                                                               + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation"
    self._patch_overlap.generator.merge_method_definition      = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    self._patch_overlap.generator.includes                    += """
#include "observers/SolverRepository.h" 
#include "peano4/utils/Loop.h" 
""" 

    self._guard_AMR = " observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction or " \
                    + "(observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary and marker.isSkeletonCell() )"
    
    
    #
    # Exchange new patch data
    #
    # See above: in the enclave version, we do exchange the new time step's information before
    # the new time step approximation is rolled over. So here, we send out data in the primary
    # sweep and then receive it in the secondary.
    #
    self._patch_overlap_new.generator.send_condition               = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " \
                                                                   + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary"
    self._patch_overlap_new.generator.includes                    += """
#include "observers/SolverRepository.h" 
#include "peano4/utils/Loop.h" 
""" 
    self._patch_overlap_new.generator.merge_method_definition      = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)

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
    self._patch.generator.store_persistent_condition  = "marker.isSkeletonCell() or observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::Primary"
    self._patch.generator.load_persistent_condition   = "marker.isSkeletonCell() or observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::Secondary"
    self._patch.generator.includes                   += """ #include "observers/SolverRepository.h" """
    
    self._use_gpu = use_gpu 


  def add_actions_to_create_grid(self, step):
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).add_actions_to_create_grid(step)
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
      respectively. One wraps the implementation of HandleCellTemplate into a 
      task, the other communicates with the task bookkeeping only. Both rely on
      additional labels within the cell. We therefore end up with three new 
      action sets: reconstruct_patch_and_apply_FV_kernel, exahype2.grid.EnclaveLabels
      and roll_over_enclave_task_results.
 
    """
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).add_actions_to_perform_time_step(step)

    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D" ]               = self._patch.no_of_unknowns * self._patch.dim[0] * self._patch.dim[0] 
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D" ]               = self._patch.no_of_unknowns * self._patch.dim[0] * self._patch.dim[0] * self._patch.dim[0] 
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] = self._patch.no_of_unknowns * (self._patch_overlap.dim[0] + self._patch.dim[0]) * (self._patch_overlap.dim[0] + self._patch.dim[0]) 
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] = self._patch.no_of_unknowns * (self._patch_overlap.dim[0] + self._patch.dim[0]) * (self._patch_overlap.dim[0] + self._patch.dim[0]) * (self._patch_overlap.dim[0] + self._patch.dim[0]) 
    

        

    if self._use_gpu:        
      task_based_implementation_primary_iteration = """
    static auto taskBody = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {{
      ::exahype2::fv::gpu::copyPatch(
        reconstructedPatch,
        originalPatch,
        {NUMBER_OF_UNKNOWNS},
        {NUMBER_OF_VOLUMES_PER_AXIS},
        {HALO_SIZE}
      );
      
      """ + self.construct_HandleCellTemplate( namespace="::exahype2::fv::gpu::" ) + """  
    }};


    ::exahype2::EnclaveGPUTask* task = new ::exahype2::EnclaveGPUTask(
        marker,
        reconstructedPatch,
        #if Dimensions==2
        {NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D},
        #else
        {NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D},
        #endif
        taskBody        
    );
"""
    else:    
      task_based_implementation_primary_iteration = """
    static auto taskBody = [&](double* reconstructedPatch, double* originalPatch, const ::peano4::datamanagement::CellMarker& marker) -> void {{
      ::exahype2::fv::copyPatch(
        reconstructedPatch,
        originalPatch,
        {NUMBER_OF_UNKNOWNS},
        {NUMBER_OF_VOLUMES_PER_AXIS},
        {HALO_SIZE}
      );
      
      """ + self.construct_HandleCellTemplate( namespace="::exahype2::fv::gpu::" ) + """  
    }};


    ::exahype2::EnclaveTask* task = new ::exahype2::EnclaveTask(
        marker,
        reconstructedPatch,
        #if Dimensions==2
        {NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D},
        #else
        {NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D},
        #endif
        taskBody        
    );
"""


    task_based_implementation_primary_iteration += """
    peano4::parallel::Tasks spawn( 
        task,
        peano4::parallel::Tasks::TaskType::LowPriorityLIFO,
        //peano4::parallel::Tasks::TaskType::Sequential,
        peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFV" )
    );
      
    fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( task->getTaskNumber() );
    """ 
    
    task_based_implementation_secondary_iteration = """
      const int taskNumber = fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.getSemaphoreNumber();
      if ( taskNumber>=0 ) {
        ::exahype2::EnclaveBookkeeping::getInstance().waitForTaskToTerminateAndCopyResultOver( taskNumber, patchData );
      }
      fineGridCell""" + exahype2.grid.EnclaveLabels.get_attribute_name(self._name) + """.setSemaphoreNumber( ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
"""    

    reconstruct_patch_and_apply_FV_kernel = peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(
      self._patch,
      self._patch_overlap,
      task_based_implementation_primary_iteration.format(**d),
      "",
      "marker.isEnclaveCell() and not marker.isRefined() and " + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + """::SolverState::Primary""",
      "false",
      self._get_default_includes() + self.get_user_includes() + """
#include "exahype2/NonCriticalAssertions.h" 
#include "exahype2/fv/Generic.h" 
#include "peano4/parallel/Tasks.h" 
""",
      True
    )
    reconstruct_patch_and_apply_FV_kernel.additional_includes += """
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "exahype2/EnclaveGPUTask.h"
"""    

    roll_over_enclave_task_results = peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,
      task_based_implementation_secondary_iteration,
      "marker.isEnclaveCell() and not marker.isRefined() and " + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + """::SolverState::Secondary""",
      self._get_default_includes() + self.get_user_includes() + """
#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"
#include "exahype2/EnclaveGPUTask.h"
"""    
    )

    step.add_action_set( reconstruct_patch_and_apply_FV_kernel )
    step.add_action_set( exahype2.grid.EnclaveLabels(self._name) ) 
    step.add_action_set( roll_over_enclave_task_results )
    

  def add_to_Peano4_datamodel( self, datamodel ):
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).add_to_Peano4_datamodel(datamodel)
    datamodel.add_cell(self._cell_sempahore_label)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).add_use_data_statements_to_Peano4_solver_step(step)
    step.use_cell(self._cell_sempahore_label)
