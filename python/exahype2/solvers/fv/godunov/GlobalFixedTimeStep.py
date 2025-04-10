# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .GenerateKernels import *

from exahype2.solvers import PDETerms
from exahype2.solvers.AbstractSolverDeclarations import AbstractSolverDeclarations
from exahype2.solvers.AbstractSolverDefinitions import AbstractSolverDefinitions
from exahype2.solvers.SolverDeclarations import SolverDeclarations
from exahype2.solvers.SolverDefinitions import SolverDefinitions

from exahype2.solvers.fv.SingleSweep import SingleSweep
from exahype2.solvers.fv.FixedTimeSteppingCodeSnippets import FixedTimeSteppingCodeSnippets


class GlobalFixedTimeStep(SingleSweep):
    def __init__(
        self,
        name,
        patch_size,
        unknowns,
        auxiliary_variables,
        min_volume_h,
        max_volume_h,
        normalised_time_step_size,
        initial_conditions=PDETerms.User_Defined_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        flux=PDETerms.None_Implementation,
        ncp=PDETerms.None_Implementation,
        eigenvalues=PDETerms.None_Implementation,
        max_eigenvalue=PDETerms.None_Implementation,
        riemann_solver=PDETerms.None_Implementation,
        source_term=PDETerms.None_Implementation,
        adjust_solution=PDETerms.None_Implementation,
        pde_terms_without_state=False,
        plot_grid_properties=False,
        overlap=1,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_volume_h=min_volume_h,
            max_volume_h=max_volume_h,
            pde_terms_without_state=pde_terms_without_state,
            plot_grid_properties=plot_grid_properties,
        )
        self._normalised_time_step_size = normalised_time_step_size

        self._flux_implementation = PDETerms.None_Implementation
        self._ncp_implementation = PDETerms.None_Implementation
        self._eigenvalues_implementation = PDETerms.None_Implementation
        self._max_eigenvalue_implementation = PDETerms.None_Implementation
        self._riemann_solver_implementation = PDETerms.None_Implementation
        self._source_term_implementation = PDETerms.None_Implementation
        self._adjust_solution_implementation = PDETerms.None_Implementation

        self._compute_eigenvalue = False

        self.set_implementation(
            initial_conditions=initial_conditions,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            flux=flux,
            ncp=ncp,
            eigenvalues=eigenvalues,
            max_eigenvalue=max_eigenvalue,
            riemann_solver=riemann_solver,
            source_term=source_term,
            adjust_solution=adjust_solution
        )

    def set_implementation(
        self,
        initial_conditions=None,
        boundary_conditions=None,
        refinement_criterion=None,
        flux=None,
        ncp=None,
        eigenvalues=None,
        max_eigenvalue=None,
        riemann_solver=None,
        source_term=None,
        adjust_solution=None,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        super(GlobalFixedTimeStep, self).set_implementation(
            initial_conditions=initial_conditions,
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            additional_action_set_includes=additional_action_set_includes,
            additional_user_includes=additional_user_includes,
            memory_location=None,
        )

        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions
        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
        if flux is not None:
            self._flux_implementation = flux
        if ncp is not None:
            self._ncp_implementation = ncp
        if max_eigenvalue is not None:
            self._max_eigenvalue_implementation = max_eigenvalue
        if eigenvalues is not None:
            self._eigenvalues_implementation = eigenvalues
        if riemann_solver is not None:
            self._riemann_solver_implementation = riemann_solver
        if source_term is not None:
            self._source_term_implementation = source_term
        if adjust_solution is not None:
            self._adjust_solution_implementation = adjust_solution

        solver_code_snippets = FixedTimeSteppingCodeSnippets(
            self._normalised_time_step_size
        )

        abstract_solver_declarations = AbstractSolverDeclarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            max_eigenvalue_implementation=self._max_eigenvalue_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            adjust_solution_implementation=self._adjust_solution_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
            precision=self._compute_precision
        )
        abstract_solver_definitions = AbstractSolverDefinitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            max_eigenvalue_implementation=self._max_eigenvalue_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            adjust_solution_implementation=self._adjust_solution_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
            precision=self._compute_precision
        )
        solver_declarations = SolverDeclarations(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            max_eigenvalue_implementation=self._max_eigenvalue_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            adjust_solution_implementation=self._adjust_solution_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
            precision=self._compute_precision
        )
        solver_definitions = SolverDefinitions(
            flux_implementation=self._flux_implementation,
            ncp_implementation=self._ncp_implementation,
            eigenvalues_implementation=self._eigenvalues_implementation,
            max_eigenvalue_implementation=self._max_eigenvalue_implementation,
            riemann_solver_implementation=self._riemann_solver_implementation,
            source_term_implementation=self._source_term_implementation,
            adjust_solution_implementation=self._adjust_solution_implementation,
            pde_terms_without_state=self._pde_terms_without_state,
            precision=self._compute_precision
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

    def _generate_kernels(self, namespace, output, subdirectory, dimensions):
        generate_kernels(
            solver=self,
            namespace=namespace,
            output=output,
            subdirectory=subdirectory,
            dimensions=dimensions,
        )
