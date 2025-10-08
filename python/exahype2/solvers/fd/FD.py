# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2

import peano4.datamodel
from peano4.Logger import log

import exahype2.solvers.fd.actionsets

from exahype2.solvers import PDETerms
from exahype2.solvers.Storage import Storage


class FD(object):
    """!

    See FV.py for documentation

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
        baseline_action_set_descend_invocation_order=0,
    ):
        """!
        See FV.py for any parameter not listed here

        order: int
           That's the order of the (highest) derivative this solver should be able
           to compute. A value of n will give the user n right and n left neighbours.
           The order has to be bigger or equal to one. It has to be smaller or equal
           to patch_size.

        min_h: double
           This size refers to the minimum distance between nodes.

        max_h: double
           This size refers to the maximum distance between nodes.
        """
        self._name = name

        "Set from the project script to choose between PlotPatchesInPeanoBlockFormat and PlotPatchesInHDF5Format"
        self._plotter_option = "PeanoPatch"

        self._plot_grid_properties = plot_grid_properties
        self._plot_description = ""
        self._select_dofs_to_print = None

        self._min_h = min_h
        self._max_h = max_h

        if order < 1:
            raise Exception("order (" + str(order) + ") is smaller than 1")

        if order > patch_size:
            raise Exception(
                "order ("
                + str(order)
                + ") is bigger than patch_size ("
                + str(patch_size)
                + ")"
            )

        self._patch_size = patch_size
        self._order = order

        self._solvers_before_in_coupling = solvers_before_in_coupling
        self._last_solver = None

        self._dimensions = 2
        self._namespace = ""

        """

        See FV.py

        """
        self._use_var_shortcut = False
        self._variable_names = []
        self._variable_pos = [0]

        if isinstance(unknowns, dict):
            self._unknowns = sum(unknowns.values())
            self._variable_names += list(unknowns.keys())
            for var in list(unknowns.values()):
                self._variable_pos.append(var + self._variable_pos[-1])
            self._use_var_shortcut = True
        elif type(unknowns) is int:
            self._unknowns = unknowns
        else:
            raise Exception(
                "Not a valid type for parameter unknowns, needs to be int or dictionary."
            )

        if isinstance(auxiliary_variables, dict):
            self._auxiliary_variables = sum(auxiliary_variables.values())
            self._variable_names += list(auxiliary_variables.keys())
            for var in list(auxiliary_variables.values()):
                self._variable_pos.append(var + self._variable_pos[-1])
            self._use_var_shortcut = True
        elif type(auxiliary_variables) is int:
            self._auxiliary_variables = auxiliary_variables
        else:
            raise Exception(
                "Not a valid type for parameter auxiliary_variables, needs to be int or dictionary."
            )

        self._plot_description = ", ".join(self._variable_names)

        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        if min_h > max_h:
            raise Exception(
                "min_h (" + str(min_h) + ") is bigger than max_h (" + str(max_h) + ")"
            )

        self._solver_template_file_class_name = None

        self._templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        self._action_set_initial_conditions = None
        self._action_set_initial_conditions_for_grid_construction = None
        self._action_set_AMR = None
        self._action_set_AMR_throughout_grid_construction = None
        self._action_set_AMR_commit_without_further_analysis = None
        self._action_set_handle_boundary = None
        self._action_set_project_patch_onto_faces = None
        self._action_set_roll_over_update_of_faces = None
        self._action_set_copy_new_faces_onto_old_faces = None
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = (
            None
        )
        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"
        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self._compute_kernel_call = "#error Not yet defined"
        self._fused_compute_kernel_call = "#error Not yet defined"

        self._can_fuse = True
        self._use_enclave_tasking = use_enclave_tasking
        self._number_of_enclave_tasks = number_of_enclave_tasks
        self._enclave_task_priority = "tarch::multicore::Task::DefaultPriority - 1"

        self._initial_conditions_implementation = PDETerms.User_Defined_Implementation
        self._boundary_conditions_implementation = PDETerms.User_Defined_Implementation
        self._refinement_criterion_implementation = PDETerms.Empty_Implementation
        self._max_eigenvalue_implementation = PDETerms.None_Implementation
        self._flux_implementation = PDETerms.None_Implementation
        self._source_term_implementation = PDETerms.None_Implementation
        self._solver_implementation = PDETerms.None_Implementation

        self._use_stateless_initial_conditions = False
        self._use_stateless_boundary_conditions = False
        self._use_stateless_refinement_criterion = False
        self._use_stateless_max_eigenvalue = False
        self._use_stateless_flux = False
        self._use_stateless_source_term = False
        self._use_stateless_solver = False

        self._compute_eigenvalue = True
        self._time_stepping_code_snippets = None
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._print_time_step_implementation = ""
        self._constructor_implementation = ""
        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

        self._interpolation = "piecewise_constant"
        self._restriction = "averaging"

        self._compute_precision = "double"
        self._storage_precision = "double"

        self._baseline_action_set_descend_invocation_order = (
            baseline_action_set_descend_invocation_order
        )

        self._cell_data_storage = Storage.SmartPointers
        self._face_data_storage = Storage.SmartPointers

        self.create_data_structures()
        self.create_action_sets()

    def __str__(self):
        result = (
            """
Name:                   """
            + self._name
            + """
Type:                   """
            + self.__class__.__module__
            + """
Dimensions:             """
            + str(self._dimensions)
            + """
Use enclave tasking:    """
            + str(self._use_enclave_tasking)
            + """
Can fuse:               """
            + str(self._can_fuse)
            + """
Patch size:             """
            + str(self._patch_size)
            + """
Order:                  """
            + str(self._order)
            + """
Unknowns:               """
            + str(self._unknowns)
            + """
Auxiliary variables:    """
            + str(self._auxiliary_variables)
            + """
min_h:                  """
            + str(self._min_h)
            + """
max_h:                  """
            + str(self._max_h)
            + """
Initial conditions:     """
            + str(self._initial_conditions_implementation)
            + """
Boundary conditions:    """
            + str(self._boundary_conditions_implementation)
            + """
Refinement criterion:   """
            + str(self._refinement_criterion_implementation)
            + """
Max eigenvalue:         """
            + str(self._max_eigenvalue_implementation)
            + """
Solver:                 """
            + str(self._solver_implementation)
            + """
"""
        )
        return result

    __repr__ = __str__

    def get_min_number_of_spacetree_levels(self, domain_size):
        coarsest_tree_level = 0
        while (
            domain_size * 3 ** (-coarsest_tree_level) / self._patch_size > self._max_h
        ):
            coarsest_tree_level += 1
        return coarsest_tree_level

    def get_max_number_of_spacetree_levels(self, domain_size):
        finest_tree_level = 0
        while domain_size * 3 ** (-finest_tree_level) / self._patch_size > self._min_h:
            finest_tree_level += 1
        return finest_tree_level

    def get_coarsest_number_of_patches(self, domain_size):
        return 3 ** self.get_min_number_of_spacetree_levels(domain_size)

    def get_finest_number_of_patches(self, domain_size):
        return 3 ** self.get_max_number_of_spacetree_levels(domain_size)

    def get_coarsest_number_of_nodes(self, domain_size):
        return self.get_coarsest_number_of_patches(domain_size) * self._patch_size

    def get_finest_number_of_nodes(self, domain_size):
        return self.get_finest_number_of_patches(domain_size) * self._patch_size

    def get_coarsest_H(self, domain_size):
        return domain_size / self.get_coarsest_number_of_nodes(domain_size)

    def get_finest_H(self, domain_size):
        return domain_size / self.get_finest_number_of_nodes(domain_size)

    def create_readme_descriptor(self, domain_offset, domain_size):
        return (
            """
### ExaHyPE 2 solver

"""
            + str(self)
            + """

Real type of this solver: """
            + str(type(self))
            + """

We assume that you use a domain size of (0,"""
            + str(domain_size)
            + """)^d. Peano 4 will cut this domain equidistantly
and recursively into three parts along each coordinate axis. This yields a spacetree.

The spacetree will at least have """
            + str(self.get_min_number_of_spacetree_levels(domain_size))
            + """ levels.
The spacetree will at most have """
            + str(self.get_max_number_of_spacetree_levels(domain_size))
            + """ levels.

The spacetree will thus span at least """
            + str(self.get_coarsest_number_of_patches(domain_size))
            + """ cells (or patches) per coordinate axis. This means a """
            + str(self.get_coarsest_number_of_patches(domain_size))
            + """^d grid of patches.
The spacetree will thus span at most """
            + str(self.get_finest_number_of_patches(domain_size))
            + """ cells (or patches) per coordinate axis. This means a """
            + str(self.get_finest_number_of_patches(domain_size))
            + """^d grid of patches.

ExaHyPE 2 embeds """
            + str(self._patch_size)
            + """^d patches of nodes into the finest tree level.

The coarsest possible mesh will consist of """
            + str(self.get_coarsest_number_of_nodes(domain_size))
            + """ nodes per coordinate axis.
The finest possible mesh will consist of """
            + str(self.get_finest_number_of_nodes(domain_size))
            + """ nodes per coordinate axis.

The coarsest mesh width of """
            + str(self.get_coarsest_H(domain_size))
            + """ is thus just smaller than the maximum internodal distance """
            + str(self._max_h)
            + """.
The finest mesh width of """
            + str(self.get_finest_H(domain_size))
            + """ is thus just smaller than the minimum internodal distance """
            + str(self._min_h)
            + """.
"""
        )

    @property
    def user_action_set_includes(self):
        """
        Add further includes to this property, if your action sets require some additional
        routines from other header files.
        """
        return self._user_action_set_includes

    @property
    def user_solver_includes(self):
        """
        Add further includes to this property, if your solver requires some additional
        routines from other header files.
        """
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        """
        Add further includes to this property, if your action sets require some additional
        routines from other header files.
        """
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        """
        Add further includes to this property, if your solver requires some additional
        routines from other header files.
        """
        self._user_solver_includes += value

    @abstractmethod
    def create_data_structures(self):
        """!

        See FV.py

        """

        self._patch = peano4.datamodel.Patch(
            (self._patch_size, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier(),
            self._storage_precision,
        )
        self._patch_reconstructed = peano4.datamodel.Patch(
            (
                self._patch_size + 2 * self._order,
                self._patch_size + 2 * self._order,
                self._patch_size + 2 * self._order,
            ),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Reconstructed",
            self._storage_precision,
        )
        self._patch_overlap_old = peano4.datamodel.Patch(
            (2 * self._order, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Old",
            self._storage_precision,
        )
        self._patch_overlap_new = peano4.datamodel.Patch(
            (2 * self._order, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "New",
            self._storage_precision,
        )
        self._patch_overlap_update = peano4.datamodel.Patch(
            (2 * self._order, self._patch_size, self._patch_size),
            self._unknowns + self._auxiliary_variables,
            self._unknown_identifier() + "Update",
            self._storage_precision,
        )

        if self._cell_data_storage == Storage.CallStack:
            self._patch.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch, self._storage_precision
            )
            self._patch_reconstructed.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_reconstructed, self._storage_precision
            )
        elif self._cell_data_storage == Storage.Heap:
            self._patch.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(
                self._patch, self._storage_precision
            )
            self._patch_reconstructed.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_reconstructed, self._storage_precision
                )
            )
        elif self._cell_data_storage == Storage.SmartPointers:
            self._patch.generator = peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                self._patch, self._storage_precision
            )
            self._patch_reconstructed.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_reconstructed, self._storage_precision
                )
            )
        else:
            assert False, "Storage variant {} not supported".format(
                self._cell_data_storage
            )

        if self._face_data_storage == Storage.CallStack:
            self._patch_overlap_old.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_old, self._storage_precision
            )
            self._patch_overlap_new.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_new, self._storage_precision
            )
            self._patch_overlap_update.generator = peano4.datamodel.PatchToDoubleArray(
                self._patch_overlap_update, self._storage_precision
            )
        elif self._face_data_storage == Storage.Heap:
            self._patch_overlap_old.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_old, self._storage_precision
                )
            )
            self._patch_overlap_new.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_new, self._storage_precision
                )
            )
            self._patch_overlap_update.generator = (
                peano4.datamodel.PatchToDoubleArrayOnHeap(
                    self._patch_overlap_update, self._storage_precision
                )
            )
        elif self._face_data_storage == Storage.SmartPointers:
            self._patch_overlap_old.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_old, self._storage_precision
                )
            )
            self._patch_overlap_new.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_new, self._storage_precision
                )
            )
            self._patch_overlap_update.generator = (
                peano4.datamodel.PatchToDoubleArrayWithSmartPointer(
                    self._patch_overlap_update, self._storage_precision
                )
            )
        else:
            assert False, "Storage variant {} not supported".format(
                self._face_data_storage
            )

        self._patch_overlap_old.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_face_merge_implementation(
                self._patch_overlap_old
            )
        )
        self._patch_overlap_new.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_face_merge_implementation(
                self._patch_overlap_new
            )
        )
        self._patch.generator.merge_method_definition = (
            peano4.toolbox.blockstructured.get_cell_merge_implementation(self._patch)
        )

        self._patch_overlap_old.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._patch.generator.load_store_compute_flag = (
            "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
                self._provide_cell_data_to_compute_kernels_default_guard(),
                self._load_cell_data_default_guard(),
                self._store_cell_data_default_guard(),
            )
        )

        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"

        self._patch_overlap_new.generator.send_condition = "false"
        self._patch_overlap_new.generator.receive_and_merge_condition = "false"

        self._patch_overlap_update.generator.send_condition = "false"
        self._patch_overlap_update.generator.receive_and_merge_condition = "false"

        self._patch_overlap_old.generator.load_store_compute_flag = (
            "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
                self._provide_face_data_to_compute_kernels_default_guard(),
                self._load_face_data_default_guard(),
                self._store_face_data_default_guard(),
            )
        )

        self._patch_overlap_new.generator.load_store_compute_flag = (
            "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
                self._provide_face_data_to_compute_kernels_default_guard(),
                self._load_face_data_default_guard(),
                self._store_face_data_default_guard(),
            )
        )

        self._patch_overlap_update.generator.load_store_compute_flag = (
            "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
                self._provide_face_data_to_compute_kernels_default_guard(),
                "false",
                "false",
            )
        )

        self._patch.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_old.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "../repositories/SolverRepository.h"
"""
        self._patch_overlap_update.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    @abstractmethod
    def create_action_sets(self):
        """!

        See FV.py

        """
        self._action_set_initial_conditions = (
            exahype2.solvers.fd.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard(), "true"
            )
        )
        self._action_set_initial_conditions_for_grid_construction = (
            exahype2.solvers.fd.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard(), "false"
            )
        )
        self._action_set_AMR_throughout_grid_construction = (
            exahype2.solvers.fd.actionsets.AdaptivityCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
            )
        )
        self._action_set_AMR_commit_without_further_analysis = (
            exahype2.solvers.fd.actionsets.AdaptivityCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=False,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
            )
        )
        self._action_set_AMR = exahype2.solvers.fd.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=False,
        )
        self._action_set_handle_boundary = (
            exahype2.solvers.fd.actionsets.HandleBoundary(
                self, self._store_face_data_default_guard()
            )
        )
        self._action_set_project_patch_onto_faces = (
            exahype2.solvers.fd.actionsets.ProjectPatchOntoFaces(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_roll_over_update_of_faces = (
            exahype2.solvers.fd.actionsets.RollOverUpdatedFace(
                self, self._store_face_data_default_guard()
            )
        )
        self._action_set_copy_new_faces_onto_old_faces = (
            peano4.toolbox.blockstructured.BackupPatchOverlap(
                self._patch_overlap_new,
                self._patch_overlap_old,
                False,
                self._store_face_data_default_guard(),
                self._get_default_includes(),
            )
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fd.actionsets.DynamicAMR(
            solver=self,
            interpolation=self._interpolation,
            restriction=self._restriction,
        )

        # This one is different: it is the hook-in point for other solvers, so we create it
        # only once if it does not exist.
        if self._action_set_postprocess_solution == None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.fd.actionsets.EmptyPostprocessSolution(self)
            )
        if self._action_set_preprocess_solution == None:
            self._action_set_preprocess_solution = (
                exahype2.solvers.fd.actionsets.EmptyPreprocessSolution(self)
            )

        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)

        self._action_set_update_patch = None
        self._action_set_reconstruct_patch = None
        self._action_set_enclave_tasking = None

        self._action_set_initial_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_initial_conditions_for_grid_construction.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_AMR.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_AMR_commit_without_further_analysis.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_handle_boundary.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_project_patch_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )  # touch cell last time, i.e. leave cell
        self._action_set_roll_over_update_of_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_copy_new_faces_onto_old_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_postprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )  # touch cell last time, i.e. leave cell
        self._action_set_preprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        """!

        See FV.py

        """
        return "not marker.willBeRefined() and repositories::{}.getSolverState()!={}::SolverState::GridConstruction".format(
            self.get_name_of_global_instance(), self._name
        )

    def _provide_face_data_to_compute_kernels_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _store_cell_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_cell_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.hasBeenRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _store_face_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _load_face_data_default_guard(self):
        """!
        Extend the guard via ands only. Never use an or, as subclasses might
        extend it as well, and they will append further ends.
        """
        return (
            "not marker.hasBeenRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _unknown_identifier(self):
        return self._name + "Q"

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def add_to_Peano4_datamodel(self, datamodel):
        """
        Add all required data to the Peano project's datamodel
        so it is properly built up.
        """
        log.debug("Patch data")
        log.debug("----------")
        log.debug(str(self._patch))
        log.debug(str(self._patch_reconstructed))
        log.debug("----------")
        log.debug("Patch overlap data")
        log.debug(str(self._patch_overlap_old))
        log.debug(str(self._patch_overlap_new))
        log.debug("----------")
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._patch)
        datamodel.add_cell(self._patch_reconstructed)
        datamodel.add_face(self._patch_overlap_old)
        datamodel.add_face(self._patch_overlap_new)
        datamodel.add_face(self._patch_overlap_update)
        datamodel.add_face(self._face_label)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        """
        Tell Peano what data to move around

        Inform Peano4 step which data are to be moved around via the
        use_cell and use_face commands. This operation is generic from
        ExaHyPE's point of view, i.e. I use it for all grid sweep types.

        It is important that the cell label comes first, as some other
        data might make their load/store decisions depend on the label.
        """
        step.use_cell(self._patch)
        step.use_cell(self._patch_reconstructed)
        step.use_cell(self._cell_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)
        step.use_face(self._patch_overlap_update)
        step.use_face(self._face_label)

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_actions_to_init_grid(self, step, restart_from_checkpoint=False):
        """!

        See FV.py

        """
        assert (
            self._action_set_copy_new_faces_onto_old_faces.descend_invocation_order
            < self._action_set_roll_over_update_of_faces.descend_invocation_order
        )

        if restart_from_checkpoint:
            self._action_set_initial_conditions = (
                exahype2.solvers.fd.actionsets.InitialCondition(
                    self,
                    self._store_cell_data_default_guard(),
                    "true",
                    restart_from_checkpoint=True,
                )
            )

        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_copy_new_faces_onto_old_faces)
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        """
        The boundary information is set only once. It is therefore important that
        we ues the face label and initialise it properly.
        """
        step.add_action_set(self._action_set_initial_conditions_for_grid_construction)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        if evaluate_refinement_criterion:
            step.add_action_set(self._action_set_AMR_throughout_grid_construction)
        else:
            step.add_action_set(self._action_set_AMR_commit_without_further_analysis)

    @property
    def plot_description(
        self,
    ):
        return self._plot_description

    @plot_description.setter
    def plot_description(self, description):
        """!
        Add a proper description to the plots

        Use this one to set a description within the output patch file that tells
        the solver what the semantics of the entries are. Typically, I use
        a comma-separated list here.
        """
        self._plot_description = description

    def set_plotter(self, plotter_option):
        self._plotter_option = plotter_option

    def add_actions_to_plot_solution(
        self, step, output_path, restart_from_checkpoint=False
    ):
        """!
        Add action sets to plot solution step

        Consult the discussion in add_actions_to_init_grid() around the order
        of the individual action sets.

        It is important that we have the coupling/dynamic AMR part in here, as
        there might be pending AMR refinement requests that now are realised.
        For the same reason, we need the update of the face label and the update
        of the cell label in here: The AMR might just propagate over into the
        plotting, i.e. we might create new grid entities throughout the plot.
        These entities (faces and cells) have to be initialised properly.
        Otherwise, their un-initialised data will propagate through to the next
        time step.

        To make the restriction work, we have to project the solutions onto the
        faces.
        """
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_project_patch_onto_faces)

        if self._plotter_option == "HDF5":
            plot_patches_action_set = peano4.toolbox.blockstructured.PlotPatchesInHDF5Format(
                filename=output_path + "solution-" + self._name,
                patch=self._patch,
                dataset_name=self._unknown_identifier(),
                description=self._plot_description,
                time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                guard="repositories::plotFilter.plotPatch(marker) and "
                + self._load_cell_data_default_guard(),
                additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
                precision="PlotterPrecision",
                dataType=self._storage_precision,
                select_dofs=self._select_dofs_to_print,
                # restart_preprocess=restart_from_checkpoint
            )
        elif self._plotter_option == "PeanoPatch":
            plot_patches_action_set = peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat(
                filename=output_path + "solution-" + self._name,
                patch=self._patch,
                dataset_name=self._unknown_identifier(),
                description=self._plot_description,
                guard="repositories::plotFilter.plotPatch(marker) and "
                + self._load_cell_data_default_guard(),
                additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
                precision="PlotterPrecision",
                dataType=self._storage_precision,
                time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                select_dofs=self._select_dofs_to_print,
            )

        plot_patches_action_set.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        step.add_action_set(plot_patches_action_set)

        if self._plot_grid_properties:
            plot_grid_properties_action_set = peano4.toolbox.PlotGridInPeanoBlockFormat(
                filename=output_path + "grid-" + self._name,
                cell_unknown=None,
                guard="repositories::plotFilter.plotPatch(marker) and "
                + self._load_cell_data_default_guard(),
                additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
            )
            plot_grid_properties_action_set.descend_invocation_order = (
                self._baseline_action_set_descend_invocation_order
            )
            step.add_action_set(plot_grid_properties_action_set)
        pass

    def add_actions_to_checkpoint_solution(
        self, step, output_path, restart_from_checkpoint=False
    ):
        """!
        Add action sets to plot solution step

        Consult the discussion in add_actions_to_init_grid() around the order
        of the individual action sets.

        It is important that we have the coupling/dynamic AMR part in here, as
        there might be pending AMR refinement requests that now are realised.
        For the same reason, we need the update of the face label and the update
        of the cell label in here: The AMR might just propagate over into the
        plotting, i.e. we might create new grid entities throughout the plot.
        These entities (faces and cells) have to be initialised properly.
        Otherwise, their un-initialised data will propagate through to the next
        time step.

        To make the restriction work, we have to project the solutions onto the
        faces.
        """
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_project_patch_onto_faces)

        plot_patches_action_set = peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat(
            filename=output_path + "checkpoint-" + self._name,
            patch=self._patch,
            dataset_name=self._unknown_identifier(),
            description=self._plot_description,
            guard=self._load_cell_data_default_guard(),
            additional_includes="""
#include "../repositories/SolverRepository.h"
""",
            precision="PlotterPrecision",
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            select_dofs=None,
            restart_preprocess=restart_from_checkpoint,
        )
        plot_patches_action_set.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        step.add_action_set(plot_patches_action_set)

        if self._plot_grid_properties:
            plot_grid_properties_action_set = peano4.toolbox.PlotGridInPeanoBlockFormat(
                filename=output_path + "grid-" + self._name,
                cell_unknown=None,
                guard=self._load_cell_data_default_guard(),
                additional_includes="""
#include "../repositories/SolverRepository.h"
""",
            )
            plot_grid_properties_action_set.descend_invocation_order = (
                self._baseline_action_set_descend_invocation_order
            )
            step.add_action_set(plot_grid_properties_action_set)
        pass

    def add_actions_to_perform_time_step(self, step):
        """!
        Add action sets to time step

        See exahype2.solvers.fd.FV.add_actions_to_init_grid() for details on the
        ordering of the action sets.

        ## AMR

        It is important that we do the inter-grid transfer operators before we
        apply the boundary conditions.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(
            self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement
        )
        step.add_action_set(self._action_set_roll_over_update_of_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_reconstruct_patch)
        step.add_action_set(self._action_set_update_patch)
        if self._use_enclave_tasking:
            step.add_action_set(self._action_set_enclave_tasking)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(
        self, namespace, output, dimensions, subdirectory="", abstract_overwrite=True
    ):
        self._dimensions = dimensions
        self._namespace = namespace

        self._generate_kernels(output, subdirectory)
        self._action_set_reconstruct_patch.generate_kernels(
            namespace, output, dimensions, subdirectory
        )
        self._action_set_project_patch_onto_faces.generate_kernels(
            namespace, output, dimensions, subdirectory
        )
        if self._use_enclave_tasking and self._can_fuse:
            self._action_set_reconstruct_patch.generate_fused_kernels(
                namespace, output, dimensions, subdirectory
            )

        if self._solver_template_file_class_name is None:
            self._templatefile_prefix += self.__class__.__name__
        else:
            self._templatefile_prefix += self._solver_template_file_class_name

        if subdirectory:
            subdirectory += "/"

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        header_file_abstr_template = self._templatefile_prefix + "Abstract.template.h"
        cpp_file_abstr_template = self._templatefile_prefix + "Abstract.template.cpp"
        header_file_template = self._templatefile_prefix + ".template.h"
        cpp_file_template = self._templatefile_prefix + ".template.cpp"

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_abstr_template,
                cpp_file_abstr_template,
                "Abstract" + self._name,
                namespace,
                subdirectory + ".",
                d,
                abstract_overwrite,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_template,
                cpp_file_template,
                self._name,
                namespace,
                subdirectory + ".",
                d,
                False,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file(
            subdirectory + "Abstract" + self._name + ".h", generated=True
        )
        output.makefile.add_h_file(subdirectory + self._name + ".h", generated=True)
        output.makefile.add_cpp_file(
            subdirectory + "Abstract" + self._name + ".cpp", generated=True
        )
        output.makefile.add_cpp_file(subdirectory + self._name + ".cpp", generated=True)

        if self._use_var_shortcut:
            generated_shortcut_file = peano4.output.Jinja2TemplatedHeaderFile(
                os.path.dirname(os.path.realpath(__file__))
                + "/"
                + "../../VariableShortcuts.template.h",
                "VariableShortcuts" + self._name,
                namespace,
                subdirectory + ".",
                d,
                True,
                True,
            )
            output.add(generated_shortcut_file)
            output.makefile.add_h_file(
                subdirectory + "VariableShortcuts" + self._name + ".h", generated=True
            )

        if self._use_enclave_tasking:
            generated_enclave_task = (
                peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                    headfile_template=os.path.dirname(os.path.realpath(__file__))
                    + "/"
                    + "EnclaveTask.template.h",
                    cppfile_template=os.path.dirname(os.path.realpath(__file__))
                    + "/"
                    + "EnclaveTask.template.cpp",
                    classname="{}EnclaveTask".format(self._name),
                    namespace=namespace + ["tasks"],
                    subdirectory=subdirectory + "tasks",
                    dictionary=d,
                    default_overwrite=True,
                    apply_iteratively=True,
                )
            )
            output.add(generated_enclave_task)
            output.makefile.add_h_file(
                subdirectory + "tasks/" + "{}EnclaveTask".format(self._name) + ".h",
                generated=True,
            )
            output.makefile.add_cpp_file(
                subdirectory + "tasks/" + "{}EnclaveTask".format(self._name) + ".cpp",
                generated=True,
            )

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring + "\n"

    def set_solver_precisions(
        self, storage_precision="double", compute_precision="double"
    ):
        self._storage_precision = storage_precision
        self._compute_precision = compute_precision
        self.set_implementation()
        self.create_data_structures()

    def _init_dictionary_with_default_parameters(self, d):
        """
        This one is called by all algorithmic steps before I invoke
        add_entries_to_text_replacement_dictionary().

        See the remarks on set_postprocess_updated_patch_kernel to understand why
        we have to apply the (partially befilled) dictionary to create a new entry
        for this very dictionary.
        """
        d["DIMENSIONS"] = self._dimensions
        d["NUMBER_OF_NODES_PER_AXIS"] = self._patch.dim[0]
        d["ORDER"] = self._order
        d["HALO_SIZE"] = int(self._patch_overlap_old.dim[0] / 2)
        d["NUMBER_OF_NODES_PER_AXIS_WITH_HALO"] = (
            d["NUMBER_OF_NODES_PER_AXIS"] + 2 * d["HALO_SIZE"]
        )
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["NAMESPACE"] = self._namespace
        d["SOLVER_NAME"] = self._name
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._auxiliary_variables
        d["NUMBER_OF_VARIABLES_AT_NODE"] = self._unknowns + self._auxiliary_variables

        d["COMPUTE_PRECISION"] = self._compute_precision
        d["STORAGE_PRECISION"] = self._storage_precision

        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PER_AXIS"] = (
            d["NUMBER_OF_NODES_PER_AXIS"] * d["NUMBER_OF_VARIABLES_AT_NODE"]
        )
        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_2D"] = (
            d["NUMBER_OF_NODES_PER_AXIS"]
            * d["NUMBER_OF_NODES_PER_AXIS"]
            * d["NUMBER_OF_VARIABLES_AT_NODE"]
        )
        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_3D"] = (
            d["NUMBER_OF_NODES_PER_AXIS"]
            * d["NUMBER_OF_NODES_PER_AXIS"]
            * d["NUMBER_OF_NODES_PER_AXIS"]
            * d["NUMBER_OF_VARIABLES_AT_NODE"]
        )

        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_PER_AXIS"] = (
            d["NUMBER_OF_NODES_PER_AXIS_WITH_HALO"] * d["NUMBER_OF_VARIABLES_AT_NODE"]
        )
        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_2D"] = (
            d["NUMBER_OF_NODES_PER_AXIS_WITH_HALO"] ** 2
        ) * d["NUMBER_OF_VARIABLES_AT_NODE"]
        d["NUMBER_OF_DOUBLE_VALUES_IN_PATCH_PLUS_HALO_3D"] = (
            d["NUMBER_OF_NODES_PER_AXIS_WITH_HALO"] ** 3
        ) * d["NUMBER_OF_VARIABLES_AT_NODE"]

        d["FACE_CARDINALITY_2D"] = int(
            self._patch_overlap_new.no_of_unknowns
            * self._patch_overlap_new.dim[0]
            * self._patch_overlap_new.dim[1]
        )
        d["FACE_CARDINALITY_3D"] = int(
            self._patch_overlap_new.no_of_unknowns
            * self._patch_overlap_new.dim[0]
            * self._patch_overlap_new.dim[1]
            * self._patch_overlap_new.dim[2]
        )

        if self._min_h > self._max_h:
            raise Exception("min/max h are inconsistent")

        d["MAX_H"] = self._max_h
        d["MIN_H"] = self._min_h
        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"] = self.user_solver_includes

        d["SOLVERS_BEFORE_IN_COUPLING"] = [
            (solver.get_name_of_global_instance(), solver.name)
            for solver in self._solvers_before_in_coupling
        ]
        if self._last_solver is None:
            self._last_solver = self
        d["LAST_SOLVER"] = (
            self._last_solver.get_name_of_global_instance(),
            self._last_solver.name,
        )

        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d[
            "BOUNDARY_CONDITIONS_IMPLEMENTATION"
        ] = self._boundary_conditions_implementation
        d[
            "REFINEMENT_CRITERION_IMPLEMENTATION"
        ] = self._refinement_criterion_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["SOLVER_IMPLEMENTATION"] = self._solver_implementation

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        d["STATELESS_MAX_EIGENVALUE"] = self._use_stateless_max_eigenvalue
        d["STATELESS_FLUX"] = self._use_stateless_flux
        d["STATELESS_SOURCE_TERM"] = self._use_stateless_source_term
        d["STATELESS_SOLVER"] = self._use_stateless_solver

        d["ABSTRACT_SOLVER_USER_DECLARATIONS"] = jinja2.Template(
            self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["ABSTRACT_SOLVER_USER_DEFINITIONS"] = jinja2.Template(
            self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["SOLVER_USER_DECLARATIONS"] = jinja2.Template(
            self._solver_user_declarations, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["SOLVER_USER_DEFINITIONS"] = jinja2.Template(
            self._solver_user_definitions, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["START_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._start_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._finish_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["PRINT_TIME_STEP_IMPLEMENTATION"] = jinja2.Template(
            self._print_time_step_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["START_SIMULATION_IMPLEMENTATION"] = jinja2.Template(
            self._start_simulation_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FINISH_SIMULATION_IMPLEMENTATION"] = jinja2.Template(
            self._finish_simulation_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["PREPROCESS_RECONSTRUCTED_PATCH"] = jinja2.Template(
            self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["POSTPROCESS_UPDATED_PATCH"] = jinja2.Template(
            self._postprocess_updated_patch, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["COMPUTE_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_new_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_MAX_EIGENVALUE"] = self._compute_eigenvalue

        d["CAN_FUSE"] = self._can_fuse
        d["USE_ENCLAVE_TASKING"] = self._use_enclave_tasking
        d["NUMBER_OF_ENCLAVE_TASKS"] = self._number_of_enclave_tasks
        d["ENCLAVE_TASK_PRIORITY"] = self._enclave_task_priority
        d["ENCLAVE_TASK_TYPE"] = f"{self._name}EnclaveTask"

        d["COMPUTE_KERNEL_CALL"] = jinja2.Template(
            self._compute_kernel_call, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["FUSED_COMPUTE_KERNEL_CALL"] = jinja2.Template(
            self._fused_compute_kernel_call, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["USE_VARIABLE_SHORTCUT"] = self._use_var_shortcut
        d["VARIABLE_NAMES"] = self._variable_names
        d["VARIABLE_POSITIONS"] = self._variable_pos

    @property
    def unknowns(self):
        return self._unknowns

    @property
    def patch_size(self):
        return self._patch_size

    @property
    def last_solver(self):
        return self._last_solver

    @last_solver.setter
    def last_solver(self, solver):
        self._last_solver = solver

    @property
    def auxiliary_variables(self):
        return self._auxiliary_variables

    @patch_size.setter
    def patch_size(self, value):
        self._patch_size = value
        self.create_data_structures()
        self.create_action_sets()

    @unknowns.setter
    def unknowns(self, value):
        self._unknowns = value
        self.create_data_structures()
        self.create_action_sets()

    @auxiliary_variables.setter
    def auxiliary_variables(self, value):
        self._auxiliary_variables = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def preprocess_reconstructed_patch(self):
        return self._preprocess_reconstructed_patch

    @preprocess_reconstructed_patch.setter
    def preprocess_reconstructed_patch(self, kernel):
        """!
        Set a new processing kernel

        Before the actual update, the numerical solver takes the actual solution
        including all auxiliary variables and copies it into another array which
        is calls reconstructedPatch (in FD4, this field is called oldQWithHalo).
        This array is bigger than the actual patch data, as it also holds the
        halo. Once the solution is copied over, the kernel writes in the halo
        data using data from the faces. This reconstructed object is now passed
        into the compute kernel, which derives new values for all evolution
        variables. That is, the compute kernels' output does not hold the auxiliary
        variables anymore, as they are not subject to the PDE. The new variables
        are eventually written back into the real data, where they replace the
        old quantities (the auxiliary variables in there remain untouched).
        After the compute kernel has terminated, the reconstructed data including
        the halo is thrown away.

        ## Auxiliary variables

        If you alter the auxiliary variables (material parameters) in the patch
        preparation, this is absolutely fine, but the change will not be committed
        into the real data. It is only there temporarily while the actual patch
        will continue to hold the original material parameters.

        You can alter the actual patch data Q in the preprocessing. If this
        actual data are evolution (PDE) parameters, these changes however will
        be overwritten by the compute kernel. If you alter auxiliary variables
        within the preprocessing within the patch (Q), then this is fine, but the
        changes will not be visible to the reconstructed patch immediately. They
        will be visible there in the next time step. So you might be well-advised
        to alter the auxiliary variables in both the patch data and the
        reconstructed data.

        ## Access

        We recommend that you use the AoS enumerator exahype2::enumerator::AoSLexicographicEnumerator.
        This one has to be given 1 as number of cells/patches, it requires the size
        of the patch, its halo size, you have to pass it the unknowns and the correct
        number of auxiliary parameters. If you want the preprocessing also to alter
        the actual input, you need a second enumerator. In this second one, all
        parameters are the same besides the halo size, which you have to set to 0:
        The input patch has no halo. This halo is only added temporarily in the
        reconstruction step.
        """
        self._preprocess_reconstructed_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def name(self):
        return self._name

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """

        See FV.py

        """
        self._postprocess_updated_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def order(self):
        return self._order

    @order.setter
    def order(self, value):
        if value < 1:
            raise Exception(
                "Halo (order) size has to be bigger than zero but was {}".format(value)
            )
        self._order = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def interpolation(self):
        return self._interpolation

    @interpolation.setter
    def interpolation(self, value):
        self._interpolation = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def restriction(self):
        return self._restriction

    @restriction.setter
    def restriction(self, value):
        self._restriction = value
        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def _generate_kernels(self, namespace, output, subdirectory, dimensions):
        pass
