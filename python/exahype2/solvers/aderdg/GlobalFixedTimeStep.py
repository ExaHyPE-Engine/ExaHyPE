# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms

from .SingleSweep import SingleSweep
from .AbstractSolverDeclarations import AbstractSolverDeclarations
from .AbstractSolverDefinitions import AbstractSolverDefinitions
from .SolverDeclarations import SolverDeclarations
from .SolverDefinitions import SolverDefinitions
from .FixedTimeSteppingCodeSnippets import FixedTimeSteppingCodeSnippets


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        normalised_time_step_size,
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

        self._normalised_time_step_size = normalised_time_step_size

        self.set_implementation(
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            source_term=source_term,
            point_source=point_source,
            material_parameters=material_parameters,
        )

    def set_implementation(
        self,
        flux=None,
        ncp=None,
        eigenvalues=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        source_term=None,
        point_source=0,
        material_parameters=None,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        super(GlobalFixedTimeStep, self).set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes=additional_action_set_includes,
            additional_user_includes=additional_user_includes,
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            source_term=source_term,
            material_parameters=material_parameters,
            point_source=point_source,
        )

        computation_precisions = self._predictor_computation_precisions[:]
        if self._corrector_computation_precision not in computation_precisions:
            computation_precisions.append(self._corrector_computation_precision)
        if (
            self._precompute_picard_precision != False
            and self._precompute_picard_precision not in computation_precisions
        ):
            computation_precisions.append(self._precompute_picard_precision)
        if self._solution_persistent_storage_precision not in computation_precisions:
            computation_precisions.append(self._solution_persistent_storage_precision)

        solver_code_snippets = FixedTimeSteppingCodeSnippets(self._normalised_time_step_size)
        abstract_solver_declarations = AbstractSolverDeclarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            source_term_implementation=self._source_term_implementation,
            material_parameters_implementation=self._material_param_implementation,
            point_source_implementation=self._point_sources_implementation,
            is_linear=self._is_linear,
            computation_precisions=computation_precisions,
            pde_terms_without_state=False,
        )
        abstract_solver_definitions = AbstractSolverDefinitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            source_term_implementation=self._source_term_implementation,
            material_parameters_implementation=self._material_param_implementation,
            point_source_implementation=self._point_sources_implementation,
            is_linear=self._is_linear,
            computation_precisions=computation_precisions,
            pde_terms_without_state=False,
        )
        solver_declarations = SolverDeclarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            source_term_implementation=self._source_term_implementation,
            material_parameters_implementation=self._material_param_implementation,
            point_source_implementation=self._point_sources_implementation,
            is_linear=self._is_linear,
            computation_precisions=computation_precisions,
            pde_terms_without_state=False,
        )
        solver_definitions = SolverDefinitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            source_term_implementation=self._source_term_implementation,
            material_parameters_implementation=self._material_param_implementation,
            point_source_implementation=self._point_sources_implementation,
            is_linear=self._is_linear,
            computation_precisions=computation_precisions,
            pde_terms_without_state=False,
        )

        self._abstract_solver_user_declarations = (
            abstract_solver_declarations.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_declarations += (
            solver_code_snippets.create_abstract_solver_user_declarations()
        )

        self._abstract_solver_user_definitions = (
            abstract_solver_definitions.create_abstract_solver_user_definitions()
        )
        self._abstract_solver_user_definitions += (
            solver_code_snippets.create_abstract_solver_user_definitions()
        )

        self._solver_user_declarations = (
            solver_declarations.create_solver_user_declarations()
        )
        self._solver_user_definitions = (
            solver_definitions.create_solver_user_definitions()
        )

        self._compute_time_step_size = (
            solver_code_snippets.create_compute_time_step_size()
        )
        self._compute_new_time_step_size = (
            solver_code_snippets.create_compute_new_time_step_size()
        )

        self._start_time_step_implementation = (
            solver_code_snippets.create_start_time_step_implementation()
        )
        self._finish_time_step_implementation = (
            solver_code_snippets.create_finish_time_step_implementation()
        )

        self._constructor_implementation = (
            solver_code_snippets.create_abstract_solver_constructor_statements()
        )
