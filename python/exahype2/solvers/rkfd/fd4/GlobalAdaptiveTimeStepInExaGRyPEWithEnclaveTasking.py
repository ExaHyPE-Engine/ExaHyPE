# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms    import PDETerms
from exahype2.solvers.rkfd.SeparateSweepsWithEnclaveTasking import SeparateSweepsWithEnclaveTasking

import jinja2

from .kernels import create_FD4InExaGRyPE_kernel_definitions_3d
from .kernels import create_DeriCalInExaGRyPE_kernel_definitions_3d
from .kernels import create_compute_kernel_for_FD4InExaGRyPE_DSL
from .kernels import create_derivative_calculation_kernel_for_FD4InExaGRyPE_DSL

from exahype2.solvers.rkfd.kernels import create_abstract_solver_declarations
from exahype2.solvers.rkfd.kernels import create_abstract_solver_definitions
from exahype2.solvers.rkfd.kernels import create_solver_declarations
from exahype2.solvers.rkfd.kernels import create_solver_definitions
#
from exahype2.solvers.rkfd.kernels import SolverVariant
from exahype2.solvers.rkfd.kernels import KernelVariant

from exahype2.solvers.rkfd.AdaptiveTimeSteppingCodeSnippets import AdaptiveTimeSteppingCodeSnippets



class GlobalAdaptiveTimeStepInExaGRyPEWithEnclaveTasking( SeparateSweepsWithEnclaveTasking ):
  def __init__(self, 
    name, patch_size, rk_order, unknowns, auxiliary_variables, min_meshcell_h, max_meshcell_h, time_step_relaxation,
    reconstruction_with_rk=False,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=PDETerms.None_Implementation, 
    point_source=PDETerms.None_Implementation,
    boundary_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.Empty_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    source_term=PDETerms.None_Implementation,
    eigenvalues=PDETerms.User_Defined_Implementation, 
    pde_terms_without_state: bool = True,
    plot_grid_properties=False, KOSigma=8.0
  ):
    """!
    
    Construct solver
    
    
    time_step_relaxation: Float
      Calibration factor of CFL condition. The Runge-Kutta order is multiplied 
      with this value following the formula for Cockburn-Shu damping. However, 
      also the actual polynomial order has to enter the chosen time step size
      through an additional @f$ p^{-2} @f$ scaling. We expect the user to 
      incorporate such an additional scaling within time_step_relaxation.
      
    """  


    if reconstruction_with_rk:
      self._reconstruction_stages=rk_order
    else:
      self._reconstruction_stages=0

    super(GlobalAdaptiveTimeStepInExaGRyPEWithEnclaveTasking,self).__init__(name, 
                                             patch_size, 
                                             3, #overlap, 
                                             rk_order, 
                                             unknowns, 
                                             auxiliary_variables, 
                                             min_meshcell_h, 
                                             max_meshcell_h, 
                                             plot_grid_properties, 
                                             kernel_namespace="fd4",
                                             pde_terms_without_state=pde_terms_without_state,
                                             ) 

    self._time_step_relaxation = time_step_relaxation
    self._compute_eigenvalue   = True

    self._KO_Sigma             = KOSigma

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
    KOSigma                 = None,
    reconstruction_with_rk= False
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.

      Please note that not all options are supported by all solvers.

      This routine should be the very last invoked by the constructor.
    """
    super(GlobalAdaptiveTimeStepInExaGRyPEWithEnclaveTasking,self).set_implementation(  
      flux, ncp, source_term, eigenvalues,
      boundary_conditions, refinement_criterion, initial_conditions, memory_location, additional_action_set_includes, additional_user_includes)

    if not                  KOSigma==None:  self._KO_Sigma                                  = KOSigma

    create_FD4InExaGRyPE_kernel_definitions_3d()

    self._compute_kernel_call = create_compute_kernel_for_FD4InExaGRyPE_DSL(
      self._flux_implementation, 
      self._ncp_implementation, 
      self._source_term_implementation, 
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant                           = SolverVariant.WithVirtualFunctions,
      kernel_variant                           = KernelVariant.PatchWiseAoSHeap,
      KOSigma                                  = self._KO_Sigma,
      SecondOrder                              = reconstruction_with_rk
    )
    self._fused_compute_kernel_call_cpu = create_compute_kernel_for_FD4InExaGRyPE_DSL(
      self._flux_implementation, 
      self._ncp_implementation, 
      self._source_term_implementation, 
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant                           = SolverVariant.Stateless,
      kernel_variant                           = KernelVariant.PatchWiseAoSHeap,
      KOSigma                                  = self._KO_Sigma,
      SecondOrder                              = reconstruction_with_rk
    )
    self._fused_compute_kernel_call_gpu = create_compute_kernel_for_FD4InExaGRyPE_DSL(
      self._flux_implementation, 
      self._ncp_implementation, 
      self._source_term_implementation, 
      compute_max_eigenvalue_of_next_time_step = True,
      solver_variant                           = SolverVariant.AcceleratorWithExplicitCopy,
      kernel_variant                           = KernelVariant.PatchWiseAoSHeap,
      KOSigma                                  = self._KO_Sigma,
      SecondOrder                              = reconstruction_with_rk
    )

    if reconstruction_with_rk:
      create_DeriCalInExaGRyPE_kernel_definitions_3d()

      self._reconstruction_kernel_call=create_derivative_calculation_kernel_for_FD4InExaGRyPE_DSL(
        solver_variant  = SolverVariant.WithVirtualFunctions, 
        kernel_variant  = KernelVariant.PatchWiseAoSHeap,
        SecondOrder     = reconstruction_with_rk)
      self._primary_variables_indices = "int primaryVarsIndices[25]={ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 53, 54};" 
      self._auxiliary_variables_indices = "int auxiliaryVarsIndices[34]={23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,55,56,57,58}; "

    solver_code_snippets = AdaptiveTimeSteppingCodeSnippets(self._time_step_relaxation)

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)
    self._abstract_solver_user_declarations += solver_code_snippets.create_abstract_solver_user_declarations()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._pde_terms_without_state)
    self._abstract_solver_user_definitions  += solver_code_snippets.create_abstract_solver_user_definitions()

    self._compute_time_step_size              = solver_code_snippets.create_compute_time_step_size()
    self._compute_new_time_step_size          = solver_code_snippets.create_compute_new_time_step_size()

    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)
    self._solver_user_definitions            = create_solver_definitions(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, False)

    self._start_time_step_implementation     = solver_code_snippets.create_start_time_step_implementation()
    self._finish_time_step_implementation    = solver_code_snippets.create_finish_time_step_implementation()
    self._constructor_implementation         = solver_code_snippets.create_abstract_solver_constructor_statements()
    
    self.create_action_sets()


  @property
  def user_action_set_includes(self):
    tem = super(GlobalAdaptiveTimeStepInExaGRyPEWithEnclaveTasking, self).user_action_set_includes + """
#include "kernels/FD4InExaGRyPE.h"
"""
    if self._reconstruction_stages > 0:
      tem = tem + """
#include "kernels/DeriCallInExaGRyPE.h"
"""

    return tem

  def add_entries_to_text_replacement_dictionary(self, d):
    """!
    
    Invoce superclass and then add one more field:
    
    - RECONSTRUCTION_STAGES
    
    """
    super(GlobalAdaptiveTimeStepInExaGRyPEWithEnclaveTasking, self).add_entries_to_text_replacement_dictionary(d)
    d["RECONSTRUCTION_STAGES"]=self._reconstruction_stages
    