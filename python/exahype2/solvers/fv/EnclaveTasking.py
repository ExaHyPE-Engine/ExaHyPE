# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import jinja2

from .FV import FV

import peano4.output
import exahype2.grid

from exahype2.solvers.fv.actionsets import UpdateCellWithEnclaveTasking

class EnclaveTasking(FV):
    """!
    Enclave tasking variant of the Finite Volume scheme
    """

    def __init__(
        self,
        name,
        patch_size,
        overlap,
        unknowns,
        auxiliary_variables,
        min_volume_h,
        max_volume_h,
        number_of_tasks,
        plot_grid_properties: bool,
    ):
        super(EnclaveTasking, self).__init__(
            name,
            patch_size,
            overlap,
            unknowns,
            auxiliary_variables,
            min_volume_h,
            max_volume_h,
            plot_grid_properties,
        )
        self._solver_template_file_class_name = "EnclaveTasking"

        self._number_of_tasks = number_of_tasks
        self._fused_compute_kernel_call = "#error Not yet defined"
        self._can_fuse = True

        additional_includes = """
#include "exahype2/EnclaveTask.h"
"""

        self.add_user_action_set_includes(additional_includes)
        self.enclave_task_priority = "tarch::multicore::Task::DefaultPriority-1"


    def _create_guards(self):
        """!
        All the internal logic depends on guards, i.e., boolean predicates. We
        want to be able to alter them in subclasses, but we need a certain
        baseline. It is defined in this routine.
        """
        self._initialisation_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )

        self._first_iteration_after_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PrimaryAfterGridInitialisation or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PlottingAfterGridInitialisation"
            + ")"
        )

        self._primary_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Primary or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PrimaryAfterGridInitialisation"
            + ")"
        )

        self._primary_sweep_or_plot_guard = """(
             repositories::{}.getSolverState()=={}::SolverState::Primary
          or repositories::{}.getSolverState()=={}::SolverState::PrimaryAfterGridInitialisation
          or repositories::{}.getSolverState()=={}::SolverState::Plotting
          or repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation
          or repositories::{}.getSolverState()=={}::SolverState::Suspended
        )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
        )

        self._primary_or_initialisation_sweep_guard = """(
             repositories::{}.getSolverState()=={}::SolverState::GridInitialisation
          or repositories::{}.getSolverState()=={}::SolverState::Primary
          or repositories::{}.getSolverState()=={}::SolverState::PrimaryAfterGridInitialisation
        )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
        )

        self._primary_or_grid_construction_or_initialisation_sweep_guard = """(
            repositories::{}.getSolverState()=={}::SolverState::GridInitialisation
         or repositories::{}.getSolverState()=={}::SolverState::Primary
         or repositories::{}.getSolverState()=={}::SolverState::PrimaryAfterGridInitialisation
         or repositories::{}.getSolverState()=={}::SolverState::GridConstruction
        )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
        )

        self._secondary_sweep_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Secondary"
            + ")"
        )

        self._secondary_sweep_or_grid_construction_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Secondary or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridConstruction"
            + ")"
        )

        self._secondary_sweep_or_grid_initialisation_guard = (
            "("
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Secondary or "
            + "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation"
            + ")"
        )

        self._secondary_sweep_or_grid_initialisation_or_plot_guard = """(
            repositories::{}.getSolverState()=={}::SolverState::Secondary
         or repositories::{}.getSolverState()=={}::SolverState::GridInitialisation
         or repositories::{}.getSolverState()=={}::SolverState::PlottingAfterGridInitialisation
         or repositories::{}.getSolverState()=={}::SolverState::Plotting
         or repositories::{}.getSolverState()=={}::SolverState::Suspended
        )""".format(
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            self.get_name_of_global_instance(), self._name,
            )


    def create_data_structures(self):
        """
        This routine does not really add new data, but it heavily tailors when data are
        stored, exchanged, ... Each generator has some guard attributes, i.e., some guards,
        which control when data is stored, sent, received. The routine takes these guards
        and rewires them to the local guards of this object. If you alter these guards
        further, you have to alter them before you invoke this class' create_data_structures().
        """
        super(EnclaveTasking, self).create_data_structures()

        self._create_guards()

        self._patch_overlap_new.generator.send_condition = (
            self._secondary_sweep_or_grid_initialisation_or_plot_guard
        )
        self._patch_overlap_new.generator.receive_and_merge_condition = (
            self._primary_sweep_or_plot_guard
        )

        self._patch_overlap_old.generator.send_condition = (
            self._initialisation_sweep_guard
        )
        self._patch_overlap_old.generator.receive_and_merge_condition = (
            self._first_iteration_after_initialisation_guard
        )


    def create_action_sets(self):
        """
        Adaptive mesh handing

        Adaptive meshes require us to clear the patch overlaps and to restrict/interpolate.
        Obviously, there's no need to do this for a refined faces. So we can eliminate these
        cases a priori. Furthermore, we clear faces only in the primary sweep. We know that
        either the primary sweep (for skeleton) or the secondary sweep (for enclaves) will
        write in proper data into anything that's cleared, and we know that restriction only
        has to happen after the primary sweep, as all cells next to an adaptivity boundary
        are skeleton cells.

        As pointed out, both interpolation and restriction are to be active for the first
        sweep only. We interpolate into hanging faces, and we have to restrict immediately
        again as they are non-persistent. The projection onto the (hanging) faces is also
        happening directly in the primary sweep, as the cells adjacent to the hanging
        face are skeleton cells.

        # TODO: Revise
        AMR and adjust cell have to be there always, i.e., also throughout
        the grid construction. But the criterion is something that we only
        evaluate in the secondary sweep. That's when we have an updated/changed time step.
        If we identify coarsening and refinement instructions in the secondary sweep, the
        next primary one will actually see them and trigger the update. That is, the
        subsequent secondary switch will actually implement the grid changes, and we can
        evaluate the criteria again.

        For dynamic AMR, this implies that we have to ensure that all changed grid parts
        are labelled as skeleton cells. This way, we can implement the AMR properly, we
        ensure that all the enclaves run in parallel, and we know that all data is held
        persistently on the stacks.
        """
        super(EnclaveTasking, self).create_action_sets()

        self._action_set_update_cell = UpdateCellWithEnclaveTasking(self)

        self._action_set_initial_conditions.guard = (
            self._action_set_initial_conditions.guard
        )
        self._action_set_initial_conditions_for_grid_construction.guard = (
            self._action_set_initial_conditions_for_grid_construction.guard
        )
        self._action_set_AMR.guard = self._secondary_sweep_guard
        self._action_set_AMR_commit_without_further_analysis.guard = (
            self._secondary_sweep_guard
        )
        # We do not set the guard of the secondary sweep
        # self._action_set_postprocess_solution.guard = self._secondary_sweep_guard

        self._action_set_handle_boundary.guard = (
            self._store_face_data_default_guard()
            + " and "
            + self._primary_or_initialisation_sweep_guard
        )
        self._action_set_project_patch_onto_faces.guard = (
            self._store_cell_data_default_guard()
            + " and ("
            + "(repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Primary and marker.willBeSkeletonCell()) "
            + "or (repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::PrimaryAfterGridInitialisation  and marker.willBeSkeletonCell()) "
            + "or (repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::Secondary and marker.willBeEnclaveCell()) "
            + "or (repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation)"
            + ")"
        )
        self._action_set_roll_over_update_of_faces.guard = (
            self._store_face_data_default_guard()
            + " and "
            + self._secondary_sweep_or_grid_initialisation_guard
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guard = (
            self._store_cell_data_default_guard()
            + " and "
            + self._secondary_sweep_or_grid_initialisation_guard
        )


    def set_implementation(
        self,
        boundary_conditions,
        refinement_criterion,
        initial_conditions,
        memory_location,
        additional_action_set_includes,
        additional_user_includes,
    ):
        """
        If you pass in User_Defined, then the generator will create C++ stubs
        that you have to befill manually. If you pass in None_Implementation, it
        will create nop, i.e., no implementation or defaults. Any other string
        is copied 1:1 into the implementation. If you pass in None, then the
        set value so far won't be overwritten.
        """
        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions
        if memory_location is not None:
            self._reconstructed_array_memory_location = memory_location

        if refinement_criterion == exahype2.solvers.PDETerms.None_Implementation:
            assert False, "Refinement criterion cannot be none"
        if initial_conditions == exahype2.solvers.PDETerms.None_Implementation:
            assert False, "Initial conditions cannot be none"

        self.create_action_sets()


    def add_entries_to_text_replacement_dictionary(self, d):
        d["NUMBER_OF_TASKS"] = self._number_of_tasks

        d["FUSED_COMPUTE_KERNEL_CALL"] = jinja2.Template(
            self._fused_compute_kernel_call, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["CAN_FUSE"] = self._can_fuse

        d["SEMAPHORE_LABEL"] = exahype2.grid.UpdateCellLabel.get_attribute_name(
            self._name
        )
        d["ENCLAVE_TASK_PRIORITY"] = self.enclave_task_priority


    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        super(EnclaveTasking, self).add_actions_to_create_grid(
            step, evaluate_refinement_criterion
        )
        step.add_action_set(exahype2.grid.UpdateCellLabel(self._name))


    def add_actions_to_init_grid(self, step):
        super(EnclaveTasking, self).add_actions_to_init_grid(step)
        step.add_action_set(exahype2.grid.UpdateCellLabel(self._name))


    def add_actions_to_perform_time_step(self, step):
        """!
        Add enclave aspect to time stepping

        There's a bunch of different things to do to extend my standard solver
        into an enclave solver. In this operation, we add the runtime logic,
        i.e., what happens at which point.

        We need additional action sets that are
        triggered throughout the traversal in every second time step. I call this
        one task_based_implementation_primary_iteration or secondary,
        respectively. One wraps the implementation of _HandleCellTemplate into a
        task, the other communicates with the task bookkeeping only. Both rely on
        additional labels within the cell. We therefore end up with three new
        action sets: reconstruct_patch_and_apply_FV_kernel, exahype2.grid.UpdateCellLabel
        and roll_over_enclave_task_results.
        """
        super(EnclaveTasking, self).add_actions_to_perform_time_step(step)


    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory=""):
        super(EnclaveTasking, self).add_implementation_files_to_project(
            namespace, output, dimensions, subdirectory
        )

        templatefile_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            "EnclaveTasking.EnclaveTask.template",
        )

        if(subdirectory):
            subdirectory += "/"

        implementationDictionary = {}
        self._init_dictionary_with_default_parameters(implementationDictionary)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)
        implementationDictionary["DIMENSIONS"] = dimensions

        # Some includes might logically belong into the action sets, but now they are
        # 'outsourced' into the enclave task. So we manually add it here.
        implementationDictionary["SOLVER_INCLUDES"] += self.user_solver_includes
        implementationDictionary["SOLVER_INCLUDES"] += self.user_action_set_includes

        task_name = self._enclave_task_name()
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                headfile_template="{}.h".format(templatefile_prefix),
                cppfile_template="{}.cpp".format(templatefile_prefix),
                classname=task_name,
                namespace=namespace + ["tasks"],
                subdirectory=subdirectory + "tasks",
                dictionary=implementationDictionary,
                default_overwrite=True,
                apply_iteratively=True
            )
        )

        output.add(generated_solver_files)
        output.makefile.add_h_file(subdirectory + "tasks/" + task_name + ".h", generated=True)
        output.makefile.add_cpp_file(subdirectory + "tasks/" + task_name + ".cpp", generated=True)


    def _enclave_task_name(self):
        return "{}EnclaveTask".format(self._name)
