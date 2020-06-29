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

  def __init__(self, name, patch_size, unknowns, flux, ncp):
    super(AbstractGenericRusanovFV,self).__init__(name, patch_size, 1, unknowns)
    
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


  CreateCellTemplate = """
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
  def __init__(self, name, patch_size, unknowns, time_step_size, flux=True, ncp=False):
    """
      Instantiate a generic FV scheme with an overlap of 1.
    """
    super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, flux, ncp)
    self._time_step_size              = time_step_size
    pass



   

class GenericRusanovFVFixedTimeStepSizeWithEnclaves( AbstractGenericRusanovFV ):
  """
  
    @image html GenericRusanovFVFixedTimeStepSizeWithEnclaves_state-transitions.svg
    
  """
  def __init__(self, name, patch_size, unknowns, time_step_size, flux=True, ncp=False):
    """
      Instantiate a generic FV scheme with an overlap of 1.
    """
    super(GenericRusanovFVFixedTimeStepSizeWithEnclaves,self).__init__(name, patch_size, unknowns, flux, ncp)

    self._time_step_size              = time_step_size

    #
    # Distribute activities over the two grid sweeps
    #
    self._guard_backup_faces                       = self.get_name_of_global_instance() + ".isPrimary()"
    self._guard_touch_face_first_time_in_time_step = "fineGridFaceLabel.getBoundary() and " + self.get_name_of_global_instance() + ".isPrimary()"
    self._guard_AMR                                = self.get_name_of_global_instance() + ".isSecondary()"
    
    #
    # Actual enclave tasking
    #
    self._guard_project_patch_onto_faces = self.get_name_of_global_instance() + ".isPrimary() or " + self.get_name_of_global_instance() + ".isGridInitialisation()"
    self._guard_update_cell              = self.get_name_of_global_instance() + ".isPrimary()"

    #
    # Ensure that data is sent out after primary traversal and received after secondary traversal
    #
    self._patch_overlap.generator.send_condition               = "observers::" + self.get_name_of_global_instance() + ".sendOutBoundaryData()"
    self._patch_overlap.generator.receive_and_merge_condition  = "observers::" + self.get_name_of_global_instance() + ".receiveAndMergeBoundaryData()"
    self._patch_overlap.generator.includes                    += """#include "observers/SolverRepository.h" """ 

