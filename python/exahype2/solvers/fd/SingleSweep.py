# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .FD import FD

from .AbstractSolverDeclarations import AbstractSolverDeclarations
from .SolverDeclarations import SolverDeclarations
from .SolverDefinitions import SolverDefinitions

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.fd.actionsets import UpdatePatch, EnclaveTasking
from peano4.toolbox.blockstructured.ReconstructPatch import ReconstructPatch


class SingleSweep(FD):
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
        order,
        unknowns,
        auxiliary_variables,
        min_h,
        max_h,
        use_enclave_tasking: bool,
        number_of_enclave_tasks: int,
        plot_grid_properties: bool,
        solvers_before_in_coupling: list,
    ):
        super(SingleSweep, self).__init__(
            name=name,
            patch_size=patch_size,
            order=order,
            unknowns=unknowns,
            auxiliary_variables=auxiliary_variables,
            min_h=min_h,
            max_h=max_h,
            use_enclave_tasking=use_enclave_tasking,
            number_of_enclave_tasks=number_of_enclave_tasks,
            plot_grid_properties=plot_grid_properties,
            solvers_before_in_coupling=solvers_before_in_coupling,
        )

        self._solver_template_file_class_name = "SingleSweep"

    def create_data_structures(self):
        """
        Call the superclass' create_data_structures() to ensure that all the data
        structures are in place, i.e., each cell can host a patch, that each face hosts
        patch overlaps, and so forth. These quantities are all set to defaults. See
        FV.create_data_structures().

        After that, take the patch overlap (that's the data stored within the faces)
        and ensure that these are sent and received via MPI whenever they are also
        stored persistently. The default in FV is that no domain boundary data exchange
        is active. Finally, ensure that the old data is only exchanged between the
        initialisation sweep and the first first grid run-through.
        """
        super(SingleSweep, self).create_data_structures()

        initialisation_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )

        first_iteration_after_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::IsSolvingAfterGridInitialisation or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PlottingAfterGridInitialisation"
            + ")"
        )

        self._patch_overlap_new.generator.send_condition = "true"
        self._patch_overlap_new.generator.receive_and_merge_condition = "true"

        self._patch_overlap_old.generator.send_condition = initialisation_sweep_guard
        self._patch_overlap_old.generator.receive_and_merge_condition = (
            first_iteration_after_initialisation_guard
        )

    def create_action_sets(self):
        """
        Call superclass routine and then reconfigure the update cell call.
        This operation is implicitly called via the superconstructor.
        """
        super(SingleSweep, self).create_action_sets()
        self._action_set_update_patch = UpdatePatch(self)
        self._action_set_enclave_tasking = EnclaveTasking(self)
        self._action_set_reconstruct_patch = ReconstructPatch(
            patch=self._patch,
            patch_overlap=self._patch_overlap_new,
            patch_reconstructed=self._patch_reconstructed,
            solver_name=self._name,
            precision=self._storage_precision,
            guard="not marker.willBeRefined() and not marker.hasBeenRefined()"
            + (" and marker.willBeSkeletonCell()" if self._use_enclave_tasking else ""),
            add_assertions_to_halo_exchange=True,
        )

    def set_implementation(
        self,
        initial_conditions=None,
        boundary_conditions=None,
        refinement_criterion=None,
        max_eigenvalue=None,
        flux=None,
        source_term=None,
        solver=None,
    ):
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions
            if initial_conditions is not PDETerms.User_Defined_Implementation:
                self._use_stateless_initial_conditions = True
            else:
                self._can_fuse = False  # We can only fuse if all terms are stateless

        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
            if refinement_criterion is not PDETerms.User_Defined_Implementation:
                self._use_stateless_refinement_criterion = True
            else:
                self._can_fuse = False

        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
            if boundary_conditions is not PDETerms.User_Defined_Implementation:
                self._use_stateless_boundary_conditions = True
            else:
                self._can_fuse = False

        if max_eigenvalue is not None:
            self._max_eigenvalue_implementation = max_eigenvalue
            if max_eigenvalue is not PDETerms.User_Defined_Implementation:
                self._use_stateless_max_eigenvalue = True
            else:
                self._can_fuse = False

        if flux is not None:
            self._flux_implementation = flux
            if flux is not PDETerms.User_Defined_Implementation:
                self._use_stateless_flux = True
            else:
                self._can_fuse = False

        if source_term is not None:
            self._source_term_implementation = source_term
            if source_term is not PDETerms.User_Defined_Implementation:
                self._use_stateless_source_term = True
            else:
                self._can_fuse = False

        if solver is not None:
            self._solver_implementation = solver
            if solver is not PDETerms.User_Defined_Implementation:
                self._use_stateless_solver = True
            else:
                self._can_fuse = False

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
        self._print_time_step_implementation = (
            self._time_stepping_code_snippets.create_print_time_step_implementation()
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
