# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from exahype2.solvers.new_ader.SingleSweep import SingleSweep
from exahype2.solvers.new_ader.PDETerms import PDETerms

class GlobalFixedTimeStep( SingleSweep ):
  def __init__(self,
    name,
    order, 
    polynomials, 
    unknowns, auxiliary_variables, 
    min_cell_h, max_cell_h, 
    time_step,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=None, 
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False
  ):
      
    super(GlobalFixedTimeStep, self).__init__(name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties)
    
    self._time_step = time_step
    
    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation

    self._compute_time_step_size              = ""
    self._compute_new_time_step_size          = ""
    self._compute_eigenvalue                  = True
    
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
    additional_action_set_includes = "",
    additional_user_includes       = ""
  ):
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term
    
    super(GlobalFixedTimeStep,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, additional_action_set_includes, additional_user_includes)