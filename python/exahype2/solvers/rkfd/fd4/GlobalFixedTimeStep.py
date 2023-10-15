# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms    import PDETerms
from exahype2.solvers.rkfd.SeparateSweeps import SeparateSweeps


import jinja2

from .kernels import create_compute_kernel_for_FD4

from exahype2.solvers.rkfd.kernels import create_abstract_solver_declarations
from exahype2.solvers.rkfd.kernels import create_abstract_solver_definitions
from exahype2.solvers.rkfd.kernels import create_solver_declarations
from exahype2.solvers.rkfd.kernels import create_solver_definitions

from exahype2.solvers.rkfd.kernels import SolverVariant
from exahype2.solvers.rkfd.kernels import KernelVariant

from exahype2.solvers.rkfd.FixedTimeSteppingCodeSnippets import FixedTimeSteppingCodeSnippets

from .amr import switch_to_FD4_tensor_product_interpolation
from .amr import switch_to_FD4_tensor_product_restriction


class GlobalFixedTimeStep( SeparateSweeps ):
  def __init__(self, 
    name, patch_size, rk_order, unknowns, auxiliary_variables, min_meshcell_h, max_meshcell_h, normalised_time_step_size,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=PDETerms.None_Implementation, 
    eigenvalues=PDETerms.None_Implementation, 
    boundary_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.Empty_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    plot_grid_properties=False, KOSigma=8.0
  ):
    """!
    
    Construct the solver
    
    eigenvalues: C++ source code or from PDETerms or None
      By default is None_Implementation, as we don't need eigenvalues as long as we
      work with fixed time step sizes. You can change it manually. Still, the fixed
      time stepping won't use the eigenvalues but you might have code that has it and
      that you don't want to change.
      
  
    normalised_time_step_size: Float
      This is the normalised time step size w.r.t. the coarsest admissible h value
      and the polynomial order. If
      the code employs AMR on top of it and refines further, it will automatically 
      downscale the time step size accordingly. So hand in a valid time step size w.r.t.
      to max_meshcell_h. The actual polynomial order enters the chosen time step size
      as an additional @f$ p^{-2} @f$ scaling, and we expect the user to incorporate
      such a factor into the passed normalised value.
  
    """
    super(GlobalFixedTimeStep,self).__init__(name, 
                                             patch_size, 
                                             3, #overlap, 
                                             rk_order, unknowns, auxiliary_variables, min_meshcell_h, max_meshcell_h, plot_grid_properties, kernel_namespace="fd4") 
    
    self._normalised_time_step_size = normalised_time_step_size

    self._KO_Sigma                            = KOSigma
    
    self._user_action_set_includes += """
#include "exahype2/fd/fd4/FD4.h"
"""

    switch_to_FD4_tensor_product_interpolation( self, "TP_constant")
    switch_to_FD4_tensor_product_restriction( self, "TP_inject_normal_extrap")

    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      initial_conditions=initial_conditions, 
      source_term=source_term )


  def set_implementation(self,
    flux=None, ncp=None, source_term=None, eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,
    memory_location         = None,
    additional_action_set_includes = "",
    additional_user_includes       = "",
    KOSigma                 = None
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
    super(GlobalFixedTimeStep,self).set_implementation(  
      flux, ncp, source_term, eigenvalues,
      boundary_conditions, refinement_criterion, initial_conditions, memory_location, additional_action_set_includes, additional_user_includes)

    if not                  KOSigma==None:  self._KO_Sigma                                  = KOSigma

    self._compute_kernel_call = create_compute_kernel_for_FD4(
      self._flux_implementation, 
      self._ncp_implementation, 
      self._source_term_implementation, 
      compute_max_eigenvalue_of_next_time_step = False,
      solver_variant                           = SolverVariant.WithVirtualFunctions,
      kernel_variant                           = KernelVariant.PatchWiseAoSHeap,
      KOSigma                                  = self._KO_Sigma
    )

    solver_code_snippets = FixedTimeSteppingCodeSnippets(self._normalised_time_step_size,False)

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._abstract_solver_user_declarations += solver_code_snippets.create_abstract_solver_user_declarations()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._abstract_solver_user_definitions  += solver_code_snippets.create_abstract_solver_user_definitions()

    self._compute_time_step_size              = solver_code_snippets.create_compute_time_step_size()
    self._compute_new_time_step_size          = solver_code_snippets.create_compute_new_time_step_size()
    
    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_definitions            = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    self._start_time_step_implementation     = solver_code_snippets.create_start_time_step_implementation()
    self._finish_time_step_implementation    = solver_code_snippets.create_finish_time_step_implementation()
    self._constructor_implementation         = solver_code_snippets.create_abstract_solver_constructor_statements()
      
    self.create_action_sets()

