# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.EnclaveTasking import EnclaveTasking
from exahype2.solvers.fv.PDETerms       import PDETerms

import jinja2

from .kernels import create_source_term_kernel_for_Rusanov
from .kernels import create_compute_Riemann_kernel_for_Rusanov
from .kernels import create_abstract_solver_declarations
from .kernels import create_abstract_solver_definitions
from .kernels import create_solver_declarations
from .kernels import create_solver_definitions
from .kernels import create_compute_time_step_size_for_fixed_time_stepping_with_subcycling
from .kernels import create_fused_compute_Riemann_kernel_for_Rusanov

from exahype2.solvers.fv.kernels import create_halo_layer_construction_with_interpolation_for_reconstructed_patch
from exahype2.solvers.fv.kernels import create_abstract_solver_user_declarations_for_fixed_time_stepping
from exahype2.solvers.fv.kernels import create_abstract_solver_user_definitions_for_fixed_time_stepping
from exahype2.solvers.fv.kernels import create_finish_time_step_implementation_for_fixed_time_stepping
from exahype2.solvers.fv.kernels import create_start_time_step_implementation_for_fixed_time_stepping_with_subcycling


class SubcyclingFixedTimeStepWithEnclaveTasking( EnclaveTasking ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, time_step_size,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False,
    interpolate_linearly_in_time=True,
    stateless_pde_terms=False
  ):
    """
  
    time_step_size: Float
      This is the normalised time step size w.r.t. the coarsest admissible h value. If
      the code employs AMR on top of it and refines further, it will automatically 
      downscale the time step size accordingly. So hand in a valid time step size w.r.t.
      to max_volume_h.
  
    """
    self._interpolate_linearly_in_time        = interpolate_linearly_in_time
    
    super(SubcyclingFixedTimeStepWithEnclaveTasking,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties,stateless_pde_terms) 
    
    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation
    
    self._compute_time_step_size  = create_compute_time_step_size_for_fixed_time_stepping_with_subcycling( time_step_size, name )
    
    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    memory_location         = None,
    use_split_loop          = False,
    additional_action_set_includes = "",
    additional_user_includes       = ""
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers. You
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.

      This routine should be the very last invoked by the constructor.
    """
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term

    self._source_term_call           = create_source_term_kernel_for_Rusanov(self._source_term_implementation)
    self._Riemann_solver_call        = create_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation)
    self._fused_Riemann_solver_call  = create_fused_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation, self._source_term_implementation)

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._stateless_pde_terms)
    self._abstract_solver_user_declarations += create_abstract_solver_user_declarations_for_fixed_time_stepping()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._stateless_pde_terms)
    self._abstract_solver_user_definitions  += create_abstract_solver_user_definitions_for_fixed_time_stepping()

    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._stateless_pde_terms)
    self._solver_user_definitions            = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._stateless_pde_terms)

    self._start_time_step_implementation     = create_start_time_step_implementation_for_fixed_time_stepping_with_subcycling(True)
    self._finish_time_step_implementation    = create_finish_time_step_implementation_for_fixed_time_stepping(self._time_step_size)
      
    super(SubcyclingFixedTimeStepWithEnclaveTasking,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop, additional_action_set_includes, additional_user_includes)


  def create_action_sets(self):
    """
    
    The actual action sets all are created by the superclass. So nothing
    is to be done here. But we want to reset the actual updates and 
    projection, and these only happen if we are allowed to update 
    indeed.
    
    """
    super(SubcyclingFixedTimeStepWithEnclaveTasking, self).create_action_sets()

    update_cell_guard  = "::exahype2::runTimeStepOnCell( fineGridCell" + self._name + "CellLabel, fineGridFaces" + self._name + "FaceLabel)"
    #updated_cell_guard = "fineGridCell" + self._name + "CellLabel.getSemaphoreNumber()!=::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber"
    self._action_set_update_cell.guard                += " and " + update_cell_guard
    #self._action_set_merge_enclave_task_outcome.guard  = updated_cell_guard
    
    if self._interpolate_linearly_in_time:
      self._action_set_update_cell._Template_TouchCellFirstTime_Fill_Halos = create_halo_layer_construction_with_interpolation_for_reconstructed_patch(self._name)

    self._action_set_AMR.set_event_lifetime( 2*int(self._max_volume_h/self._min_volume_h+1) )
     

  def get_user_action_set_includes(self):
    return super(SubcyclingFixedTimeStepWithEnclaveTasking, self).get_user_action_set_includes() + """
#include "exahype2/TimeStepping.h"
"""


  def create_data_structures(self):
    super(SubcyclingFixedTimeStepWithEnclaveTasking, self).create_data_structures()

    self._patch_overlap_old.generator.send_condition               = self._patch_overlap_new.generator.send_condition
    self._patch_overlap_old.generator.receive_and_merge_condition  = self._patch_overlap_new.generator.receive_and_merge_condition

    