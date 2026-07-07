# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .FV import FV

from .AbstractSolverDeclarations import AbstractSolverDeclarations
from .SolverDeclarations import SolverDeclarations
from .SolverDefinitions import SolverDefinitions

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.fv.actionsets import UpdatePatch

class SingleSweep(FV):
    """
    Probably the simplest solver you could think off.

    :: Write your own specialisation ::

    self._preprocess_reconstructed_patch
      Has to hold any preprocessing, but it also has to set the doubles
      timeStepSize and timeStamp to valid data.

    self._postprocess_updated_patch
      You don't have to redefine this one, but if you want to alter the
      time step size, then this is the right place to do so. If you don't
      alter timeStepSize, the code will automatically continue with
      the current one subject to a preprocessing in the next step.
    """

    def __init__(
        self,
        name,
        patch_size,
        overlap,
        unknowns,
        auxiliary_variables,
        fuse_compute_kernels=False,
    ):
        super(SingleSweep, self).__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            fuse_compute_kernels=fuse_compute_kernels,
        )

        self._solver_template_file_class_name = "SingleSweep"

    def create_action_sets(self):
        """
        Call superclass routine and then reconfigure for the single sweep setup.
        This operation is implicitly called via the superconstructor.
        """
        super(SingleSweep, self).create_action_sets()
        self._action_set_update_patch = UpdatePatch(self)
        self._action_set_update_patch.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )

    def set_implementation(
        self,
        initial_conditions=None,
        boundary_conditions=None,
        refinement_criterion=None,
        flux=None,
        ncp=None,
        max_eigenvalue=None,
        riemann_solver=None,
        source_term=None,
        diffusive_source_term=None,
        adjust_solution=None,
        limiter=None,
    ):
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions
            if initial_conditions is not PDETerms.User_Defined_Implementation:
                self._use_stateless_initial_conditions = True

        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
            if refinement_criterion is not PDETerms.User_Defined_Implementation:
                self._use_stateless_refinement_criterion = True

        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
            if boundary_conditions is not PDETerms.User_Defined_Implementation:
                self._use_stateless_boundary_conditions = True

        if flux is not None:
            self._flux_implementation = flux
            if flux is not PDETerms.User_Defined_Implementation:
                self._use_stateless_flux = True

        if ncp is not None:
            self._ncp_implementation = ncp
            if ncp is not PDETerms.User_Defined_Implementation:
                self._use_stateless_ncp = True

        if max_eigenvalue is not None:
            self._max_eigenvalue_implementation = max_eigenvalue
            if max_eigenvalue is not PDETerms.User_Defined_Implementation:
                self._use_stateless_max_eigenvalue = True

        if riemann_solver is not None:
            self._riemann_solver_implementation = riemann_solver
            if riemann_solver is not PDETerms.User_Defined_Implementation:
                self._use_stateless_riemann_solver = True

        if source_term is not None:
            self._source_term_implementation = source_term
            if source_term is not PDETerms.User_Defined_Implementation:
                self._use_stateless_source_term = True

        if diffusive_source_term is not None:
            self._diffusive_source_term_implementation = diffusive_source_term
            if diffusive_source_term is not PDETerms.User_Defined_Implementation:
                self._use_stateless_diffusive_source_term = True

        if adjust_solution is not None:
            self._adjust_solution_implementation = adjust_solution
            if adjust_solution is not PDETerms.User_Defined_Implementation:
                self._use_stateless_adjust_solution = True

        if limiter is not None:
            self._limiter_implementation = limiter
            if limiter is not PDETerms.User_Defined_Implementation:
                self._use_stateless_limiter = True

        abstract_solver_declarations = AbstractSolverDeclarations()
        solver_declarations = SolverDeclarations()
        solver_definitions = SolverDefinitions()

        self._abstract_solver_user_declarations = (
            abstract_solver_declarations.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_declarations += (
            self._time_stepping_code_snippets.create_abstract_solver_user_declarations()
        )
        self._abstract_solver_user_definitions = (
            self._time_stepping_code_snippets.create_abstract_solver_user_definitions()
        )
        self._solver_user_declarations = (
            solver_declarations.create_solver_user_declarations()
        )
        self._solver_user_definitions = (
            solver_definitions.create_solver_user_definitions()
        )

        self._compute_time_step_size = (
            self._time_stepping_code_snippets.create_compute_time_step_size()
        )
        self._compute_new_time_step_size = (
            self._time_stepping_code_snippets.create_compute_new_time_step_size()
        )
        self._start_time_step_implementation = (
            self._time_stepping_code_snippets.create_start_time_step_implementation()
        )
        self._finish_time_step_implementation = (
            self._time_stepping_code_snippets.create_finish_time_step_implementation()
        )
        self._constructor_implementation = (
            self._time_stepping_code_snippets.create_abstract_solver_constructor_statements()
        )

        self.create_action_sets()

    def add_entries_to_text_replacement_dictionary(self, d):
        """
        d: Dictionary of string to string
           in/out argument
        """
        pass
