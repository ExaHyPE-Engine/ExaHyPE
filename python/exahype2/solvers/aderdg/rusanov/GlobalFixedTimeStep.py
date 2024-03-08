# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.ADERDG import PrecisionType
from exahype2.solvers.aderdg.SingleSweep import SingleSweep

from exahype2.solvers.aderdg.kernels import create_abstract_solver_declarations
from exahype2.solvers.aderdg.kernels import create_abstract_solver_definitions
from exahype2.solvers.aderdg.kernels import create_solver_declarations
from exahype2.solvers.aderdg.kernels import create_solver_definitions

from exahype2.solvers.aderdg.kernels import (
    create_abstract_solver_user_declarations_for_fixed_time_stepping,
)
from exahype2.solvers.aderdg.kernels import (
    create_abstract_solver_user_definitions_for_fixed_time_stepping,
)

from exahype2.solvers.aderdg.kernels import (
    create_compute_time_step_size_for_fixed_time_stepping,
)

from exahype2.solvers.aderdg.kernels import (
    create_finish_time_step_implementation_for_fixed_time_stepping,
)
from exahype2.solvers.aderdg.kernels import (
    create_start_time_step_implementation_for_fixed_time_stepping,
)


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        time_step_size,
        flux=PDETerms.User_Defined_Implementation,
        ncp=PDETerms.None_Implementation,
        eigenvalues=PDETerms.User_Defined_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        initial_conditions=PDETerms.User_Defined_Implementation,
        source_term=PDETerms.None_Implementation,
        point_source=0,
        material_parameters=PDETerms.None_Implementation,
        plot_grid_properties=False,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name,
            order,
            unknowns,
            auxiliary_variables,
            min_cell_h,
            max_cell_h,
            plot_grid_properties,
        )

        self._time_step_size = time_step_size

        self.set_implementation(
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            source_term=source_term,
            point_source=point_source,
            material_parameters=material_parameters
        )

    def set_implementation(
        self,
        flux=PDETerms.None_Implementation,
        ncp=PDETerms.None_Implementation,
        eigenvalues=PDETerms.User_Defined_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        initial_conditions=PDETerms.User_Defined_Implementation,
        source_term=PDETerms.None_Implementation,
        point_source=0,
        material_parameters=PDETerms.None_Implementation,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        super(GlobalFixedTimeStep, self).set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes="",
            additional_user_includes="",
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            source_term=source_term,
            material_parameters=material_parameters,
            point_source=point_source,
        )

        # self._source_term_call    = create_source_term_kernel(self._source_term_implementation)
        # self._Riemann_solver_call = create_compute_Riemann_kernel_for_Rusanov(self._flux_implementation, self._ncp_implementation)

        computation_precisions = self._predictor_computation_precisions[:]
        if self._corrector_computation_precision not in computation_precisions:
            computation_precisions.append(self._corrector_computation_precision)
        if self._precompute_picard_precision!=False and self._precompute_picard_precision not in computation_precisions:
            computation_precisions.append(self._precompute_picard_precision)
        if self._solution_persistent_storage_precision not in computation_precisions:
            computation_precisions.append(self._solution_persistent_storage_precision)

        self._abstract_solver_user_declarations = create_abstract_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )

        self._abstract_solver_user_declarations += (
            create_abstract_solver_user_declarations_for_fixed_time_stepping(
                self._time_step_size
            )
        )
        self._abstract_solver_user_definitions = create_abstract_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )
        self._abstract_solver_user_definitions += (
            create_abstract_solver_user_definitions_for_fixed_time_stepping()
        )

        self._solver_user_declarations = create_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )
        self._solver_user_definitions = create_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._material_param_implementation,
            self._point_sources_implementation,
            self._is_linear,
            computation_precisions,
            False,
        )

        # Nothing fancy to be done for fixed time stepping
        # self._constructor_implementation         = create_constructor_implementation_for_adaptive_time_stepping()

        self._compute_time_step_size = (
            create_compute_time_step_size_for_fixed_time_stepping()
        )
        self._compute_new_time_step_size = (
            "const double newTimeStepSize = timeStepSize;"
        )

        self._start_time_step_implementation = (
            create_start_time_step_implementation_for_fixed_time_stepping()
        )
        self._finish_time_step_implementation = ""  # create_finish_time_step_implementation_for_fixed_time_stepping(self._time_step_size)
