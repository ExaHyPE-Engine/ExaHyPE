# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from exahype2.solvers.new_ader.SingleSweep import SingleSweep
from exahype2.solvers.new_ader.PDETerms import PDETerms

from exahype2.solvers.fv.kernels import create_abstract_solver_declarations
from exahype2.solvers.fv.kernels import create_abstract_solver_definitions
from exahype2.solvers.fv.kernels import create_solver_declarations
from exahype2.solvers.fv.kernels import create_solver_definitions
from exahype2.solvers.fv.kernels import create_compute_time_step_size_for_fixed_time_stepping
from exahype2.solvers.kernels import create_abstract_solver_user_declarations_for_fixed_time_stepping
from exahype2.solvers.kernels import create_abstract_solver_user_definitions_for_fixed_time_stepping
from exahype2.solvers.fv.kernels import create_start_time_step_implementation_for_fixed_time_stepping
from exahype2.solvers.fv.kernels import create_finish_time_step_implementation_for_fixed_time_stepping

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
    point_source = None,
    eigenvalues=PDETerms.User_Defined_Implementation, 
    boundary_conditions=None,refinement_criterion=None,initial_conditions=None,source_term=None,
    plot_grid_properties=False
  ):
      
    super(GlobalFixedTimeStep, self).__init__(name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties)
    
    self._time_step_size = time_step
    
    #self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.None_Implementation
    self._source_term_implementation          = PDETerms.None_Implementation
    self._point_source_implementation         = PDETerms.None_Implementation

    #Used for the update cell time stepping implementation
    self._compute_time_step_size              = create_compute_time_step_size_for_fixed_time_stepping(self._time_step_size)
    self._compute_new_time_step_size          = "const double newTimeStepSize = timeStepSize;"    
    self._compute_eigenvalue                  = True
    
    self.set_implementation(flux=flux, 
      ncp=ncp, 
      eigenvalues=eigenvalues, 
      boundary_conditions=boundary_conditions, 
      refinement_criterion=refinement_criterion, 
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
    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term
    if point_source         is not None: self._point_source_implementation                = point_source
    
    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    self._abstract_solver_user_declarations += create_abstract_solver_user_declarations_for_fixed_time_stepping()    
    self._abstract_solver_user_definitions  += create_abstract_solver_user_definitions_for_fixed_time_stepping()
    
    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    self._solver_user_definitions            = create_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_source_implementation, False)
    
    self._start_time_step_implementation     = create_start_time_step_implementation_for_fixed_time_stepping(False)
    self._finish_time_step_implementation    = create_finish_time_step_implementation_for_fixed_time_stepping(self._time_step_size)
    
    super(GlobalFixedTimeStep,self).set_implementation(boundary_conditions, refinement_criterion, initial_conditions, additional_action_set_includes, additional_user_includes)
    

      
      
          