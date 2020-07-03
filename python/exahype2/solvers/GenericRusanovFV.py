# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair

from .FV import FV


class AbstractGenericRusanovFV( FV ):
  __User_Defined = "<user-defined>"
  __None         = "<none>"

  def __init__(self, name, patch_size, unknowns, flux, ncp, plot_grid_properties):
    super(AbstractGenericRusanovFV,self).__init__(name, patch_size, 1, unknowns, plot_grid_properties)
    
    if flux and not ncp:
      self.HandleCellTemplate = self.HandleCellTemplate_Flux
    #elif not flux and ncp:
    #  self.HandleCellTemplate = self.HandleCellTemplate_NCP
    elif flux and ncp:
      self.HandleCellTemplate = self.HandleCellTemplate_Flux_NCP
    else:
      print( "ERROR: Combination of PDE terms not supported" )
      
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


  HandleCellTemplate_Flux = """::exahype2::fv::applyRusanovToPatch_FaceLoops(
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


  HandleCellTemplate_Flux_NCP = """::exahype2::fv::applyRusanovToPatch_FaceLoops(
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
      double BgradQ[]
    ) -> void {{
      {SOLVER_INSTANCE}.nonconservativeProduct( Q, gradQ, BgradQ );
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
      There's no need to trigger the step in the very first primary
      sweep, as all boundary data is implicitly set by the initial 
      conditions.

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
      Actual FV kernel invocation. I use the FV's blueprint kernel calls
      only for skeleton cells, i.e. in the primary grid sweep. While 
      this guard cares for all skeleton cells, I need specialised 
      treatment of enclaves cells. This is realised within
      add_actions_to_perform_time_step().
      
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
  
    
  """
  def __init__(self, name, patch_size, unknowns, time_step_size, flux=True, ncp=False, plot_grid_properties=False):
    """
      Instantiate a generic FV scheme with an overlap of 1.
    """
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).__init__(name, patch_size, unknowns, flux, ncp, plot_grid_properties)

    self._time_step_size              = time_step_size

    #
    # Distribute the numerical activities over the two grid sweeps
    #
    self._guard_copy_new_face_data_into_face_data = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary"
    
    self._guard_touch_face_first_time_in_time_step = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary and fineGridFaceLabel.getBoundary()"
       
    self._guard_project_patch_onto_faces = \
     "   (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary                         and marker.isSkeletonCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation  and marker.isSkeletonCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary                       and marker.isEnclaveCell() ) " + \
     "or (" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation )"
    
    self._guard_update_cell = self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary and marker.isSkeletonCell() and not marker.isRefined()"

    #
    # There's no need to exchange the current time step information at all. This is different
    # to the baseline version where the current time step information is exchanged (after we've
    # used the time step updates to obtain the new step's solution).
    #
    self._patch_overlap.generator.send_condition               = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" 
    self._patch_overlap.generator.receive_and_merge_condition  = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingInitialCondition or " \
                                                               + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation"
    self._patch_overlap.generator.merge_method_definition      = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    self._patch_overlap.generator.includes                    += """
#include "observers/SolverRepository.h" 
#include "peano4/utils/Loop.h" 
""" 

    self._guard_AMR = " observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridConstruction or" \ 
                    + "(observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary and maker.isSkeleton() )"
    
    
    #
    # See above: in the enclave version, we do exchange the new time step's information before
    # the new time step approximation is rolled over.
    #
    self._patch_overlap_new.generator.send_condition               = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Primary or " \
                                                                   + "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PrimaryAfterGridInitialisation"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "observers::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary"
    self._patch_overlap_new.generator.includes                    += """
#include "observers/SolverRepository.h" 
#include "peano4/utils/Loop.h" 
""" 
    self._patch_overlap_new.generator.merge_method_definition      = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)


  def add_actions_to_perform_time_step(self, step):
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).add_actions_to_perform_time_step(step)

    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    task_based_implementation = """
peano4::parallel::Tasks spawnBackgroundTask(
  [marker,reconstructedPatch,originalPatch]() -> bool {{
""" + self.HandleCellTemplate + """  
  
    delete[] reconstructedPatch;

    return false;
  }},
  //peano4::parallel::Tasks::TaskType::LowPriority,
  peano4::parallel::Tasks::TaskType::Sequential,
  peano4::parallel::Tasks::getLocationIdentifier( "GenericRusanovFVFixedTimeStepSizeWithEnclaves" ),
  true            // waitForCompletion
);    
"""    


    reconstruct_patch_and_apply_FV_kernel = peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(
      self._patch,
      self._patch_overlap,
      task_based_implementation.format(**d),
      "",
      self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::Secondary and marker.isEnclaveCell() and not marker.isRefined()",
      "false",
      self._get_default_includes() + self.get_user_includes() + """
#include "exahype2/NonCriticalAssertions.h" 
#include "peano4/parallel/Tasks.h" 
""",
      True
    )
    step.add_action_set( reconstruct_patch_and_apply_FV_kernel ) 
