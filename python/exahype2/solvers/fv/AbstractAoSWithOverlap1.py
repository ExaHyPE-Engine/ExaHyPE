# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair

import exahype2.grid.EnclaveLabels

import jinja2

from .PDETerms import PDETerms

from enum import Enum


class AbstractAoSWithOverlap1(object):
  def __init__(self, flux, ncp):
    self._flux_implementation        = PDETerms.User_Defined_Implementation
    self._ncp_implementation         = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.User_Defined_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

  
  def set_implementation(self,flux=PDETerms.User_Defined_Implementation,ncp=PDETerms.None_Implementation,eigenvalues=PDETerms.User_Defined_Implementation,boundary_conditions=PDETerms.User_Defined_Implementation,refinement_criterion=PDETerms.User_Defined_Implementation,initial_conditions=PDETerms.User_Defined_Implementation):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. 
    """
    self._flux_implementation        = flux
    self._ncp_implementation         = ncp
    self._eigenvalues_implementation = eigenvalues
    self._boundary_conditions_implementation        = boundary_conditions
    self._refinement_criterion_implementation       = refinement_criterion
    self._initial_conditions_implementation         = initial_conditions
    pass

  
  def _add_generic_Rusanov_FV_entries_to_text_replacement_dictionary(self,d):
    """
    
     This routine befills a dictionary d which is in turn passed to Jinja2 to 
     befill the template for the solver.
    
    """
    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    
    
  def _get_template_adjust_cell(self):
    return jinja2.Template("""
  { 
    int index = 0;
    dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
      {{SOLVER_INSTANCE}}.adjustSolution(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
        ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume), 
        ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
        {{SOLVER_INSTANCE}}.getMinTimeStamp()
      );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  } 
""")
    
    
  def _get_template_handle_boundary(self):
    return jinja2.Template("""
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
      ) -> void {
        {{SOLVER_INSTANCE}}.boundaryConditions( Qinside, Qoutside, faceCentre, volumeH, t, normal );
      },
      marker.x(),
      marker.h(),
      {{SOLVER_INSTANCE}}.getMinTimeStamp(),
      {{TIME_STEP_SIZE}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}.value
    );
""")
    
    
  def _get_template_AMR(self):
     return jinja2.Template("""
  { 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

    if (tarch::la::max( marker.h() ) > {{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
      refinementCriterion = ::exahype2::RefinementCommand::Refine;
    } 
    else {
      int index = 0;
      dfor( volume, {{NUMBER_OF_VOLUMES_PER_AXIS}} ) {
        refinementCriterion = refinementCriterion and {{SOLVER_INSTANCE}}.refinementCriterion(
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
          ::exahype2::getVolumeCentre( marker.x(), marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}}, volume), 
          ::exahype2::getVolumeSize( marker.h(), {{NUMBER_OF_VOLUMES_PER_AXIS}} ),
          {{SOLVER_INSTANCE}}.getMinTimeStamp()
        );
        index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
      }
     
      if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < {{SOLVER_INSTANCE}}.getMinMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
      else if (refinementCriterion==::exahype2::RefinementCommand::Coarsen and 3.0* tarch::la::max( marker.h() ) > {{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
    }
    
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion, {{IS_GRID_CREATION}} );
  } 
""")
     
     
    
  """
  
    This is the straightforward implementation.
  """
  CellUpdateImplementation_NestedLoop = """
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d(
    #endif
  """

  CellUpdateImplementation_SplitLoop = """
    #if Dimensions==2
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS2d_SplitLoop(
    #else
    ::exahype2::fv::applySplit1DRiemannToPatch_Overlap1AoS3d_SplitLoop(
    #endif
  """
     
     
  def _get_template_update_cell(self,
    split_Riemann_solver_function_call,
    cell_time_stamp     = "{{SOLVER_INSTANCE}}.getMinTimeStamp()", 
    cell_time_step_size = "{{TIME_STEP_SIZE}}",
    function_call       = CellUpdateImplementation_NestedLoop
  ):
    """
    
    split_Riemann_solver_function_call: String
       A plain function call to invoke the 1d Riemann solver. For the arguments 
       that are available at this point, see below. 
       
    """
    signature = """
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
        """ + split_Riemann_solver_function_call + """
      },
      marker.x(),
      marker.h(),
      """ + cell_time_stamp + """,
      """ + cell_time_step_size + """,
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
  );
"""

    return jinja2.Template( str(function_call) + str(signature) )  
