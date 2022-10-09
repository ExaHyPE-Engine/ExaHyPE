  # This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.new_ader.ADERDG         import PrecisionType
from exahype2.solvers.new_ader.SingleSweep    import SingleSweep
from exahype2.solvers.new_ader.PDETerms       import PDETerms

from exahype2.solvers.new_ader.kernels import create_abstract_solver_declarations
from exahype2.solvers.new_ader.kernels import create_abstract_solver_definitions
from exahype2.solvers.new_ader.kernels import create_solver_declarations
from exahype2.solvers.new_ader.kernels import create_solver_definitions

from exahype2.solvers.new_ader.rusanov.kernels import create_abstract_solver_user_declarations_for_adaptive_time_stepping
from exahype2.solvers.new_ader.rusanov.kernels import create_abstract_solver_user_definitions_for_adaptive_time_stepping

from exahype2.solvers.new_ader.rusanov.kernels import create_compute_time_step_size_for_adaptive_time_stepping
from exahype2.solvers.new_ader.rusanov.kernels import create_compute_new_time_step_size_for_adaptive_time_stepping

from exahype2.solvers.new_ader.rusanov.kernels import create_start_time_step_implementation_for_adaptive_time_stepping
from exahype2.solvers.new_ader.rusanov.kernels import create_finish_time_step_implementation_for_adaptive_time_stepping

from exahype2.solvers.new_ader.rusanov.kernels import create_constructor_implementation_for_adaptive_time_stepping


class GlobalAdaptiveTimeStep( SingleSweep ):
  def __init__(self, 
    name, 
    order,
    unknowns, auxiliary_variables, min_cell_h, max_cell_h, 
    time_step_relaxation,
    flux=PDETerms.User_Defined_Implementation, 
    ncp=PDETerms.None_Implementation, 
    point_source=0,
    boundary_conditions=PDETerms.User_Defined_Implementation,
    refinement_criterion=PDETerms.None_Implementation,
    initial_conditions=PDETerms.User_Defined_Implementation,
    source_term=PDETerms.None_Implementation,
    material_parameters=PDETerms.None_Implementation,
    plot_grid_properties=False,
    predictor_persistent_storage_precision=PrecisionType.double,
    predictor_computation_precision=PrecisionType.double
  ):
    super(GlobalAdaptiveTimeStep,self).__init__(name, order, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties, predictor_persistent_storage_precision, predictor_computation_precision) 
   
    self._cflSafetyFactor = time_step_relaxation

    self.set_implementation(boundary_conditions=boundary_conditions, 
                            refinement_criterion=refinement_criterion,
                            initial_conditions=initial_conditions, 
                            additional_action_set_includes="", 
                            additional_user_includes="",
                            flux=flux,
                            ncp=ncp,
                            source_term=source_term,
                            material_parameters=material_parameters,
                            point_source=point_source
                            )


  def set_implementation(self,
    flux=None,
    ncp=None,
    boundary_conditions=None,
    refinement_criterion=None,
    initial_conditions=None,
    source_term=None,
    material_parameters=None,
    point_source=None,    
    additional_action_set_includes = "",
    additional_user_includes       = ""
  ):
    super(GlobalAdaptiveTimeStep,self).set_implementation(boundary_conditions=boundary_conditions, 
                                                       refinement_criterion=refinement_criterion,
                                                       initial_conditions=initial_conditions, 
                                                       additional_action_set_includes="", 
                                                       additional_user_includes="",
                                                       flux=flux,
                                                       ncp=ncp,
                                                       eigenvalues=PDETerms.User_Defined_Implementation,
                                                       source_term=source_term,
                                                       material_parameters=material_parameters,
                                                       point_source=point_source
                                                       )

    corrector_precisions = list(set([self._predictor_computation_precision, self._corrector_computation_precision]))
    self._abstract_solver_user_declarations  = create_abstract_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_sources_implementation, corrector_precisions, False)
    self._abstract_solver_user_declarations += create_abstract_solver_user_declarations_for_adaptive_time_stepping()
    self._abstract_solver_user_definitions   = create_abstract_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_sources_implementation, corrector_precisions, False)
    self._abstract_solver_user_definitions  += create_abstract_solver_user_definitions_for_adaptive_time_stepping()
    
    self._solver_user_declarations           = create_solver_declarations(self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_sources_implementation, corrector_precisions, False)
    self._solver_user_definitions            = create_solver_definitions( self._flux_implementation, self._ncp_implementation, self._eigenvalues_implementation, self._source_term_implementation, self._point_sources_implementation, corrector_precisions, False)
    
    self._constructor_implementation         += create_constructor_implementation_for_adaptive_time_stepping()
    
    self._compute_time_step_size             = create_compute_time_step_size_for_adaptive_time_stepping()
    self._compute_new_time_step_size         = create_compute_new_time_step_size_for_adaptive_time_stepping()

    self._start_time_step_implementation     = create_start_time_step_implementation_for_adaptive_time_stepping()
    self._finish_time_step_implementation    = create_finish_time_step_implementation_for_adaptive_time_stepping()


      