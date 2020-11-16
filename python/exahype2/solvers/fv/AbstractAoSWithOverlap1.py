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
     
     
  def __init__(self, kernel_implementation = CellUpdateImplementation_NestedLoop):
    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._boundary_conditions_implementation  = PDETerms.None_Implementation
    self._refinement_criterion_implementation = PDETerms.None_Implementation
    self._initial_conditions_implementation   = PDETerms.None_Implementation
    self._kernel_implementation               = kernel_implementation



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
    
     
    
  #def _get_template_update_cell(self,
  #  split_Riemann_solver_kernel_implementation,
  #  cell_time_stamp     = "minTimeStamp", 
  #  cell_time_step_size = "{{TIME_STEP_SIZE}}"
  #):
  #  """
  #  
  #  Returns a string that handles the whole cell treatment. You will have to wrap it 
  #  into a Jinja2 template if you wanna use it within C++.
  #  
  #  
  #  split_Riemann_solver_kernel_implementation: String
  #     A plain function call to invoke the 1d Riemann solver. For the arguments 
  #     that are available at this point, see below. 
  #     
  #  """
  #  signature = """
  #    [&](
  #      double                                       QL[],
  #      double                                       QR[],
  #      const tarch::la::Vector<Dimensions,double>&  x,
  #      double                                       dx,
  #      double                                       t,
  #      double                                       dt,
  #      int                                          normal,
  #      double                                       FL[],
  #      double                                       FR[]
  #    ) -> void {
  #      """ + split_Riemann_solver_kernel_implementation + """
  #    },
  #    marker.x(),
  #    marker.h(),
  #    """ + cell_time_stamp + """,
  #    """ + cell_time_step_size + """,
  #    {{NUMBER_OF_VOLUMES_PER_AXIS}},
  #    {{NUMBER_OF_UNKNOWNS}},
  #    {{NUMBER_OF_AUXILIARY_VARIABLES}},
  #    reconstructedPatch,
  #    originalPatch
  #);
  #"""
  #
  #  return str(self._kernel_implementation) + str(signature)  
