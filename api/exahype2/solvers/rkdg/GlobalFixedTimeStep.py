# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.rkdg.SingleSweep import SingleSweep

from exahype2.solvers.rkdg.AbstractSolverDeclarations import (
    AbstractSolverDeclarations,
)
from exahype2.solvers.rkdg.SolverDeclarations import SolverDeclarations
from exahype2.solvers.rkdg.SolverDefinitions import SolverDefinitions
from exahype2.solvers import FixedTimeSteppingCodeSnippets


class GlobalFixedTimeStep(SingleSweep):
    """!
    RKDG solver with a global fixed time step.

    The time step size is prescribed, so the solver needs no admissible-step
    reduction.
    """
    def __init__(
        self,
        name,
        rk_order,
        dg_order,
        unknowns,
        auxiliary_variables,
        time_step_size,
        flux=PDETerms.User_Defined_Implementation,
        max_eigenvalue=PDETerms.User_Defined_Implementation,
        ncp=PDETerms.None_Implementation,
        point_source=PDETerms.None_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        initial_conditions=PDETerms.User_Defined_Implementation,
        source_term=PDETerms.None_Implementation,
        riemann_solver=PDETerms.None_Implementation,
        use_diffusive_flux=False,
    ):
        super(GlobalFixedTimeStep, self).__init__(
            name,
            rk_order,
            dg_order,
            unknowns,
            auxiliary_variables,
        )

        self._normalised_time_step_size = time_step_size

        self.set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes="",
            additional_user_includes="",
            flux=flux,
            max_eigenvalue=max_eigenvalue,
            riemann_solver=riemann_solver,
            ncp=ncp,
            source_term=source_term,
            point_source=point_source,
            use_diffusive_flux=use_diffusive_flux,
        )

    def set_implementation(
        self,
        flux=None,
        ncp=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        max_eigenvalue=None,
        source_term=None,
        point_source=None,
        additional_action_set_includes="",
        additional_user_includes="",
        riemann_solver=None,
        use_diffusive_flux=None,
    ):
        super(GlobalFixedTimeStep, self).set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes=additional_action_set_includes,
            additional_user_includes=additional_user_includes,
            flux=flux,
            ncp=ncp,
            max_eigenvalue=max_eigenvalue,
            riemann_solver=riemann_solver,
            source_term=source_term,
            point_source=point_source,
            use_diffusive_flux=use_diffusive_flux,
        )

        solver_code_snippets = FixedTimeSteppingCodeSnippets.for_rkdg(
            self._normalised_time_step_size
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

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        abstract_solver_declarations = AbstractSolverDeclarations(d)
        solver_declarations = SolverDeclarations(d)
        solver_definitions = SolverDefinitions(d)

        self._abstract_solver_user_declarations = (
            abstract_solver_declarations.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_declarations += (
            solver_code_snippets.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_definitions = (
            solver_code_snippets.create_abstract_solver_user_definitions()
        )

        self._solver_user_declarations = (
            solver_declarations.create_solver_user_declarations()
        )
        self._solver_user_definitions = (
            solver_definitions.create_solver_user_definitions()
        )
