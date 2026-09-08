# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from abc import abstractmethod

import jinja2

import peano4.output
import peano4.datamodel
import peano4.plotter
import exahype2.blockstructured

import exahype2.solvers.fv.actionsets

from exahype2.solvers.fv.PatchData import create_patch_and_halo
from exahype2.solvers import PDETerms
from exahype2.VariableShortcuts import VariableShortcuts
from peano4.output.Helper import render_jinja2_template

from peano4.Logger import log


class FV(object):
    def __init__(
        self,
        name,
        patch_size,
        overlap,
        unknowns,
        auxiliary_variables,
        baseline_action_set_descend_invocation_order=0,
    ):
        """!
        name identifies the solver in every generated class. patch_size counts the
        finite volumes per axis in one cell, and overlap counts the halo layers per
        face, at least one and at most patch_size.
        """
        self._name = name
        self._solver_number = 0

        self._patch_size = patch_size
        self._overlap = overlap

        self._dimensions = 2
        self._namespace = ""

        self._plotter = peano4.plotter.PatchPlotter
        self._variable_shortcuts = VariableShortcuts(name, unknowns, auxiliary_variables)

        self._particle_unknowns = []
        self._particle_variable_names = []
        self._particle_variable_pos = []
        self._use_var_shortcut_tracer = False
        self._tracer_names = []
        self._tracer_particles = []
        self._tracer_particle_sets = []
        self._sort_descend_invocation_order_within_action_sets = -1
        self._plot_descend_invocation_order_within_action_sets = 65536
        self._update_particle_association_action_sets = []
        self._update_particle_parallel_state_action_sets = []
        self._particle_attributes = []
        self._particle_number = []
        self._particle_plotters = []

        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        self._solver_template_file_class_name = None
        self._templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        self._action_set_initial_conditions = None
        self._action_set_AMR = None
        self._action_set_AMR_throughout_grid_construction = None
        self._action_set_handle_boundary = None
        self._action_set_project_patch_onto_faces = None
        self._action_set_roll_over_updates = None
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = None
        self._action_set_dynamic_amr_transfer = None
        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"
        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self._compute_kernel_call = "#error Not yet defined"

        self._initial_conditions_implementation = PDETerms.User_Defined_Implementation
        self._boundary_conditions_implementation = PDETerms.User_Defined_Implementation
        self._refinement_criterion_implementation = PDETerms.Empty_Implementation
        self._flux_implementation = PDETerms.None_Implementation
        self._ncp_implementation = PDETerms.None_Implementation
        # Can be none with fixed time stepping, but has to be defined for adaptive time stepping.
        self._max_eigenvalue_implementation = PDETerms.None_Implementation
        self._riemann_solver_implementation = PDETerms.None_Implementation
        self._source_term_implementation = PDETerms.None_Implementation
        self._diffusive_source_term_implementation = PDETerms.None_Implementation
        self._limiter_implementation = PDETerms.None_Implementation

        self._use_stateless_initial_conditions = False
        self._use_stateless_boundary_conditions = False
        # Refinement criteria may query mesh-size information from the solver.
        self._use_stateless_refinement_criterion = False
        self._use_stateless_flux = False
        self._use_stateless_ncp = False
        self._use_stateless_max_eigenvalue = False
        self._use_stateless_riemann_solver = False
        self._use_stateless_source_term = False
        self._use_stateless_diffusive_source_term = False
        self._use_stateless_limiter = False

        self._compute_eigenvalue = True
        self._time_stepping_code_snippets = None
        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""
        self._constructor_implementation = ""
        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

        self._compute_precision = "double"
        self._storage_precision = "double"

        self._baseline_action_set_descend_invocation_order = baseline_action_set_descend_invocation_order

        self.create_data_structures()
        self.create_action_sets()

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

We use a domain with offset ("""
            + str(domain_offset)
            + ") and size ("
            + str(domain_size)
            + """). Peano 4 cuts this domain equidistantly
and recursively into three parts along each coordinate axis. This yields a spacetree.

ExaHyPE 2 embeds """
            + str(self._patch_size)
            + """^d Finite Volumes into each patch. The number of Peano grid levels
and additional AMR levels is configured by the executable at runtime.
"""
        )

    @property
    def user_action_set_includes(self):
        """
        Additional includes that the generated action sets require.
        """
        return self._user_action_set_includes

    @property
    def user_solver_includes(self):
        """
        Additional includes that the generated solver requires.
        """
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        self._user_solver_includes += value

    @abstractmethod
    def create_data_structures(self):
        """!
        Only the last call matters, so a subclass calls this version first and
        then alters the data it needs. _patch holds the current snapshot,
        _patch_old the one the preceding time step produced, and each face holds
        the halo layers of both adjacent patches in an old and a new snapshot.
        """

        (
            self._patch,
            self._patch_old,
            self._patch_overlap_old,
            self._patch_overlap_new,
        ) = create_patch_and_halo(
            self.current_unknown_identifier(),
            self.previous_unknown_identifier(),
            self._patch_size,
            self._overlap,
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._storage_precision,
        )

        self._patch_overlap_old.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._patch_overlap_new.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        # The old halo is filled locally, so only the new halo is exchanged. The
        # element-wise merge writes the halo layers the sender owns, so the result
        # does not depend on the order in which the receives arrive.
        self._patch_overlap_old.generator.send_condition = "false"
        self._patch_overlap_old.generator.receive_and_merge_condition = "false"
        self._patch_overlap_old.generator.can_receive = "false"
        self._patch_overlap_old.generator.can_send = "false"
        self._patch_overlap_old.generator.use_copy_for_exchange = "true"
        self._patch_overlap_old.generator.merge_method_definition = (
            exahype2.blockstructured.get_face_merge_implementation(
                self._patch_overlap_old
            )
        )

        self._patch_overlap_new.generator.send_condition = "true"
        self._patch_overlap_new.generator.receive_and_merge_condition = "true"
        self._patch_overlap_new.generator.can_receive = self._load_face_data_default_guard()
        self._patch_overlap_new.generator.can_send = self._store_face_data_default_guard()
        self._patch_overlap_new.generator.use_copy_for_exchange = "true"
        self._patch_overlap_new.generator.merge_method_definition = (
            exahype2.blockstructured.get_face_merge_implementation(
                self._patch_overlap_new
            )
        )

        self._patch.generator.includes += """
#include "repositories/SolverRepository.h"
"""
        self._patch_old.generator.includes += """
#include "repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    @abstractmethod
    def create_action_sets(self):
        """!
        Only the last call matters, so a subclass calls this version first and
        then alters the action sets it needs. The preprocessing and
        postprocessing action sets survive the call and keep what a user set on
        them.
        """
        self._action_set_initial_conditions = (
            exahype2.solvers.fv.actionsets.InitialCondition(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_AMR_throughout_grid_construction = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=True,
            skip_after_topology_change=False,
        )
        self._action_set_AMR = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            # A time step never realises topology changes; it only commits the
            # refine/coarsen requests. Realisation happens in the DynamicAMR
            # observer, whose DynamicAMRTransfer delivers the committed events.
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=True,
        )
        # Only cells that the realise sweeps have just created evaluate the
        # criterion. Every other cell committed its request in the preceding time
        # step, and re-issuing it each sweep would keep the sweeps running forever.
        self._action_set_AMR_throughout_dynamic_amr = exahype2.solvers.fv.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard()
            + " and fineGridCell"
            + self._cell_label.name
            + ".getRequiresCriterionEvaluation()",
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=False,
        )
        self._action_set_handle_boundary = (
            exahype2.solvers.fv.actionsets.HandleBoundary(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_project_patch_onto_faces = (
            exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces(
                self, self._store_cell_data_default_guard()
            )
        )
        self._action_set_roll_over_updates = (
            exahype2.solvers.fv.actionsets.RollOverUpdates(
                self, self._store_face_data_default_guard(), self._store_cell_data_default_guard()
            )
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fv.actionsets.AdaptiveMeshFaceCoupling(
            solver=self,
        )
        self._action_set_dynamic_amr_transfer = exahype2.solvers.fv.actionsets.DynamicAMRTransfer(
            solver=self,
        )

        # Preserve user-configured hooks when action sets are recreated.
        if self._action_set_postprocess_solution is None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.fv.actionsets.EmptyPostprocessSolution(self)
            )
        if self._action_set_preprocess_solution is None:
            self._action_set_preprocess_solution = (
                exahype2.solvers.fv.actionsets.EmptyPreprocessSolution(self)
            )

        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)

        self._action_set_update_patch = None

        self._action_set_initial_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_AMR.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_AMR_throughout_dynamic_amr.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 2
        self._action_set_handle_boundary.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_project_patch_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_roll_over_updates.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_dynamic_amr_transfer.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_postprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        self._action_set_preprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        return (
            "not marker.willBeRefined() and repositories::{}.getSolverState()"
            "!={}::SolverState::GridConstruction"
        ).format(self.get_name_of_global_instance(), self._name)

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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands.
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
        extend it as well, and they will append further ands. Receiving consumes
        the batch the preceding sweep sent, so the guard asks the solver whether
        a sweep has stored face data.
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
            + ".hasStoredFaceData()"
        )

    def _unknown_identifier(self):
        return self._name + "Q"

    def current_unknown_identifier(self):
        return self._name + "QNew"

    def previous_unknown_identifier(self):
        return self._name + "QOld"

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def add_to_Peano4_datamodel(self, datamodel):
        log.debug("Patch data")
        log.debug("----------")
        log.debug(str(self._patch))
        log.debug(str(self._patch_old))
        log.debug("----------")
        log.debug("Patch overlap data")
        log.debug(str(self._patch_overlap_old))
        log.debug(str(self._patch_overlap_new))
        log.debug("----------")
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._patch)
        datamodel.add_cell(self._patch_old)
        datamodel.add_face(self._patch_overlap_old)
        datamodel.add_face(self._patch_overlap_new)
        datamodel.add_face(self._face_label)

        if self._tracer_names:
            for particle in self._tracer_particles:
                datamodel.add_global_object(particle)
            for tracer_set in self._tracer_particle_sets:
                datamodel.add_vertex(tracer_set)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        """
        Cell and face labels come before the data whose load and store decisions
        depend on them.
        """
        step.use_cell(self._cell_label)
        step.use_cell(self._patch)
        step.use_cell(self._patch_old)
        step.use_face(self._face_label)
        step.use_face(self._patch_overlap_old)
        step.use_face(self._patch_overlap_new)
        for tracer_set in self._tracer_particle_sets:
            step.use_vertex(tracer_set)

    def _get_default_includes(self):
        return """
#include "peano4/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_actions_to_init_grid(self, step):
        """!
        The traversal keeps this order while it descends the tree and inverts it
        while it ascends. The refinement action set comes first, so it runs last
        on the way up and restricts the face data the boundary conditions read.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)
        step.add_action_set(self._action_set_roll_over_updates)
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

        if self._tracer_names:
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    def add_actions_to_create_grid(self, step):
        """
        The face label carries the boundary flag, which the grid creation sets
        once.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR_throughout_grid_construction)

    def add_actions_to_plot_solution(self, step):
        plotter = self._plotter(
            filename=self._name,
            patch=self._patch,
            dataset_name=self.current_unknown_identifier(),
            description=self._variable_shortcuts.plot_description,
            representation="cell",
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_cell_data_default_guard(),
        )
        plotter.descend_invocation_order = self._baseline_action_set_descend_invocation_order
        step.add_action_set(plotter)

        for tracer_set, particle, particle_attr, particle_number, particle_unknown in zip(
            self._tracer_particle_sets,
            self._tracer_particles,
            self._particle_attributes,
            self._particle_number,
            self._particle_unknowns
        ):
            tracer_set_index = self._tracer_particle_sets.index(tracer_set)
            # NetCDF (UGRID) has no point-cloud representation, so the particle
            # plotter writes HDF5 whenever NetCDF is the solver's output format.
            particle_file_format = peano4.plotter.get_default_format()
            if particle_file_format == "netcdf":
                particle_file_format = "hdf5"
            # The number identifies a particle across layouts, so a run can be
            # compared per particle whatever rank or task plots it.
            plotter = peano4.plotter.ParticlePlotter(
                 filename=particle.name,
                 particle_set=tracer_set,
                 description="description",
                 time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                 file_format=particle_file_format,
                 attributes=[
                     ("Number", 2, "p->getNumber()"),
                     ("Data", particle_unknown, "p->getData().data(), " + str(particle_unknown)),
                 ],
             )
            plotter.descend_invocation_order = self._plot_descend_invocation_order_within_action_sets
            step.add_action_set(plotter)

            step.add_action_set(self._update_particle_association_action_sets[tracer_set_index])
            step.add_action_set(self._update_particle_parallel_state_action_sets[tracer_set_index])

    def add_actions_to_dynamic_mesh_refinement(self, step):
        """!
        These sweeps are the only ones that change the topology: They realise the
        committed refine and coarsen requests and move the solution across every
        resolution transition Peano creates or destroys.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_dynamic_amr_transfer)
        step.add_action_set(self._action_set_AMR_throughout_dynamic_amr)

    def add_actions_to_perform_time_step(self, step):
        """!
        The inter-grid transfer operators run before the boundary conditions.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)
        step.add_action_set(self._action_set_roll_over_updates)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_update_patch)
        step.add_action_set(self._action_set_project_patch_onto_faces)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

        if self._tracer_names:
            for action_set in self._update_particle_association_action_sets:
                step.add_action_set(action_set)
            for action_set in self._update_particle_parallel_state_action_sets:
                step.add_action_set(action_set)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(self, namespace, output, dimensions):
        self._dimensions = dimensions
        self._namespace = namespace

        if (
            self._riemann_solver_implementation == PDETerms.None_Implementation
            and self._max_eigenvalue_implementation == PDETerms.None_Implementation
        ):
            raise AssertionError(
                f"[{self._name}] max_eigenvalue implementation is required when no Riemann solver is used."
            )

        self._generate_kernels(output)
        self._action_set_project_patch_onto_faces.generate_kernels(namespace, output, dimensions)

        # Assemble the template prefix in a local: Mutating the attribute would
        # double the class name when the generator runs a second time on the
        # same solver object.
        if self._solver_template_file_class_name is None:
            templatefile_prefix = self._templatefile_prefix + self.__class__.__name__
        else:
            templatefile_prefix = (
                self._templatefile_prefix + self._solver_template_file_class_name
            )

        dictionary = {}
        self._init_dictionary_with_default_parameters(dictionary)
        self.add_entries_to_text_replacement_dictionary(dictionary)

        self._variable_shortcuts.generate_shortcut_file(dictionary, namespace, output)

        header_file_abstr_template = templatefile_prefix + "Abstract.template.h"
        cpp_file_abstr_template = templatefile_prefix + "Abstract.template.cpp"
        header_file_template = templatefile_prefix + ".template.h"
        cpp_file_template = templatefile_prefix + ".template.cpp"

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_abstr_template,
                cpp_file_abstr_template,
                "Abstract" + self._name,
                namespace,
                ".",
                dictionary,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                header_file_template,
                cpp_file_template,
                self._name,
                namespace,
                ".",
                dictionary,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_cpp_file(self._name + ".cpp", generated=True)

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring + "\n"

    def set_solver_precisions(self, storage_precision="double", compute_precision="double"):
        self._storage_precision = storage_precision
        self._compute_precision = compute_precision
        self.set_implementation()
        self.create_data_structures()

    def _init_dictionary_with_default_parameters(self, d):
        def render_template(template):
            return render_jinja2_template(template, d, jinja2.DebugUndefined)

        d["DIMENSIONS"] = self._dimensions
        d["NUMBER_OF_VOLUMES_PER_AXIS"] = self._patch.dim[0]
        d["OVERLAP"] = self._overlap
        d["HALO_SIZE"] = int(self._patch_overlap_old.dim[0] / 2)
        d["NAMESPACE"] = self._namespace
        d["SOLVER_NAME"] = self._name
        d["SOLVER_NUMBER"] = self._solver_number
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._variable_shortcuts.auxiliary_variables
        d["MEASURED_UNKNOWNS"] = getattr(self, "_error_norm_measured_unknowns", None) or (
            "{" + ", ".join(str(i) for i in range(self._variable_shortcuts.unknowns)) + "}"
        )
        d["NUMBER_OF_DATA"] = self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables

        d["COMPUTE_PRECISION"] = self._compute_precision
        d["STORAGE_PRECISION"] = self._storage_precision

        d["PATCH_CARDINALITY_1D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["NUMBER_OF_DATA"]
        d["PATCH_CARDINALITY_2D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_1D"]
        d["PATCH_CARDINALITY_3D"] = d["NUMBER_OF_VOLUMES_PER_AXIS"] * d["PATCH_CARDINALITY_2D"]

        d["FACE_CARDINALITY_2D"] = (
            int(self._patch_overlap_new.no_of_unknowns * self._patch_overlap_new.dim[0] * self._patch_overlap_new.dim[1])
        )
        d["FACE_CARDINALITY_3D"] = (
            int(self._patch_overlap_new.no_of_unknowns * self._patch_overlap_new.dim[0] * self._patch_overlap_new.dim[1] * self._patch_overlap_new.dim[2])
        )

        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"] = self.user_solver_includes

        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION"] = self._diffusive_source_term_implementation

        d["USE_FLUX"]                 = self._flux_implementation != PDETerms.None_Implementation
        d["USE_NCP"]                  = self._ncp_implementation != PDETerms.None_Implementation
        d["USE_MAX_EIGENVALUE"]       = self._max_eigenvalue_implementation != PDETerms.None_Implementation
        d["USE_RIEMANN_SOLVER"]       = self._riemann_solver_implementation != PDETerms.None_Implementation
        d["USE_SOURCE"]               = self._source_term_implementation != PDETerms.None_Implementation
        d["USE_DIFFUSIVE_SOURCE"]     = self._diffusive_source_term_implementation != PDETerms.None_Implementation

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        d["STATELESS_FLUX"] = self._use_stateless_flux
        d["STATELESS_NCP"] = self._use_stateless_ncp
        d["STATELESS_MAX_EIGENVALUE"] = self._use_stateless_max_eigenvalue
        d["STATELESS_RIEMANN_SOLVER"] = self._use_stateless_riemann_solver
        d["STATELESS_SOURCE_TERM"] = self._use_stateless_source_term
        d["STATELESS_DIFFUSIVE_SOURCE_TERM"] = self._use_stateless_diffusive_source_term

        d["COMPUTE_KERNEL_CALL"] = render_template(self._compute_kernel_call)

        d["ABSTRACT_SOLVER_USER_DECLARATIONS"] = render_template(
            self._abstract_solver_user_declarations
        ).strip("\n")
        d["ABSTRACT_SOLVER_USER_DEFINITIONS"] = render_template(
            self._abstract_solver_user_definitions
        ).strip("\n")
        d["SOLVER_USER_DECLARATIONS"] = render_template(
            self._solver_user_declarations
        )
        d["SOLVER_USER_DEFINITIONS"] = render_template(
            self._solver_user_definitions
        )
        d["START_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._start_time_step_implementation
        )
        d["FINISH_TIME_STEP_IMPLEMENTATION"] = render_template(
            self._finish_time_step_implementation
        )
        d["CONSTRUCTOR_IMPLEMENTATION"] = render_template(
            self._constructor_implementation
        )
        d["START_SIMULATION_IMPLEMENTATION"] = render_template(
            self._start_simulation_implementation
        )
        d["FINISH_SIMULATION_IMPLEMENTATION"] = render_template(
            self._finish_simulation_implementation
        )

        d["PREPROCESS_RECONSTRUCTED_PATCH"] = render_template(
            self._preprocess_reconstructed_patch
        )
        d["POSTPROCESS_UPDATED_PATCH"] = render_template(
            self._postprocess_updated_patch
        )

        d["COMPUTE_MAX_EIGENVALUE"] = self._compute_eigenvalue
        d["COMPUTE_TIME_STEP_SIZE"] = render_template(
            self._compute_time_step_size
        )
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = render_template(
            self._compute_new_time_step_size
        )

        d["NUMBER_OF_UNKNOWNS_PARTICLE"] = self._particle_unknowns
        if self._tracer_particle_sets:
            d["PARTICLES_CONTAINER"] = self._tracer_particle_sets[0].name
            d["PARTICLE"] = self._tracer_particles[0].name

    def add_tracer(
        self,
        name,
        particle_attributes,
        sort_descend_invocation_order_within_action_sets=-1,
        plot_descend_invocation_order_within_action_sets=65536,
    ):
        self._tracer_names.append(name)
        particle = peano4.particles.Particle(name)
        particle.association = peano4.datamodel.DoFAssociation.Global
        self._plot_descend_invocation_order_within_action_sets = plot_descend_invocation_order_within_action_sets
        self._sort_descend_invocation_order_within_action_sets = sort_descend_invocation_order_within_action_sets

        particle_variable_names = []
        particle_variable_pos = [0]
        if isinstance(particle_attributes, dict):
            unknown_count = sum(particle_attributes["unknowns"].values())
            particle_variable_names += list(particle_attributes["unknowns"].keys())
            for var in particle_attributes["unknowns"].values():
                particle_variable_pos.append(var + particle_variable_pos[-1])
            self._use_var_shortcut_tracer = True
        elif isinstance(particle_attributes, int):
            unknown_count = particle_attributes
        else:
            raise Exception("Invalid format for 'unknowns'. Must be an integer or a dictionary.")

        self._particle_unknowns.append(unknown_count)
        self._particle_variable_names.append(particle_variable_names)
        self._particle_variable_pos.append(particle_variable_pos)

        particle_attr = peano4.datamodel.DoubleArray("data", str(unknown_count))
        particle.add_attribute(particle_attr)
        self._particle_attributes.append(particle_attr)

        particle.add_attribute(
            peano4.datamodel.Enumeration("moveState", ["NotMovedYet", "Moved"])
        )

        particle_number = peano4.datamodel.IntegerArray("number", "2")
        particle.add_attribute(particle_number)
        self._particle_number.append(particle_number)

        particles = peano4.particles.ParticleSet(particle)
        particles.association = peano4.datamodel.DoFAssociation.Vertex

        updateParticleGridAssociation_LiftDrop = peano4.particles.UpdateParticleGridAssociation_LiftDrop(particles)
        updateParticleGridAssociation_LiftDrop.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        self._update_particle_association_action_sets.append(updateParticleGridAssociation_LiftDrop)
        parallel_state_action_set = peano4.particles.UpdateParallelState(particles)
        parallel_state_action_set.descend_invocation_order = self._sort_descend_invocation_order_within_action_sets
        self._update_particle_parallel_state_action_sets.append(parallel_state_action_set)

        self._tracer_particles.append(particle)
        self._tracer_particle_sets.append(particles)

        return particles

    def get_solver_statistics_metadata(self):
        return [
            ("Unknowns", self._variable_shortcuts.unknowns),
            ("Auxiliary Variables", self._variable_shortcuts.auxiliary_variables),
            ("Volumes/axis", self._patch_size),
        ]

    @property
    def unknowns(self):
        return self._variable_shortcuts.unknowns

    @property
    def variable_names(self):
        return self._variable_shortcuts.names

    @property
    def patch_size(self):
        return self._patch_size

    @property
    def auxiliary_variables(self):
        return self._variable_shortcuts.auxiliary_variables

    @patch_size.setter
    def patch_size(self, value):
        self._patch_size = value
        self.create_data_structures()
        self.create_action_sets()

    @property
    def preprocess_reconstructed_patch(self):
        return self._preprocess_reconstructed_patch

    @preprocess_reconstructed_patch.setter
    def preprocess_reconstructed_patch(self, kernel):
        """!
        The C++ snippet runs before the finite-volume update and reads QIn, QOut,
        marker, timeStamp and timeStepSize, with both patches excluding the halo.
        The face data are assembled after this hook, so the snippet cannot read
        them.
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
        The C++ snippet runs after the finite-volume update and reads QIn, QOut,
        faceQIn, faceQOut, marker, timeStamp and timeStepSize. QIn and QOut
        exclude the halo, and the face arrays hold the old and the new halo
        snapshot.
        """
        self._postprocess_updated_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def overlap(self):
        return self._overlap

    @overlap.setter
    def overlap(self, value):
        if value < 1:
            raise Exception(
                "Halo (overlap) size has to be bigger than zero but was {}".format(
                    value
                )
            )
        self._overlap = value
        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def _generate_kernels(self, output):
        pass
