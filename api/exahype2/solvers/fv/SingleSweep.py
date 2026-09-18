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
    ):
        self._state_owner = None
        super(SingleSweep, self).__init__(
            name=name,
            patch_size=patch_size,
            overlap=overlap,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
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
        if self._state_owner is not None:
            dynamic_amr = (
                self._action_set_adaptive_mesh_face_coupling
            )
            dynamic_amr.d["INTERPOLATE_GUARD"] = (
                self._provide_face_data_to_compute_kernels_default_guard()
                + " and "
                + self._state_is_not("GridInitialisation")
            )

    def attach_state_owner(self, state_owner):
        """Uses another solver as the state and PDE owner."""
        if self._state_owner is not None:
            raise RuntimeError("The FV solver already has a state owner.")
        self._state_owner = state_owner

    def get_name_of_global_instance(self):
        if self._state_owner is None:
            return super().get_name_of_global_instance()
        return self._state_owner.get_name_of_global_instance()

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        if self._state_owner is None:
            return super()._provide_cell_data_to_compute_kernels_default_guard()
        return "not marker.willBeRefined() and " + self._state_is_not(
            "GridConstruction"
        )

    def _provide_face_data_to_compute_kernels_default_guard(self):
        if self._state_owner is None:
            return super()._provide_face_data_to_compute_kernels_default_guard()
        return self._provide_cell_data_to_compute_kernels_default_guard()

    def _store_cell_data_default_guard(self):
        if self._state_owner is None:
            return super()._store_cell_data_default_guard()
        return self._provide_cell_data_to_compute_kernels_default_guard()

    def _load_cell_data_default_guard(self):
        if self._state_owner is None:
            return super()._load_cell_data_default_guard()
        return (
            "not marker.hasBeenRefined() and "
            + self._state_is_not("GridConstruction")
            + " and "
            + self._state_is_not("GridInitialisation")
        )

    def _store_face_data_default_guard(self):
        if self._state_owner is None:
            return super()._store_face_data_default_guard()
        return self._provide_face_data_to_compute_kernels_default_guard()

    def _load_face_data_default_guard(self):
        if self._state_owner is None:
            return super()._load_face_data_default_guard()
        return self._load_cell_data_default_guard()

    def _store_boundary_data_default_guard(self):
        if self._state_owner is None:
            return super()._store_boundary_data_default_guard()
        return (
            self._store_face_data_default_guard()
            + " and not repositories::"
            + self.get_name_of_global_instance()
            + ".PeriodicBC[marker.getSelectedFaceNumber() % DIMENSIONS]"
            + " and not marker.hasBeenRefined() and fineGridFace"
            + self._face_label.name
            + ".getBoundary()"
        )

    def _delete_face_data_default_guard(self):
        if self._state_owner is None:
            return super()._delete_face_data_default_guard()
        return "true"

    def _interpolate_face_data_default_guard(self):
        if self._state_owner is None:
            return super()._interpolate_face_data_default_guard()
        return self._state_is_not("GridInitialisation")

    def _restrict_face_data_default_guard(self):
        if self._state_owner is None:
            return super()._restrict_face_data_default_guard()
        return "true"

    def _state_is_not(self, state):
        return (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState() != "
            + self._state_owner.name
            + "::SolverState::"
            + state
        )

    def _init_dictionary_with_default_parameters(self, dictionary):
        super()._init_dictionary_with_default_parameters(dictionary)
        if self._state_owner is None:
            return
        namespace = "::".join(self._namespace)
        dictionary["FULL_QUALIFIED_SOLVER_NAME"] = (
            namespace + "::" + self._state_owner.name
        )
        dictionary["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        dictionary["RIEMANN_SOLVER_METHOD_NAME"] = "solveLimiterRiemannProblem"
        dictionary["LIMITER_METHOD_NAME"] = "applyLimiterSlope"

    def add_implementation_files_to_project(self, namespace, output, dimensions):
        if self._state_owner is not None:
            raise RuntimeError(
                "An embedded FV backend is generated through its state owner."
            )
        super().add_implementation_files_to_project(namespace, output, dimensions)

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
        limiter=None,
    ):
        owner_pde_terms = (
            initial_conditions,
            boundary_conditions,
            refinement_criterion,
            flux,
            ncp,
            max_eigenvalue,
            source_term,
            diffusive_source_term,
        )
        if self._state_owner is not None and any(
            implementation is not None for implementation in owner_pde_terms
        ):
            raise RuntimeError("Configure PDE terms through the state owner.")

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
        D: Dictionary of string to string.

           in/out argument
        """
        pass
