# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.rkdg.SeparateSweeps import SeparateSweeps
from exahype2.solvers.rkdg.PDETerms       import PDETerms


#from exahype2.solvers.fv.kernels import create_source_term_kernel
#from exahype2.solvers.fv.kernels import create_compute_Riemann_kernel_for_Rusanov
from exahype2.solvers.fv.kernels import create_abstract_solver_declarations
from exahype2.solvers.fv.kernels import create_abstract_solver_definitions
from exahype2.solvers.fv.kernels import create_solver_declarations
from exahype2.solvers.fv.kernels import create_solver_definitions


class GlobalAdaptiveTimeStep( SeparateSweeps ):
  def __init__(self, 
    name, 
    dg_order, 
    rk_order,
    polynomials,
    unknowns, auxiliary_variables, min_cell_h, max_cell_h, time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=PDETerms.None_Implementation, 
    point_source=PDETerms.None_Implementation,
    boundary_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.None_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    source_term=PDETerms.None_Implementation,
    plot_grid_properties=False
  ):
    super(GlobalAdaptiveTimeStep,self).__init__(name, dg_order, rk_order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties) 
   
    self._time_step_relaxation = time_step_relaxation

    self.set_implementation(flux=flux, 
      ncp=ncp, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
      eigenvalues=PDETerms.User_Defined_Implementation,
      initial_conditions=initial_conditions,
      point_source=point_source,
      source_term=source_term )


  def set_implementation(self,
    flux=None,ncp=None,
    eigenvalues=None,
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,point_source=None,
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
    super(GlobalAdaptiveTimeStep,self).set_implementation(boundary_conditions=boundary_conditions, 
                                                          refinement_criterion=refinement_criterion,
                                                          initial_conditions=initial_conditions, 
                                                          additional_action_set_includes=additional_action_set_includes, 
                                                          additional_user_includes=additional_user_includes,
                                                          flux=flux,
                                                          ncp=ncp,
                                                          eigenvalues=eigenvalues,
                                                          source_term=source_term,
                                                          point_source=point_source
                                                          )

    #self._source_term_call    = create_source_term_kernel(self._source_term_implementation)
    #self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation)

    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    #self._abstract_solver_user_declarations += create_abstract_solver_user_declarations_for_adaptive_time_stepping()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    #self._abstract_solver_user_definitions  += create_abstract_solver_user_definitions_for_adaptive_time_stepping()
    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    self._solver_user_definitions            = create_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    #self._constructor_implementation         = create_constructor_implementation_for_adaptive_time_stepping()
    
    #self._start_time_step_implementation          = create_start_time_step_implementation_for_adaptive_time_stepping(False)
    #self._finish_time_step_implementation         = create_finish_time_step_implementation_for_adaptive_time_stepping(self._time_step_relaxation)
    
