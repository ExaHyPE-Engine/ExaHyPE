# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.fv.EnclaveTasking import EnclaveTasking

import jinja2

from .kernels import create_compute_Riemann_kernel_for_Rusanov
from .kernels import create_abstract_solver_declarations
from .kernels import create_abstract_solver_definitions
from .kernels import create_solver_declarations
from .kernels import create_solver_definitions
from .kernels import create_compute_time_step_size_kernel_for_adaptive_time_stepping_with_subcycling

from .kernels import create_compute_new_time_step_size_kernel_for_adaptive_time_stepping
from .kernels import create_constructor_implementation_for_adaptive_time_stepping

from .kernels import SolverVariant
from .kernels import KernelVariant

from .kernels import create_abstract_solver_user_declarations_for_adaptive_time_stepping
from .kernels import create_abstract_solver_user_definitions_for_adaptive_time_stepping
from .kernels import create_finish_time_step_implementation_for_adaptive_time_stepping
from exahype2.solvers.fv.kernels import create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling

from exahype2.solvers.fv.kernels import create_halo_layer_construction_with_interpolation_for_reconstructed_patch


class SubcyclingAdaptiveTimeStepWithEnclaveTasking(EnclaveTasking):
  def __init__(self,
    name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation,
    ncp=PDETerms.None_Implementation,
    eigenvalues=PDETerms.User_Defined_Implementation,
    boundary_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.Empty_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    source_term=PDETerms.None_Implementation,
    plot_grid_properties=False,
    interpolate_linearly_in_time=True,
    pde_terms_without_state=False, overlap=1
  ):
    """
    time_step_size: Float
      This is the normalised time step size w.r.t. the coarsest admissible h value. If
      the code employs AMR on top of it and refines further, it will automatically
      downscale the time step size accordingly. So hand in a valid time step size w.r.t.
      to max_volume_h.
    """
    self._interpolate_linearly_in_time = interpolate_linearly_in_time

    super(SubcyclingAdaptiveTimeStepWithEnclaveTasking,self).__init__(name,
                                                                      patch_size,
                                                                      overlap,
                                                                      unknowns,
                                                                      auxiliary_variables,
                                                                      min_volume_h,
                                                                      max_volume_h,
                                                                      plot_grid_properties,
                                                                      pde_terms_without_state,
                                                                      kernel_namespace="rusanov")
    self._time_step_relaxation = time_step_relaxation

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation

    self._compute_eigenvalue                  = True

    self._compute_time_step_size              = create_compute_time_step_size_kernel_for_adaptive_time_stepping_with_subcycling( name )
    #self._compute_new_time_step_size          = """
#  const double timeStepSize = dt;
#  const double timeStamp    = t;
#""" +    create_compute_new_time_step_size_kernel_for_adaptive_time_stepping()
    self._compute_new_time_step_size          = create_compute_new_time_step_size_kernel_for_adaptive_time_stepping()

    self.set_implementation(flux=flux,
      ncp=ncp,
      eigenvalues=eigenvalues,
      boundary_conditions=boundary_conditions,
      refinement_criterion=refinement_criterion,
      initial_conditions=initial_conditions,
      source_term=source_term)


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
    will create nop, i.e., no implementation or defaults. Any other string
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

    self._compute_kernel_call = create_compute_Riemann_kernel_for_Rusanov(
      self._flux_implementation,
      self._ncp_implementation,
      self._source_term_implementation,
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant = SolverVariant.WithVirtualFunctions,
      kernel_variant = KernelVariant.PatchWiseAoS
    )

    self._compute_kernel_call_stateless = create_compute_Riemann_kernel_for_Rusanov(
      self._flux_implementation,
      self._ncp_implementation,
      self._source_term_implementation,
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant = SolverVariant.Stateless,
      kernel_variant = KernelVariant.PatchWiseAoS
    )

    self._fused_compute_kernel_call_stateless_cpu = create_compute_Riemann_kernel_for_Rusanov(
      self._flux_implementation,
      self._ncp_implementation,
      self._source_term_implementation,
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant = SolverVariant.Stateless,
      kernel_variant = KernelVariant.PatchWiseAoS
    )

    self._fused_compute_kernel_call_stateless_gpu = create_compute_Riemann_kernel_for_Rusanov(
      self._flux_implementation,
      self._ncp_implementation,
      self._source_term_implementation,
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant = SolverVariant.Accelerator,
      kernel_variant = KernelVariant.VolumeWiseAoS
    )

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)
    self._abstract_solver_user_declarations += create_abstract_solver_user_declarations_for_adaptive_time_stepping()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)
    self._abstract_solver_user_definitions  += create_abstract_solver_user_definitions_for_adaptive_time_stepping()

    self._constructor_implementation         = create_constructor_implementation_for_adaptive_time_stepping()

    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)
    self._solver_user_definitions            = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)

    self._start_time_step_implementation     = create_start_time_step_implementation_for_adaptive_time_stepping_with_subcycling(True)
    self._finish_time_step_implementation    = """if (_solverState==SolverState::Secondary) {
""" +    create_finish_time_step_implementation_for_adaptive_time_stepping(self._time_step_relaxation) + """
}
"""

    super(SubcyclingAdaptiveTimeStepWithEnclaveTasking,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop, additional_action_set_includes, additional_user_includes)


  def create_action_sets(self):
    """
    The actual action sets all are created by the superclass. So nothing
    is to be done here. But we want to reset the actual updates and
    projection, and these only happen if we are allowed to update
    indeed.
    """
    super(SubcyclingAdaptiveTimeStepWithEnclaveTasking, self).create_action_sets()

    update_cell_guard  = "::exahype2::runTimeStepOnCell( fineGridCell" + self._name + "CellLabel, fineGridFaces" + self._name + "FaceLabel, repositories::getMinTimeStamp())"
    #updated_cell_guard = "fineGridCell" + self._name + "CellLabel.getSemaphoreNumber()!=::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber"
    self._action_set_update_cell.guard                += " and " + update_cell_guard
    #self._action_set_merge_enclave_task_outcome.guard  = updated_cell_guard

    if self._interpolate_linearly_in_time:
      self._action_set_update_cell._Template_TouchCellFirstTime_Fill_Halos = create_halo_layer_construction_with_interpolation_for_reconstructed_patch(self._name)

    self._action_set_AMR.event_lifetime = 2*int(self._max_volume_h/self._min_volume_h+1)


  @property
  def user_action_set_includes(self):
    return super(SubcyclingAdaptiveTimeStepWithEnclaveTasking, self).user_action_set_includes + """
#include "exahype2/TimeStepping.h"
#include "exahype2/fv/rusanov/rusanov.h"
"""


  def create_data_structures(self):
    super(SubcyclingAdaptiveTimeStepWithEnclaveTasking, self).create_data_structures()

    self._patch_overlap_old.generator.send_condition               = self._patch_overlap_new.generator.send_condition
    self._patch_overlap_old.generator.receive_and_merge_condition  = self._patch_overlap_new.generator.receive_and_merge_condition
