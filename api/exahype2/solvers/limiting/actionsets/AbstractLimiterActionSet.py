# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet
from peano4.output.Helper import render_jinja2_template
from exahype2.solvers.LagrangeBasis import render_tensor_1
from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.limiting.ProjectionMatrices import (
    create_dmp_sampling_matrices,
)

import jinja2


class AbstractLimiterActionSet(ActionSet):
    """Renders the C++ bodies every limiter action set splices into its sweep."""

    def __init__(self, solver):
        super().__init__()
        self._solver = solver

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def _rendered_dmp_observables(self, dictionary):
        """Renders the observable mapping with its compute precision bound.

        The user's mapping names the precision it evaluates in through a
        placeholder, so it is resolved here before the mapping is spliced into a
        sweep body, where a second render would not reach it. A user-defined
        mapping delegates to the solver's member.
        """
        dictionary["COMPUTE_PRECISION"] = (
            self._solver._solution_persistent_storage_precision
        )
        observables = self._solver._subcell_limiter_dmp_observables or ""
        if observables == PDETerms.User_Defined_Implementation:
            return (
                "repositories::"
                + dictionary["SOLVER_INSTANCE"]
                + ".mapDmpObservables(observables, Q);"
            )
        return render_jinja2_template(
            observables,
            dictionary,
            jinja2.DebugUndefined,
        )

    def _physical_admissibility_criterion(self, dictionary):
        """Returns the admissibility body a sweep splices into its lambda.

        Every splice site provides the locals ``Q``, ``numberOfStates``, ``x``,
        and ``h``, so a user-defined criterion delegates to the solver's member
        with exactly these arguments.
        """
        criterion = self._solver._subcell_limiter_physical_admissibility_criterion or ""
        if criterion == PDETerms.User_Defined_Implementation:
            return (
                "return repositories::"
                + dictionary["SOLVER_INSTANCE"]
                + ".isPhysicallyAdmissible(Q, numberOfStates, x, h);"
            )
        return criterion

    def _dmp_relaxation_parameter_implementation(self, dictionary):
        """Returns the per-observable relaxation body, delegating when user-defined."""
        implementation = (
            self._solver._subcell_limiter_dmp_relaxation_parameter_implementation or ""
        )
        if implementation == PDETerms.User_Defined_Implementation:
            return (
                "return repositories::"
                + dictionary["SOLVER_INSTANCE"]
                + ".dmpRelaxationParameter(observable, specifiedRelaxationParameter);"
            )
        return implementation

    def _add_dmp_sampling_matrices_to_dictionary(self, dictionary):
        """Adds the projections that expose extrema between native DG nodes."""
        dg_to_lobatto, dg_to_subcells = create_dmp_sampling_matrices(
            self._solver._basis
        )
        dictionary["DMP_DG_TO_LOBATTO_MATRIX"] = render_tensor_1(
            [value for row in dg_to_lobatto for value in row]
        )
        dictionary["DMP_DG_TO_SUBCELL_MATRIX"] = render_tensor_1(
            [value for row in dg_to_subcells for value in row]
        )

    def get_includes(self):
        return (
            self._solver._get_default_includes()
            + self._solver.get_user_action_set_includes()
        )
