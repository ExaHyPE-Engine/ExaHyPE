# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.fv.SingleSweep import SingleSweep
from peano4.toolbox.blockstructured  import ReconstructedArrayMemoryLocation
from exahype2.solvers.fv.AdaptiveTimeSteppingCodeSnippets import AdaptiveTimeSteppingCodeSnippets
from .kernels                        import *
from exahype2.solvers.fv             import PDETerms


class UserDefinedRiemann( SingleSweep ):
  def __init__(self, 
    name, patch_size, unknowns, auxiliary_variables, min_volume_h, max_volume_h, time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation,
    riemann_solver=PDETerms.User_Defined_Implementation,
    eigenvalues=PDETerms.User_Defined_Implementation,
    boundary_conditions=PDETerms.User_Defined_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.Empty_Implementation,
    plot_grid_properties=False, 
    overlap=1
  ):
    super().__init__(name, patch_size,overlap, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties, "riemann") 
    self._time_step_relaxation = time_step_relaxation
    self._user_defined_riemann_solver_implementation       = PDETerms.None_Implementation
    self.set_implementation(flux=flux,
                            riemann_solver=riemann_solver, 
                            boundary_conditions=boundary_conditions,
                            eigenvalues=eigenvalues,
                            refinement_criterion=refinement_criterion,
                            initial_conditions=initial_conditions, 
                            pde_terms_without_state=False)




  def set_implementation(self,
                        flux=None,
                        riemann_solver=None,
                        boundary_conditions=None,
                        refinement_criterion=None,
                        initial_conditions=None,
                        eigenvalues=None,
                        memory_location         = None,
                        use_split_loop          = False,
                        pde_terms_without_state = False,
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
    if riemann_solver       is not None:  self._user_defined_riemann_solver_implementation= riemann_solver
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions
    if memory_location      is not None:  self._reconstructed_array_memory_location       = memory_location
    if use_split_loop                  :  self._use_split_loop                            = use_split_loop
    if pde_terms_without_state         :  self._pde_terms_without_state                   = pde_terms_without_state

    self._compute_kernel_call = create_compute_Riemann_kernel_for_UserDefinedRiemann(
      self._flux_implementation, self._user_defined_riemann_solver_implementation, True
    )

    if self._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       self._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       self._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete:
       raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )
    
    solver_code_snippets = AdaptiveTimeSteppingCodeSnippets(self._time_step_relaxation)

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._user_defined_riemann_solver_implementation, self._eigenvalues_implementation)
    self._abstract_solver_user_declarations += solver_code_snippets.create_abstract_solver_user_declarations()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions(self._flux_implementation, self._user_defined_riemann_solver_implementation, self._eigenvalues_implementation)
    self._abstract_solver_user_definitions  += solver_code_snippets.create_abstract_solver_user_definitions()

    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._user_defined_riemann_solver_implementation, self._eigenvalues_implementation)
    self._solver_user_definitions            = create_solver_definitions(self._flux_implementation, self._user_defined_riemann_solver_implementation, self._eigenvalues_implementation)

    self._compute_time_step_size              = solver_code_snippets.create_compute_time_step_size()
    self._compute_new_time_step_size          = solver_code_snippets.create_compute_new_time_step_size()

    self._start_time_step_implementation     = solver_code_snippets.create_start_time_step_implementation()
    self._finish_time_step_implementation    = solver_code_snippets.create_finish_time_step_implementation()
    self._constructor_implementation         = solver_code_snippets.create_abstract_solver_constructor_statements()

    super(UserDefinedRiemann,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, memory_location, use_split_loop, additional_action_set_includes, additional_user_includes)
  
  @property
  def user_action_set_includes(self):
    return super(UserDefinedRiemann, self).user_action_set_includes + """
#include "exahype2/fv/riemann/Riemann.h"
"""
