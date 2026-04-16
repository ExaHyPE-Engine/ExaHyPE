# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from enum import IntEnum
from abc import abstractmethod

import jinja2

import peano4.output
import peano4.datamodel
import peano4.plotter

import exahype2.grid
import exahype2.solvers.aderdg.actionsets

from .GenerateKernels import generate_kernels

from exahype2.AlignmentHelper import AlignmentHelper

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis

from exahype2.VariableShortcuts import VariableShortcuts

from peano4.Logger import log

"""
Used to have a central way for all of this class' descendants to differentiate between Gauss-Legendre
and Gauss-Lobatto nodes, helps to avoid having to constantly pass a string and ensure that this string is
the same everywhere.
"""
class Polynomials(IntEnum):
    Gauss_Legendre = 0,
    Gauss_Lobatto = 1

"""
Denotes valid precisions for kernels and storage but also is supposed to allow a little more flexibility
in the naming. If a user wants to name it float because they're more used to C-based languages that's
fine but if they'd rather use single we allow it as well.
"""
PrecisionType = {
    "SP":     "float",
    "float":  "float",
    "single": "float",
    "fp32":   "float",
    "DP":     "double",
    "double": "double",
    "fp64":   "double",
    "quadruple":   "long double",
    "long double": "long double",
    "fp128":       "long double",
    "float16_t":        "std::float16_t",
    "std::float16_t":   "std::float16_t",
    "_Float16":         "_Float16",
    "fp16":             "_Float16",
    "bfloat16_t":       "std::bfloat16_t",
    "std::bfloat16_t":  "std::bfloat16_t",
    "__bf16":           "__bf16",
    "bfloat":           "__bf16",
    "bf16":             "__bf16"
}

class ADERDG(object):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
        min_cell_h,
        max_cell_h,
        use_enclave_tasking: bool,
        number_of_enclave_tasks: int,
    ):
        self._name = name

        """
        Polynomial order of the representation of the underlying function in each spatial dimension.
        The number of support points used to represent this polynomial is always the order + 1
        """
        self._order = order

        self._variable_shortcuts = VariableShortcuts(name, unknowns, auxiliary_variables)

        """
        Minimal and maximal accepted sizes for the cell, domain will be subdivided until the cell size is smaller
        than the minimal size but will never be subdivided to be smaller than the minimal size.
        """
        if min_cell_h > max_cell_h:
            raise Exception(
                "min_cell_h ("
                + str(min_cell_h)
                + ") is bigger than max_cell_h ("
                + str(max_cell_h)
                + ")"
            )
        self._min_cell_h = min_cell_h
        self._max_cell_h = max_cell_h

        self._dimensions    = 2
        self._namespace     = ""
        self._simd_width    = 1
        self._solver_template_file_class_name = None
        self._templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        self._is_linear     = False
        self._polynomials   = Polynomials.Gauss_Legendre
        self._basis         = GaussLegendreBasis(order + 1)
        self._use_libxsmm   = False

        self._initialise_patches = False

        self._predictor_computation_precisions      = [PrecisionType["double"]]
        self._corrector_computation_precision       = PrecisionType["double"]
        self._solution_persistent_storage_precision = PrecisionType["double"]
        self._precompute_picard_precision           = PrecisionType["double"]
        self._use_half_precision                    = False

        self._additional_precisions = None

        # Used by children classes or setters in order to add constants or includes to the corresponding part
        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        """
        These specify the time step, they must be set by child classes in order to advance the timestepping.
        These are the main difference between child classes as they all use the same underlying routines.
        """
        self._compute_time_step_size = "#error Not yet defined"
        self._compute_new_time_step_size = "#error Not yet defined"

        self._space_time_predictor_call = "#error Not yet defined"
        self._riemann_solver_call = "#error Not yet defined"
        self._face_integral_call = "#error Not yet defined"

        self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
        self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
        self._refinement_criterion_implementation = PDETerms.Empty_Implementation
        self._flux_implementation                 = PDETerms.None_Implementation
        self._ncp_implementation                  = PDETerms.None_Implementation
        self._source_term_implementation          = PDETerms.None_Implementation
        self._diffusive_source_term_implementation= PDETerms.None_Implementation
        self._point_sources_implementation        = PDETerms.None_Implementation
        self._init_point_sources_implementation   = PDETerms.None_Implementation
        self._max_eigenvalue_implementation       = PDETerms.None_Implementation
        self._adjust_solution_implementation      = PDETerms.None_Implementation
        self._material_param_implementation       = PDETerms.None_Implementation
        self._precision_criterion_implementation  = PDETerms.None_Implementation
        self._riemann_solver_implementation       = PDETerms.None_Implementation
        self._number_of_point_sources             = 0

        self._use_stateless_initial_conditions = False
        self._use_stateless_boundary_conditions = False
        self._use_stateless_refinement_criterion = False
        self._use_stateless_flux = False
        self._use_stateless_ncp = False
        self._use_stateless_max_eigenvalue = False
        self._use_stateless_riemann_solver = False
        self._use_stateless_source_term = False
        self._use_stateless_diffusive_source_term = False
        self._use_stateless_point_source = False
        self._use_stateless_init_point_sources = False
        self._use_stateless_adjust_solution = False
        self._use_stateless_material_parameters = False
        self._use_diffusive_flux = False

        self._use_enclave_tasking = use_enclave_tasking
        self._number_of_enclave_tasks = number_of_enclave_tasks
        self._enclave_task_priority = "tarch::multicore::Task::DefaultPriority - 1"

        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation  = ""
        self._finish_time_step_implementation = ""
        self._print_time_step_implementation = ""

        self._start_grid_initialisation_implementation = ""

        self._constructor_implementation = ""
        self._destructor_implementation = ""

        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

        """
        These are ADER-DG CFL factors, they help determine the maximal stable timestep size.
        Up to order 4, computed via von Neumann analysis [1].
        [1] M. Dumbser, D. S. Balsara, E. F. Toro, and C.-D. Munz,
        ‘A unified framework for the construction of one-step finite volume and discontinuous Galerkin schemes on unstructured meshes’, Journal of Computational Physics, vol. 227, no. 18, pp. 8209–8253, Sep. 2008.
        They are necessary to compute stable timestep sizes as dt <= c*dx / (lambda_max*(2N+1)
        with c the CFL for given order, lambda_max the maximal eigenvalue, N the order and dx the minimal mesh distance.
        """
        self._cflAder = [
            1.0,
            0.33,
            0.17,
            0.1,
            0.069,
            0.045,
            0.038,
            0.03,
            0.02,
            0.015,
            0.008821428571428572,
            0.005235326086956522,
            0.0031307249999999996,
            0.0018842326388888888,
            0.001140285614224138,
            0.0006933672202620968,
        ]

        # Safety factor for CFL (Courant-Friedrichs-Lewy) condition
        self._cflSafetyFactor = 0.0

        """
        Used if the user wants to add some function to postprocessing, meaning
        it should be added after every other computation in ADER-DG is done.
        """
        self._action_set_postprocess_solution = None

        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self.create_data_structures()
        self.create_action_sets()

    """
    Creates all the structures that are attached on the Peano grid either in a cell or on the faces.
    These are defined as Peano patches which have given sizes to contain persistent data as well as
    conditions for when they should be merged, sent, received, stored, loaded, etc.
    """
    @abstractmethod
    def create_data_structures(self):
        self._number_of_face_projections = 1

        # Current time step contains the solution, projections are respectively the projection
        # of the solution and of the fluxes on the faces as predicted by the space-time predictor.
        self._current_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier(),
            self._solution_persistent_storage_precision,
        )
        self._previous_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier() + "_old",
            self._solution_persistent_storage_precision,
        )
        self._rhs_estimates_projection = peano4.datamodel.Patch(
            (2, self._order+1, self._order+1),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier() + "Estimates",
            self._corrector_computation_precision,
        )
        self._flux_estimates_projection = peano4.datamodel.Patch(
            (2, self._order+1, self._order+1),
            self._variable_shortcuts.unknowns,
            self._unknown_identifier() + "FluxEstimates",
            self._corrector_computation_precision,
        )

        self._current_time_step.generator = peano4.datamodel.PatchToData(
            self._current_time_step,
            self._solution_persistent_storage_precision
        )
        self._previous_time_step.generator = peano4.datamodel.PatchToData(
            self._previous_time_step,
            self._solution_persistent_storage_precision
        )
        self._rhs_estimates_projection.generator = peano4.datamodel.PatchToData(
            self._rhs_estimates_projection,
            self._corrector_computation_precision
        )
        self._flux_estimates_projection.generator = peano4.datamodel.PatchToData(
            self._flux_estimates_projection,
            self._corrector_computation_precision
        )

        # Custom merge implementation takes into account that the faces project such that
        # the left cell projects to the left half of the face and the right cell projects to the right half.
        self._rhs_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._rhs_estimates_projection)
        )
        self._rhs_estimates_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._flux_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._flux_estimates_projection)
        )
        self._flux_estimates_projection.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._current_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._previous_time_step.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_cell_data_to_compute_kernels_default_guard(),
            self._load_cell_data_default_guard(),
            self._store_cell_data_default_guard(),
        )

        self._rhs_estimates_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_face_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        # Gets overwritten in subclasses to take into account when to send or receive faces
        self._rhs_estimates_projection.generator.send_condition = "false"
        self._rhs_estimates_projection.generator.receive_and_merge_condition = "false"

        self._flux_estimates_projection.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
            self._provide_face_data_to_compute_kernels_default_guard(),
            self._load_face_data_default_guard(),
            self._store_face_data_default_guard()
        )

        self._flux_estimates_projection.generator.send_condition = "false"
        self._flux_estimates_projection.generator.receive_and_merge_condition = "false"

        self._current_time_step.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._previous_time_step.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._rhs_estimates_projection.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._flux_estimates_projection.generator.includes += """
#include "../repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    """
    Creates the action sets, these are actions tied to the grid that are executed at given points of
    grid generation, construction or steps of the ADER-DG solver. Peano handles grid structures that
    contain data as well as operations that are executed on that data. These are the latter.
    The guard here are the conditions for the execution of the operation, as in they are only executed
    if the guard evaluates to true.
    """
    @abstractmethod
    def create_action_sets(self):
        # Updates faces and cells (whether at boundary, refined etc.)
        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)
        self._action_set_update_face_label.descend_invocation_order = 0
        self._action_set_update_cell_label.descend_invocation_order = 0

        # Sets initial conditions for the solution
        self._action_set_initial_conditions = exahype2.solvers.aderdg.actionsets.InitialCondition(self, self._store_cell_data_default_guard())
        self._action_set_initial_conditions.descend_invocation_order = 1

        self._action_set_handle_boundary = exahype2.solvers.aderdg.actionsets.HandleBoundary(self, self._store_boundary_data_default_guard())
        self._action_set_handle_boundary.descend_invocation_order = 1

        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.aderdg.actionsets.DynamicAMR(
            solver=self,
            delete_guard=self._delete_face_data_default_guard(),
            interpolate_guard=self._interpolate_face_data_default_guard(),
            restrict_guard=self._restrict_face_data_default_guard(),
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = 1

        # All the actual computation of the solution happens in the prediction and correction action sets.
        # Predictor extrapolates the solution from time t onto time-nodes, then projects this
        # extrapolated solution to the faces.
        # Correction uses the extrapolated face solutions from the predictor and the neighboring
        # cells to compute a solution at the next time step.
        self._action_set_correction = exahype2.solvers.aderdg.actionsets.Correction(
            solver=self, 
            cell_guard=self._store_cell_data_default_guard(),
            face_guard=self._store_face_data_default_guard()
        )
        self._action_set_correction.descend_invocation_order = 2  # Needs to happen after handle_boundary

        self._action_set_prediction = exahype2.solvers.aderdg.actionsets.Prediction(self, self._store_cell_data_default_guard())
        self._action_set_prediction.descend_invocation_order = 3

        # Handles adaptive mesh refinement
        self._action_set_AMR = exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard()
            + " and (repositories::"
            + self.get_name_of_global_instance()
            + ".isFirstGridSweepOfTimeStep() or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation)",
            build_up_new_refinement_instructions=True,
            called_by_grid_construction=False,
            implement_previous_refinement_instructions=True
        )
        self._action_set_AMR.descend_invocation_order = 4

        self._action_set_AMR_commit_without_further_analysis = exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=False,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=False,
        )
        self._action_set_AMR_commit_without_further_analysis.descend_invocation_order = 4

        self._action_set_AMR_throughout_grid_construction = exahype2.solvers.aderdg.actionsets.AdaptivityCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=True,
        )
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = 4

        if self._action_set_postprocess_solution == None:
            self._action_set_postprocess_solution = exahype2.solvers.aderdg.actionsets.EmptyPostprocessSolution(solver=self)
        self._action_set_postprocess_solution.descend_invocation_order = 4

        self._plotter = None
        self.set_plotter(peano4.plotter.HDF5PatchNodalPlotter)

    def create_readme_descriptor(self, domain_offset, domain_size):
        return (
            """ ExaHyPE 2 ADER-DG solver of order: """
            + str(self._order)
            + """ with a domain size of: """
            + str(domain_size)
            + """ and a domain offset of: """
            + str(domain_offset)
        )

    def get_user_action_set_includes(self):
        return self._user_action_set_includes

    def get_user_solver_includes(self):
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        self._user_solver_includes += value

    def _load_cell_data_default_guard(self):
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

    def _store_cell_data_default_guard(self):
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
        )

    def _provide_cell_data_to_compute_kernels_default_guard(self):
        return (
                    "not marker.willBeRefined() "
                    + "and repositories::"
                    + self.get_name_of_global_instance()
                    + ".getSolverState()!="
                    + self._name
                    + "::SolverState::GridConstruction"
                )

    def _load_face_data_default_guard(self):
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
        return (
            "not marker.willBeRefined() "
            + "and repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridConstruction"
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

    def _store_boundary_data_default_guard(self):
        return (
            self._store_face_data_default_guard()
            + " and not repositories::"
            + self.get_name_of_global_instance()
            + ".PeriodicBC[marker.getSelectedFaceNumber()%DIMENSIONS]"
            + " and not marker.hasBeenRefined() and fineGridFace"
            + self._name
            + "FaceLabel.getBoundary()"
        )

    def _delete_face_data_default_guard(self):
        return "true"

    def _interpolate_face_data_default_guard(self):
        return (
            "repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()!="
            + self._name
            + "::SolverState::GridInitialisation"
        )

    def _restrict_face_data_default_guard(self):
        return "true"

    def _unknown_identifier(self):
        return self._name + "Q"

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
""" + "#include <stdfloat>" if self._use_half_precision else ""

    def add_to_Peano4_datamodel(self, datamodel):
        log.debug("Patch data")
        log.debug("----------")
        log.debug(str(self._current_time_step))
        log.debug("Patch overlap data")
        log.debug("----------")
        log.debug(str(self._rhs_estimates_projection))
        log.debug(str(self._flux_estimates_projection))
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._current_time_step)
        datamodel.add_cell(self._previous_time_step)
        datamodel.add_face(self._face_label)
        datamodel.add_face(self._rhs_estimates_projection)
        datamodel.add_face(self._flux_estimates_projection)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        step.use_cell(self._previous_time_step)
        step.use_face(self._face_label)
        step.use_face(self._rhs_estimates_projection)
        step.use_face(self._flux_estimates_projection)

    def add_actions_to_init_grid(self, step):
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        if evaluate_refinement_criterion:
            step.add_action_set(self._action_set_AMR_throughout_grid_construction)
        else:
            step.add_action_set(self._action_set_AMR_commit_without_further_analysis)

    def add_actions_to_plot_solution(self, step, output_path):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)

        self._plotter.set_output_path(output_path)
        step.add_action_set(self._plotter)

        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)

    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)

        step.add_action_set(self._action_set_handle_boundary)
        step.add_action_set(self._action_set_correction)
        step.add_action_set(self._action_set_prediction)

        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_implementation_files_to_project(self, namespace, output, dimensions, subdirectory="", abstract_overwrite=True):
        self._dimensions = dimensions
        self._namespace = namespace
        self._simd_width = int(output.makefile.d["SIMDLEN"])

        if (not self._is_linear) and (
            self._material_param_implementation != PDETerms.None_Implementation
            or self._point_sources_implementation != PDETerms.None_Implementation
        ):
            raise AssertionError(
                "Material parameters and point sources are currently only available for linear generated kernels."
            )

        if (
            (
                self._riemann_solver_implementation == PDETerms.None_Implementation
                or self.__class__.__name__ == "GlobalAdaptiveTimeStep"
            )
            and self._max_eigenvalue_implementation == PDETerms.None_Implementation
        ):
            raise AssertionError(
                f"[{self._name}] max_eigenvalue implementation is required when no Riemann solver is used "
                f"or when global adaptive time stepping is enabled."
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

        self._variable_shortcuts.generate_shortcut_file(d, namespace, output, subdirectory)

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                self._templatefile_prefix + "Abstract.template.h",
                self._templatefile_prefix + "Abstract.template.cpp",
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
                self._templatefile_prefix + ".template.h",
                self._templatefile_prefix + ".template.cpp",
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
        output.makefile.add_h_file(subdirectory + "Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(subdirectory + self._name + ".h", generated=True)
        output.makefile.add_cpp_file(subdirectory + "Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_cpp_file(subdirectory + self._name + ".cpp", generated=True)

        if self._use_enclave_tasking:
            generated_prediction_enclave_task = (
                peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                    headfile_template=os.path.dirname(os.path.realpath(__file__))
                    + "/tasks/"
                    + "Prediction.EnclaveTask.template.h",
                    cppfile_template=os.path.dirname(os.path.realpath(__file__))
                    + "/tasks/"
                    + "Prediction.EnclaveTask.template.cpp",
                    classname="Prediction{}EnclaveTask".format(self._name),
                    namespace=namespace + ["tasks"],
                    subdirectory=subdirectory + "tasks",
                    dictionary=d,
                    default_overwrite=True,
                    apply_iteratively=True
                )
            )
            output.add(generated_prediction_enclave_task)
            output.makefile.add_h_file(subdirectory + "tasks/" + "Prediction{}EnclaveTask".format(self._name) + ".h", generated=True)
            output.makefile.add_cpp_file(subdirectory + "tasks/" + "Prediction{}EnclaveTask".format(self._name) + ".cpp", generated=True)

        generate_kernels(solver=self, d=d, output=output, subdirectory=subdirectory)

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring

    @property
    def name(self):
        return self._name

    @property
    def order(self):
        return self._order

    @property
    def unknowns(self):
        return self._variable_shortcuts.unknowns

    @property
    def auxiliary_variables(self):
        return self._variable_shortcuts.auxiliary_variables

    @property
    def preprocess_reconstructed_patch(self):
        return self._preprocess_reconstructed_patch

    @preprocess_reconstructed_patch.setter
    def preprocess_reconstructed_patch(self, kernel):
        self._preprocess_reconstructed_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        self._postprocess_updated_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def _init_dictionary_with_default_parameters(self, d):
        d["DIMENSIONS"]         = self._dimensions
        d["NAMESPACE"]          = self._namespace
        d["SOLVER_NAME"]        = self._name
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"]    = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()

        d["ORDER"]              = self._order
        d["NUMBER_OF_DOF"]      = self._order + 1
        d["NUMBER_OF_DOF_3D"] = 1 if self._dimensions == 2 else d["NUMBER_OF_DOF"]
        d["NUMBER_OF_UNKNOWNS"]             = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._variable_shortcuts.auxiliary_variables
        d["NUMBER_OF_DATA"]                 = self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables

        alignment_helper                          = AlignmentHelper(self._simd_width)
        d["NUMBER_OF_DOF_PADDED"]                 = alignment_helper.get_size_with_padding(self._order + 1)
        d["NUMBER_OF_UNKNOWNS_PADDED"]            = alignment_helper.get_size_with_padding(self._variable_shortcuts.unknowns)
        d["NUMBER_OF_AUXILIARY_VARIABLES_PADDED"] = alignment_helper.get_size_with_padding(self._variable_shortcuts.auxiliary_variables)
        d["NUMBER_OF_DATA_PADDED"]                = alignment_helper.get_size_with_padding(self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables)

        d["CFL_SAFETY_FACTOR"]  = self._cflSafetyFactor
        d["CFL_ADER"]           = self._cflAder[self._order]  # /(2*(self._order+1)+1)

        d["USE_HALF_PRECISION"] = self._use_half_precision
        d["IS_LINEAR"] = self._is_linear
        d["LINEARITY"] = "linear" if self._is_linear else "nonlinear"

        d["USE_GAUSS_LOBATTO"] = ( "true" if self._polynomials is Polynomials.Gauss_Lobatto else "false" )
        d["POLYNOMIAL_TYPE"]   = ( "lobatto" if self._polynomials is Polynomials.Gauss_Lobatto else "legendre" )

        d["SOLUTION_STORAGE_PRECISION"] = self._solution_persistent_storage_precision

        d["PREDICTOR_COMPUTATION_PRECISIONS"] = self._predictor_computation_precisions
        d["CORRECTOR_COMPUTATION_PRECISION"]  = self._corrector_computation_precision
        d["PICARD_COMPUTATION_PRECISION"]     = self._precompute_picard_precision

        d["COMPUTATION_PRECISIONS"] = self._predictor_computation_precisions[:]
        if self._additional_precisions!=None and self._additional_precisions not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._additional_precisions)
        if self._corrector_computation_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._corrector_computation_precision)
        if self._precompute_picard_precision!=False and self._precompute_picard_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._precompute_picard_precision)
        if self._solution_persistent_storage_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._solution_persistent_storage_precision)
        d["NUMBER_OF_PREDICTOR_PRECISIONS"] = len(d["PREDICTOR_COMPUTATION_PRECISIONS"])
        d["NUMBER_OF_PRECISIONS"] = len(d["COMPUTATION_PRECISIONS"])

        if self._min_cell_h > self._max_cell_h:
            raise Exception("min/max h are inconsistent")
        d["MAX_CELL_H"] = self._max_cell_h
        d["MIN_CELL_H"] = self._min_cell_h

        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"]  = self.get_user_solver_includes()

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
        d["START_GRID_INITIALISATION_IMPLEMENTATION"] = jinja2.Template(
            self._start_grid_initialisation_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)

        d["CONSTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._constructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["DESTRUCTOR_IMPLEMENTATION"] = jinja2.Template(
            self._destructor_implementation, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_time_step_size, undefined=jinja2.DebugUndefined
        ).render(**d)
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = jinja2.Template(
            self._compute_new_time_step_size, undefined=jinja2.DebugUndefined
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

        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
        d["BOUNDARY_CONDITIONS_IMPLEMENTATION"] = self._boundary_conditions_implementation
        d["REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
        d["ADAPTIVE_PRECISION_IMPLEMENTATION"] = self._precision_criterion_implementation
        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["MAX_EIGENVALUE_IMPLEMENTATION"] = self._max_eigenvalue_implementation
        d["RIEMANN_SOLVER_IMPLEMENTATION"] = self._riemann_solver_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["DIFFUSIVE_SOURCE_TERM_IMPLEMENTATION"] = self._diffusive_source_term_implementation
        d["ADJUST_SOLUTION_IMPLEMENTATION"] = self._adjust_solution_implementation
        d["NUMBER_OF_POINT_SOURCES"]  = self._number_of_point_sources
        d["POINT_SOURCES_IMPLEMENTATION"] = self._point_sources_implementation
        d["INIT_POINT_SOURCES_IMPLEMENTATION"] = self._init_point_sources_implementation
        d["MATERIAL_PARAM_IMPLEMENTATION"] = self._material_param_implementation

        d["USE_FLUX"]                 = self._flux_implementation != PDETerms.None_Implementation
        d["USE_NCP"]                  = self._ncp_implementation != PDETerms.None_Implementation
        d["USE_MAX_EIGENVALUE"]       = self._max_eigenvalue_implementation != PDETerms.None_Implementation
        d["USE_RIEMANN_SOLVER"]       = self._riemann_solver_implementation != PDETerms.None_Implementation
        d["USE_SOURCE"]               = self._source_term_implementation != PDETerms.None_Implementation
        d["USE_DIFFUSIVE_SOURCE"]     = self._diffusive_source_term_implementation != PDETerms.None_Implementation
        d["USE_MATERIAL_PARAMETERS"]  = self._material_param_implementation != PDETerms.None_Implementation
        d["USE_POINT_SOURCE"]         = self._point_sources_implementation != PDETerms.None_Implementation
        d["USE_ADJUST_SOLUTION"]      = self._adjust_solution_implementation != PDETerms.None_Implementation
        d["USE_DIFFUSIVE_FLUX"]       = self._use_diffusive_flux

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        d["STATELESS_FLUX"] = self._use_stateless_flux
        d["STATELESS_NCP"] = self._use_stateless_ncp
        d["STATELESS_MAX_EIGENVALUE"] = self._use_stateless_max_eigenvalue
        d["STATELESS_RIEMANN_SOLVER"] = self._use_stateless_riemann_solver
        d["STATELESS_SOURCE_TERM"] = self._use_stateless_source_term
        d["STATELESS_DIFFUSIVE_SOURCE_TERM"] = self._use_stateless_diffusive_source_term
        d["STATELESS_ADJUST_SOLUTION"] = self._use_stateless_adjust_solution
        d["STATELESS_POINT_SOURCE"] = self._use_stateless_point_source
        d["STATELESS_INIT_POINT_SOURCES"] = self._use_stateless_init_point_sources
        d["STATELESS_MATERIAL_PARAMETERS"] = self._use_stateless_material_parameters

        d["USE_ENCLAVE_TASKING"] = self._use_enclave_tasking
        d["NUMBER_OF_ENCLAVE_TASKS"] = self._number_of_enclave_tasks
        d["ENCLAVE_TASK_PRIORITY"] = self._enclave_task_priority
        d["ENCLAVE_TASK_TYPE"] = f"{self._name}EnclaveTask"

        d["SPACE_TIME_PREDICTOR_CALL"] = jinja2.Template(self._space_time_predictor_call, undefined=jinja2.DebugUndefined).render(**d)
        d["RIEMANN_SOLVER_CALL"] = jinja2.Template(self._riemann_solver_call, undefined=jinja2.DebugUndefined).render(**d)
        d["FACE_INTEGRAL_CALL"] = jinja2.Template(self._face_integral_call, undefined=jinja2.DebugUndefined).render(**d)

        d["QUADRATURE_POINTS"]      = ", ".join(self._basis.quadrature_points())

        d["USE_LIBXSMM"] = self._use_libxsmm

        # self._basis.init_dictionary_with_default_parameters(d)

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
        material_parameters=None,
        number_of_point_sources=0,
        point_source=None,
        init_point_source_location=None,
        use_diffusive_flux=None,
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

        if material_parameters is not None:
            self._material_param_implementation = material_parameters
            if material_parameters is not PDETerms.User_Defined_Implementation:
                self._use_stateless_material_parameters = True

        assert(type(number_of_point_sources) == int)
        assert(number_of_point_sources >= 0)
        self._number_of_point_sources = number_of_point_sources

        if number_of_point_sources > 0:
            if point_source is not None:
                self._point_sources_implementation = point_source
                if point_source is not PDETerms.User_Defined_Implementation:
                    self._use_stateless_point_source = True

            if init_point_source_location is not None:
                self._init_point_sources_implementation = init_point_source_location
                if init_point_source_location is not PDETerms.User_Defined_Implementation:
                    self._use_stateless_init_point_sources = True

        if use_diffusive_flux is not None:
          self._use_diffusive_flux = use_diffusive_flux

        self.create_data_structures()
        self.create_action_sets()

    def add_kernel_optimisations(
        self,
        is_linear                             = False,
        polynomials                           = Polynomials.Gauss_Legendre,
        use_libxsmm                           = False,
        precision                             = False,
        predictor_computation_precisions      = False,
        corrector_computation_precision       = False,
        solution_persistent_storage_precision = False,
        precompute_picard_precision           = False,
        precision_criterion_implementation    = PDETerms.User_Defined_Implementation,
        initialise_patches                    = False
    ):
        if polynomials is Polynomials.Gauss_Legendre:
            self._basis = GaussLegendreBasis(self._order + 1)
        elif polynomials is Polynomials.Gauss_Lobatto:
            self._basis = GaussLobattoBasis(self._order + 1)
        else:
            raise Exception(
                "No proper basis chosen: {}, valid options are Gauss_Legendre and Gauss_Lobatto nodes".format(
                    polynomials
                )
            )

        self._is_linear = is_linear
        self._polynomials = polynomials

        self._use_libxsmm = use_libxsmm

        if precision != False:
            if precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision was not a valid choice. valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            else:
                self._predictor_computation_precisions      = [PrecisionType[precision]]
                self._precompute_picard_precision           = PrecisionType[precision]
                self._solution_persistent_storage_precision = PrecisionType[precision]
                self._corrector_computation_precision       = PrecisionType[precision]

        if solution_persistent_storage_precision != False:
            if solution_persistent_storage_precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision was not a valid choice. valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._solution_persistent_storage_precision = PrecisionType[solution_persistent_storage_precision]

        if corrector_computation_precision != False:
            if corrector_computation_precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision was not a valid choice. valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._corrector_computation_precision = PrecisionType[corrector_computation_precision]

        if precompute_picard_precision != False:
            if precompute_picard_precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision was not a valid choice. valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._precompute_picard_precision = PrecisionType[precompute_picard_precision]

        if predictor_computation_precisions != False:
            self._predictor_computation_precisions = []
            for prec in predictor_computation_precisions:
                if prec not in PrecisionType.keys():
                    raise AssertionError(
                        "The chosen precision was not a valid choice. valid choices are: "
                        + ", ".join(PrecisionType.keys())
                    )
                self._predictor_computation_precisions.append(PrecisionType[prec])

        if len(self._predictor_computation_precisions) > 1:
            self._precision_criterion_implementation = precision_criterion_implementation

        self._initialise_patches = initialise_patches

        self.create_data_structures()
        self.create_action_sets()

    def set_plotter(self, plotter):
        mapping = []
        for z in self._basis.quadrature_points():
            for y in self._basis.quadrature_points():
                for x in self._basis.quadrature_points():
                    mapping.append((x, y, z))

        self._plotter = plotter(
            output_path=self._name,
            filename=self._name,
            patch=self._previous_time_step,
            dataset_name=self._unknown_identifier() + "_old",
            description=self._variable_shortcuts.plot_description,
            mapping=mapping,
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_cell_data_default_guard(),
        )

    def set_face_plotter(self, plotter, guard):
        mapping = []
        for z in self._basis.quadrature_points():
            for y in self._basis.quadrature_points():
                for x in self._basis.quadrature_points():
                    mapping.append((x, y, z))

        self._plotter = plotter(
            output_path=self._name,
            filename=self._name,
            patch=self._rhs_estimates_projection,
            dataset_name=self._unknown_identifier() + "Estimates",
            description=self._variable_shortcuts.plot_description,
            mapping=mapping,
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_face_data_default_guard() + " and " + guard,
        )

def get_face_overlap_merge_implementation(patch_overlap):
    d = {
        "OVERLAP": patch_overlap.dim[0]
        / 2,  # Assumed to always be two, but present here anyway.
        "DOFS_PER_AXIS": patch_overlap.dim[1],
        "UNKNOWNS": patch_overlap.no_of_unknowns,
    }
    Template = """
#if DIMENSIONS == 2
  constexpr int SpaceFaceSize = {DOFS_PER_AXIS} * {UNKNOWNS}; // Order + 1
#else
  constexpr int SpaceFaceSize = {DOFS_PER_AXIS} * {DOFS_PER_AXIS} * {UNKNOWNS}; // (Order + 1)^2
#endif

  auto* dst = this->data();
  const auto* const src = neighbour.data();
  assertion(dst);
  assertion(src);

  const int faceDimension = marker.getSelectedFaceNumber() % DIMENSIONS;
  // If the outer normal is positive, the normal points to the right so the face is on the right.
  const int faceLR = (marker.outerNormal()[faceDimension] > 0.0 ? 0 : 1);

  // std::copy_n(from, how_many, to);
  std::copy_n(
    &src[(1 - faceLR) * SpaceFaceSize],
    SpaceFaceSize,
    &dst[(1 - faceLR) * SpaceFaceSize]
  );
"""
    return Template.format(**d)
