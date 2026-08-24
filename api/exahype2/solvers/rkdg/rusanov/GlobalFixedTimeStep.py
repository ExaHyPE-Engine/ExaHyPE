# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.rkdg.SeparateSweeps import SeparateSweeps

from exahype2.solvers.rkdg.kernels import create_abstract_solver_declarations
from exahype2.solvers.rkdg.kernels import create_abstract_solver_definitions
from exahype2.solvers.rkdg.kernels import create_solver_declarations
from exahype2.solvers.rkdg.kernels import create_solver_definitions

from exahype2.solvers.rkdg.kernels import SolverVariant

from exahype2.solvers.rkdg.kernels import create_volumetric_solver_call
from exahype2.solvers.rkdg.kernels import create_add_solver_contributions_call
from exahype2.solvers.rkdg.kernels import create_multiply_with_inverted_mass_matrix_call
from exahype2.solvers.rkdg.rusanov.kernels import create_Riemann_solver_call

from exahype2.solvers import FixedTimeSteppingCodeSnippets

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections


class GlobalFixedTimeStep(SeparateSweeps):
    """!

    RKDG solver with Rusanov Riemann solver and global fixed time step.

    We take over all parameters from the superclass. The major difference is
    that the time step size is prescribed rather than derived from an
    eigenvalue, so the solver needs no admissible-step reduction.
    """
    def __init__(
        self,
        name,
        rk_order,
        polynomials,
        unknowns,
        auxiliary_variables,
        time_step_size,
        flux=PDETerms.User_Defined_Implementation,
        eigenvalues=PDETerms.User_Defined_Implementation,
        ncp=PDETerms.None_Implementation,
        point_source=PDETerms.None_Implementation,
        boundary_conditions=PDETerms.User_Defined_Implementation,
        refinement_criterion=PDETerms.Empty_Implementation,
        initial_conditions=PDETerms.User_Defined_Implementation,
        source_term=PDETerms.None_Implementation,
        pde_terms_without_state=False,
    ):
        """!

        Construct RKDG solver with fixed time step.

        For Rusanov, we only need the solution along the face, so one quantity
        per PDE has to be projected.
        """
        super(GlobalFixedTimeStep, self).__init__(
            name,
            rk_order,
            polynomials,
            FaceProjections.Solution,
            unknowns,
            auxiliary_variables,
            pde_terms_without_state
        )

        self._kernel_namespace                                = "rusanov"
        self._normalised_time_step_size                       = time_step_size

        self._volumetric_compute_kernel_call                  = create_volumetric_solver_call(self._basis, SolverVariant.WithVirtualFunctions)
        self._volumetric_compute_kernel_call_stateless        = create_volumetric_solver_call(self._basis, SolverVariant.Stateless)
        self._Riemann_compute_kernel_call                     = create_Riemann_solver_call(self._basis, FaceProjections.Solution)
        # TODO: A stateless variant of the Riemann compute kernel call is missing.
        self._add_solver_contributions_call                   = create_add_solver_contributions_call(self._basis)
        self._multiply_with_inverted_mass_matrix_call         = create_multiply_with_inverted_mass_matrix_call(self._basis)

        self.set_implementation(
            boundary_conditions=boundary_conditions,
            refinement_criterion=refinement_criterion,
            initial_conditions=initial_conditions,
            additional_action_set_includes="",
            additional_user_includes="",
            flux=flux,
            eigenvalues=eigenvalues,
            ncp=ncp,
            source_term=source_term,
            point_source=point_source,
        )

    def set_implementation(
        self,
        flux=None,
        ncp=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        eigenvalues=None,
        source_term=None,
        point_source=None,
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
            point_source=point_source,
        )

        solver_code_snippets = FixedTimeSteppingCodeSnippets.for_rkdg(
            self._normalised_time_step_size
        )

        self._abstract_solver_user_declarations = create_abstract_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._point_sources_implementation,
            self._pde_terms_without_state,
        )
        self._abstract_solver_user_declarations += (
            solver_code_snippets.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_definitions = create_abstract_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._point_sources_implementation,
            self._pde_terms_without_state,
        )
        self._abstract_solver_user_definitions += (
            solver_code_snippets.create_abstract_solver_user_definitions()
        )

        self._solver_user_declarations = create_solver_declarations(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._point_sources_implementation,
            self._pde_terms_without_state,
        )
        self._solver_user_definitions = create_solver_definitions(
            self._flux_implementation,
            self._ncp_implementation,
            self._eigenvalues_implementation,
            self._source_term_implementation,
            self._point_sources_implementation,
            self._pde_terms_without_state,
        )

        self._compute_time_step_size             = solver_code_snippets.create_compute_time_step_size()
        self._compute_new_time_step_size         = solver_code_snippets.create_compute_new_time_step_size()

        self._start_time_step_implementation     = solver_code_snippets.create_start_time_step_implementation()
        self._finish_time_step_implementation    = solver_code_snippets.create_finish_time_step_implementation()
        self._constructor_implementation         = solver_code_snippets.create_abstract_solver_constructor_statements()

    @property
    def user_action_set_includes(self):
        return (
            super(GlobalFixedTimeStep, self).user_action_set_includes
            + """
#include "exahype2/dg/rusanov/Rusanov.h"
"""
        )
