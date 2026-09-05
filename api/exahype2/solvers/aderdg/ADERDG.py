# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
from enum import IntEnum
from abc import abstractmethod

import jinja2

import peano4.output
import peano4.checkpoint
import peano4.datamodel
import peano4.plotter

import exahype2.grid
import exahype2.solvers.aderdg.actionsets

from .GenerateKernels import generate_kernels

from exahype2.AlignmentHelper import AlignmentHelper

from exahype2.solvers.PDETerms import PDETerms
from exahype2.solvers.aderdg.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis
from exahype2.solvers.limiting.SubcellLimiting import SubcellLimiting

from exahype2.VariableShortcuts import VariableShortcuts

from peano4.Logger import log
from peano4.output.Helper import render_jinja2_template


class Polynomials(IntEnum):
    Gauss_Legendre = 0
    Gauss_Lobatto = 1


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


class ADERDG(SubcellLimiting):
    def __init__(
        self,
        name,
        order,
        unknowns,
        auxiliary_variables,
    ):
        self._name = name
        self._solver_number = 0

        self._order = order

        self._variable_shortcuts = VariableShortcuts(name, unknowns, auxiliary_variables)

        self._dimensions    = 2
        self._namespace     = ""
        self._simd_width    = 1
        self._solver_template_file_class_name = None
        self._templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        self._is_linear     = False
        self._polynomials   = Polynomials.Gauss_Legendre
        self._basis         = GaussLegendreBasis(order + 1)

        self._initialise_patches = False

        self._predictor_computation_precisions      = [PrecisionType["double"]]
        self._corrector_computation_precision       = PrecisionType["double"]
        self._solution_persistent_storage_precision = PrecisionType["double"]
        self._precompute_picard_precision           = PrecisionType["double"]
        self._use_half_precision                    = False

        self._additional_precisions = None

        # Used by child classes or setters to add constants or includes to the corresponding part
        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        # A child class sets these to advance the time stepping.
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
        self._use_stateless_material_parameters = False
        self._use_diffusive_flux = False

        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation  = ""
        self._finish_time_step_implementation = ""

        self._start_grid_initialisation_implementation = ""

        self._constructor_implementation = ""
        self._destructor_implementation = ""

        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

        """
        These are the ADER-DG CFL factors which determine the maximal stable time step size.
        Up to order 4, they are computed via von Neumann analysis [1].
        [1] M. Dumbser, D. S. Balsara, E. F. Toro, and C.-D. Munz,
        ‘A unified framework for the construction of one-step finite volume and discontinuous Galerkin schemes on unstructured meshes’, Journal of Computational Physics, vol. 227, no. 18, pp. 8209–8253, Sep. 2008.
        A stable time step size follows as dt <= c*dx / (lambda_max*(2N+1)) with c the CFL factor
        for the given order, lambda_max the maximal eigenvalue, N the order and dx the minimal
        mesh distance.
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

        # The postprocessing action set runs after every other ADER-DG computation of the sweep.
        self._action_set_postprocess_solution = None

        self._preprocess_reconstructed_patch = ""
        self._postprocess_updated_patch = ""

        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def create_data_structures(self):
        self._number_of_face_projections = 1

        # The two projections hold the solution and the fluxes the space-time predictor
        # writes on the faces.
        self._current_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.current_unknown_identifier(),
            self._solution_persistent_storage_precision,
        )
        self._previous_time_step = peano4.datamodel.Patch(
            (self._order + 1, self._order + 1, self._order + 1),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self.previous_unknown_identifier(),
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

        # The left cell projects to the left half of the face and the right cell to the
        # right half, so the merge copies half a face.
        self._rhs_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._rhs_estimates_projection)
        )
        self._rhs_estimates_projection.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._flux_estimates_projection.generator.merge_method_definition = (
            get_face_overlap_merge_implementation(self._flux_estimates_projection)
        )
        self._flux_estimates_projection.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._rhs_estimates_projection.generator.use_copy_for_exchange = "true"

        # A child class overwrites the send and the receive condition.
        self._rhs_estimates_projection.generator.send_condition = "false"
        self._rhs_estimates_projection.generator.receive_and_merge_condition = "false"
        self._rhs_estimates_projection.generator.can_receive = self._load_face_data_default_guard()
        self._rhs_estimates_projection.generator.can_send = self._store_face_data_default_guard()

        self._flux_estimates_projection.generator.use_copy_for_exchange = "true"

        self._flux_estimates_projection.generator.send_condition = "false"
        self._flux_estimates_projection.generator.receive_and_merge_condition = "false"
        self._flux_estimates_projection.generator.can_receive = self._load_face_data_default_guard()
        self._flux_estimates_projection.generator.can_send = self._store_face_data_default_guard()

        self._previous_time_step.generator.can_receive = self._load_cell_data_default_guard()
        self._previous_time_step.generator.can_send = self._store_cell_data_default_guard()

        self._current_time_step.generator.can_receive = self._load_cell_data_default_guard()
        self._current_time_step.generator.can_send = self._store_cell_data_default_guard()

        self._current_time_step.generator.includes += """
#include "repositories/SolverRepository.h"
"""

        self._previous_time_step.generator.includes += """
#include "repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

        self._add_subcell_limiter_data_structures()

    @abstractmethod
    def create_action_sets(self):
        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)
        self._action_set_update_face_label.descend_invocation_order = 0
        self._action_set_update_cell_label.descend_invocation_order = 0

        # On a resume the checkpoint loader replaces the initial condition.
        self._action_set_initial_conditions = exahype2.solvers.aderdg.actionsets.InitialCondition(
            self,
            "not ::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath() and ("
            + self._store_cell_data_default_guard()
            + ")",
        )
        self._action_set_initial_conditions.descend_invocation_order = 1
        self._action_set_checkpoint_loading = exahype2.solvers.aderdg.actionsets.CheckpointLoading(
            self, self._store_cell_data_default_guard()
        )
        self._action_set_checkpoint_loading.descend_invocation_order = 1

        # This action set couples the resolution transitions of a stable grid and changes
        # no topology.
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.aderdg.actionsets.AdaptiveMeshFaceCoupling(
            solver=self,
            interpolate_guard=self._interpolate_face_data_default_guard(),
            restrict_guard=self._restrict_face_data_default_guard(),
            solve_guard=self._store_cell_data_default_guard(),
        )
        self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.descend_invocation_order = 1

        # This action set realises the topology changes and transfers the solution across
        # every created or destroyed resolution transition.
        self._action_set_dynamic_amr_transfer = exahype2.solvers.aderdg.actionsets.DynamicAMRTransfer(
            solver=self,
            interpolate_guard=self._interpolate_face_data_default_guard(),
            restrict_guard=self._restrict_face_data_default_guard(),
        )
        self._action_set_dynamic_amr_transfer.descend_invocation_order = 1

        # Prediction extrapolates the solution onto the time nodes and projects it to the
        # faces. Correction reads those face values and computes the next time step.
        self._action_set_correction = exahype2.solvers.aderdg.actionsets.Correction(
            solver=self,
            cell_guard=self._store_cell_data_default_guard(),
            face_guard=self._store_face_data_default_guard(),
            hanging_face_guard=self._interpolate_face_data_default_guard(),
        )
        self._action_set_correction.descend_invocation_order = 2  # Follows AMR prolongation.

        self._action_set_prediction = exahype2.solvers.aderdg.actionsets.Prediction(self, self._store_cell_data_default_guard())
        self._action_set_prediction.descend_invocation_order = 3

        self._action_set_AMR = exahype2.solvers.aderdg.actionsets.RefinementCriterion(
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
            # A time step commits the refine and erase requests and realises none of them.
            implement_previous_refinement_instructions=False,
            skip_after_topology_change=True,
        )
        self._action_set_AMR.descend_invocation_order = 4

        self._action_set_AMR_throughout_grid_construction = exahype2.solvers.aderdg.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=True,
            called_by_grid_construction=True,
            skip_after_topology_change=False,
        )
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = 4

        # A cell the mesh-refinement sweeps have just created evaluates the criterion again,
        # so one round of sweeps reaches every level the data asks for. Every other cell
        # committed its request in the preceding time step and would never stop repeating it.
        self._action_set_AMR_throughout_dynamic_amr = exahype2.solvers.aderdg.actionsets.RefinementCriterion(
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
        self._action_set_AMR_throughout_dynamic_amr.descend_invocation_order = 4

        if self._action_set_postprocess_solution is None:
            self._action_set_postprocess_solution = exahype2.solvers.aderdg.actionsets.EmptyPostprocessSolution(solver=self)
        self._action_set_postprocess_solution.descend_invocation_order = 4

        # add_actions_to_plot_solution() constructs both plotters. add_face_plotter() sets
        # the face plotter, which stays None without that call.
        self._plotter = peano4.plotter.PatchPlotter
        self._face_plotter = None
        self._face_plotter_guard = None

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
        # Receiving consumes the batch the previous sweep sent, so it starts with the first
        # sweep that follows a face-data-storing sweep. The solver state alone does not say
        # that, because it stays at GridInitialisation across every sending sweep.
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

    def current_unknown_identifier(self):
        return self._name + "Q"

    def previous_unknown_identifier(self):
        return self._name + "Q_old"

    def _plotted_solution_patch(self):
        """Returns the cell patch that holds the accepted solution at plot time.

        Fused ADER-DG corrects into ``Q`` and copies ``Q`` into ``Q_old`` in one traversal.
        With subcell limiting ``Q_old`` stays the predictor backup and ``Q`` carries the
        accepted solution.
        """
        if self.subcell_limiter_is_enabled:
            return self._current_time_step
        return self._previous_time_step

    def _plotted_solution_identifier(self):
        if self.subcell_limiter_is_enabled:
            return self.current_unknown_identifier()
        return self.previous_unknown_identifier()

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def _get_default_includes(self):
        return """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

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
        self._add_subcell_limiter_to_datamodel(datamodel)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        step.use_cell(self._previous_time_step)
        step.use_face(self._face_label)
        step.use_face(self._rhs_estimates_projection)
        step.use_face(self._flux_estimates_projection)
        self._use_subcell_limiter_data(step)

    def add_actions_to_init_grid(self, step):
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_checkpoint_loading)
        self._add_subcell_limiter_actions_to_init_grid(step)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_create_grid(self, step):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR_throughout_grid_construction)

    def add_actions_to_plot_solution(self, step):
        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        mapping = []
        for z in self._basis.quadrature_points():
            for y in self._basis.quadrature_points():
                for x in self._basis.quadrature_points():
                    mapping.append((x, y, z))

        plotter = self._plotter(
            filename=self._name,
            patch=self._plotted_solution_patch(),
            dataset_name=self._plotted_solution_identifier(),
            description=self._variable_shortcuts.plot_description,
            representation="nodal",
            mapping=mapping,
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_cell_data_default_guard(),
        )
        step.add_action_set(plotter)

        if self._face_plotter is not None:
            face_plotter = self._face_plotter(
                filename=self._name + "-face",
                patch=self._rhs_estimates_projection,
                dataset_name=self._unknown_identifier() + "Estimates",
                description=self._variable_shortcuts.plot_description,
                mapping=mapping,
                time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
                guard=self._load_face_data_default_guard() + " and " + self._face_plotter_guard,
            )
            step.add_action_set(face_plotter)

    def add_actions_to_checkpoint_solution(self, step):
        dictionary = {}
        self._init_dictionary_with_default_parameters(dictionary)
        self.add_entries_to_text_replacement_dictionary(dictionary)

        # The cell labels carry the time step size the last prediction ran
        # with, which a fused resume's replayed prediction needs. A limiting
        # resume computes its next prediction afresh and captures nothing.
        capture_arguments = {}
        if not self.subcell_limiter_is_enabled:
            capture_arguments = {
                "capture_attributes": """
  static double _capturedTimeStepSize;
""",
                "capture_statement": (
                    "_capturedTimeStepSize = fineGridCell"
                    + self._name
                    + "CellLabel.getTimeStepSize();"
                ),
                "capture_static_initialisations": """
double {{FULL_QUALIFIED_CLASSNAME}}::_capturedTimeStepSize = 0.0;
""",
            }
        writer = peano4.checkpoint.CellDataWriter(
            group_name="/solvers/" + self._name,
            output_path_expression="::runner::RuntimeParameters::getInstance().getCheckpointOutputPath()",
            # The named array holds the accepted solution of the completed
            # step: Q_old for the fused solver, whose unknowns already carry
            # the following correction's partial update by the time the
            # checkpoint sweep runs, and Q for the limiting solver.
            cell_data_name=self._plotted_solution_identifier(),
            values_per_cell_expression=(
                "BasePowerExponent<"
                + str(self._current_time_step.dim[0])
                + ", DIMENSIONS>::value * "
                + str(self._current_time_step.no_of_unknowns)
            ),
            guard=self._load_cell_data_default_guard(),
            scalars_expression=jinja2.Template(
                self._time_stepping_code_snippets.create_checkpoint_scalars()
            ).render(**dictionary),
            time_stamp_expression="repositories::getMinTimeStamp()",
            additional_includes="""
#include "repositories/SolverRepository.h"
#include "runner/RuntimeParameters.h"
""",
            **capture_arguments,
        )
        step.add_action_set(writer)
        self._add_subcell_limiter_actions_to_checkpoint_solution(step)

    def add_actions_to_dynamic_mesh_refinement(self, step):
        """Adds the action sets of the one observer that changes the topology.

        This observer realises the refine and erase requests the time step committed.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_dynamic_amr_transfer)
        self._add_subcell_limiter_actions_to_dynamic_mesh_refinement(step)
        step.add_action_set(self._action_set_AMR_throughout_dynamic_amr)

    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement)

        step.add_action_set(self._action_set_correction)
        step.add_action_set(self._action_set_prediction)

        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

        self._add_subcell_limiter_actions_to_perform_time_step(step)

    def add_implementation_files_to_project(self, namespace, output, dimensions):
        self._dimensions = dimensions
        self._namespace = namespace
        self._simd_width = int(output.makefile.d["SIMDLEN"])

        if (not self._is_linear) and (
            self._material_param_implementation != PDETerms.None_Implementation
            or self._point_sources_implementation != PDETerms.None_Implementation
        ):
            raise AssertionError(
                "Material parameters and point sources are only available for linear generated kernels."
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

        # The class name goes into a local, because a second generation run on the same
        # solver object would append it to the attribute twice.
        if self._solver_template_file_class_name is None:
            templatefile_prefix = self._templatefile_prefix + self.__class__.__name__
        else:
            templatefile_prefix = (
                self._templatefile_prefix + self._solver_template_file_class_name
            )

        d = {}
        self._init_dictionary_with_default_parameters(d)
        self.add_entries_to_text_replacement_dictionary(d)

        self._variable_shortcuts.generate_shortcut_file(d, namespace, output)

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + "Abstract.template.h",
                templatefile_prefix + "Abstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                ".",
                d,
                True,
            )
        )
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + ".template.h",
                templatefile_prefix + ".template.cpp",
                self._name,
                namespace,
                ".",
                d,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        output.makefile.add_cpp_file(self._name + ".cpp", generated=True)

        generate_kernels(solver=self, dictionary=d, output=output)

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring

    def get_solver_statistics_metadata(self):
        return [
            ("Unknowns", self._variable_shortcuts.unknowns),
            ("Auxiliary Variables", self._variable_shortcuts.auxiliary_variables),
            ("Order", self._order),
        ]

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
        """Sets the C++ code that runs before the predictor.

        The code reads ``QNew``, ``QOld``, ``marker``, ``timeStamp`` and ``timeStepSize``.
        """
        self._preprocess_reconstructed_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @property
    def postprocess_updated_patch(self):
        return self._postprocess_updated_patch

    @postprocess_updated_patch.setter
    def postprocess_updated_patch(self, kernel):
        """Sets the C++ code that runs after the corrector.

        The code reads ``QNew``, ``marker``, ``timeStamp`` and ``timeStepSize``.
        """
        self._postprocess_updated_patch = kernel
        self.create_data_structures()
        self.create_action_sets()

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def _init_dictionary_with_default_parameters(self, d):
        def render_template(template):
            return render_jinja2_template(template, d, jinja2.DebugUndefined)

        d["DIMENSIONS"]         = self._dimensions
        d["NAMESPACE"]          = self._namespace
        d["SOLVER_NAME"]        = self._name
        d["SOLVER_NUMBER"]      = self._solver_number
        d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(self._namespace) + "::" + self._name
        d["SOLVER_INSTANCE"]    = self.get_name_of_global_instance()
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()

        d["ORDER"]              = self._order
        d["NUMBER_OF_DOF"]      = self._order + 1
        d["NUMBER_OF_DOF_3D"] = 1 if self._dimensions == 2 else d["NUMBER_OF_DOF"]
        d["NUMBER_OF_UNKNOWNS"]             = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._variable_shortcuts.auxiliary_variables
        d["MEASURED_UNKNOWNS"] = getattr(self, "_error_norm_measured_unknowns", None) or (
            "{" + ", ".join(str(i) for i in range(self._variable_shortcuts.unknowns)) + "}"
        )
        d["NUMBER_OF_DATA"]                 = self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables

        alignment_helper                          = AlignmentHelper(self._simd_width)
        d["NUMBER_OF_DOF_PADDED"]                 = alignment_helper.get_size_with_padding(self._order + 1)
        d["NUMBER_OF_UNKNOWNS_PADDED"]            = alignment_helper.get_size_with_padding(self._variable_shortcuts.unknowns)
        d["NUMBER_OF_AUXILIARY_VARIABLES_PADDED"] = alignment_helper.get_size_with_padding(self._variable_shortcuts.auxiliary_variables)
        d["NUMBER_OF_DATA_PADDED"]                = alignment_helper.get_size_with_padding(self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables)

        d["CFL_SAFETY_FACTOR"]  = self._cflSafetyFactor
        d["CFL_ADER"]           = self._cflAder[self._order]

        d["SUBCELL_LIMITER"]    = self.subcell_limiter_is_enabled
        d["SUBCELL_RIEMANN_SOLVER_IMPLEMENTATION"] = (
            self._subcell_limiter_solver.riemann_solver
            if self.subcell_limiter_is_enabled
            and self._subcell_limiter_solver.riemann_solver is not None
            else PDETerms.None_Implementation
        )
        d["SUBCELL_PAC_IMPLEMENTATION"] = (
            self._subcell_limiter_physical_admissibility_criterion
            if self.subcell_limiter_is_enabled
            and self._subcell_limiter_physical_admissibility_criterion is not None
            else PDETerms.None_Implementation
        )
        d["SUBCELL_DMP_OBSERVABLES_IMPLEMENTATION"] = (
            self._subcell_limiter_dmp_observables
            if self.subcell_limiter_is_enabled
            and self._subcell_limiter_dmp_observables is not None
            else PDETerms.None_Implementation
        )
        d["SUBCELL_NUMBER_OF_DMP_OBSERVABLES"] = (
            self._subcell_limiter_number_of_dmp_observables
            if self.subcell_limiter_is_enabled
            else 0
        )
        d["SUBCELL_DMP_RELAXATION_IMPLEMENTATION"] = (
            self._subcell_limiter_dmp_relaxation_parameter_implementation
            if self.subcell_limiter_is_enabled
            and self._subcell_limiter_dmp_relaxation_parameter_implementation is not None
            else PDETerms.None_Implementation
        )

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
        if (
            self._additional_precisions is not None
            and self._additional_precisions not in d["COMPUTATION_PRECISIONS"]
        ):
            d["COMPUTATION_PRECISIONS"].append(self._additional_precisions)
        if self._corrector_computation_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._corrector_computation_precision)
        if self._precompute_picard_precision!=False and self._precompute_picard_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._precompute_picard_precision)
        if self._solution_persistent_storage_precision not in d["COMPUTATION_PRECISIONS"]:
            d["COMPUTATION_PRECISIONS"].append(self._solution_persistent_storage_precision)
        d["NUMBER_OF_PREDICTOR_PRECISIONS"] = len(d["PREDICTOR_COMPUTATION_PRECISIONS"])
        d["NUMBER_OF_PRECISIONS"] = len(d["COMPUTATION_PRECISIONS"])

        d["SOLVER_CONSTANTS"] = self._solver_constants
        d["SOLVER_INCLUDES"]  = self.get_user_solver_includes()

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
        d["START_GRID_INITIALISATION_IMPLEMENTATION"] = render_template(
            self._start_grid_initialisation_implementation
        )

        d["CONSTRUCTOR_IMPLEMENTATION"] = render_template(
            self._constructor_implementation
        )
        d["DESTRUCTOR_IMPLEMENTATION"] = render_template(
            self._destructor_implementation
        )
        d["COMPUTE_TIME_STEP_SIZE"] = render_template(self._compute_time_step_size)
        d["COMPUTE_NEW_TIME_STEP_SIZE"] = render_template(
            self._compute_new_time_step_size
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
        d["USE_DIFFUSIVE_FLUX"]       = self._use_diffusive_flux

        d["STATELESS_INITIAL_CONDITIONS"] = self._use_stateless_initial_conditions
        d["STATELESS_BOUNDARY_CONDITIONS"] = self._use_stateless_boundary_conditions
        d["STATELESS_REFINEMENT_CRITERION"] = self._use_stateless_refinement_criterion
        # The subcell scheme's kernels reach every PDE term through the solver
        # type, so a limited solver's user-defined terms are static members and
        # every call site uses the stateless form.
        def stateless_or_static_member(stateless, implementation):
            return stateless or (
                self.subcell_limiter_is_enabled
                and implementation == PDETerms.User_Defined_Implementation
            )
        d["STATELESS_FLUX"] = stateless_or_static_member(
            self._use_stateless_flux, self._flux_implementation
        )
        d["STATELESS_NCP"] = stateless_or_static_member(
            self._use_stateless_ncp, self._ncp_implementation
        )
        d["STATELESS_MAX_EIGENVALUE"] = stateless_or_static_member(
            self._use_stateless_max_eigenvalue, self._max_eigenvalue_implementation
        )
        d["STATELESS_RIEMANN_SOLVER"] = self._use_stateless_riemann_solver
        d["STATELESS_SOURCE_TERM"] = stateless_or_static_member(
            self._use_stateless_source_term, self._source_term_implementation
        )
        d["STATELESS_DIFFUSIVE_SOURCE_TERM"] = stateless_or_static_member(
            self._use_stateless_diffusive_source_term,
            self._diffusive_source_term_implementation,
        )
        d["STATELESS_POINT_SOURCE"] = self._use_stateless_point_source
        d["STATELESS_INIT_POINT_SOURCES"] = self._use_stateless_init_point_sources
        d["STATELESS_MATERIAL_PARAMETERS"] = self._use_stateless_material_parameters

        d["SPACE_TIME_PREDICTOR_CALL"] = jinja2.Template(self._space_time_predictor_call, undefined=jinja2.DebugUndefined).render(**d)
        d["RIEMANN_SOLVER_CALL"] = jinja2.Template(self._riemann_solver_call, undefined=jinja2.DebugUndefined).render(**d)
        d["FACE_INTEGRAL_CALL"] = jinja2.Template(self._face_integral_call, undefined=jinja2.DebugUndefined).render(**d)

        d["QUADRATURE_POINTS"]      = ", ".join(self._basis.quadrature_points())

    @staticmethod
    def _validate_diffusive_configuration(
        is_linear,
        use_diffusive_flux,
        diffusive_source_term,
    ):
        if is_linear and (
            use_diffusive_flux
            or diffusive_source_term != PDETerms.None_Implementation
        ):
            raise ValueError(
                "Linear ADER-DG does not support diffusive fluxes or source terms."
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
        material_parameters=None,
        number_of_point_sources=0,
        point_source=None,
        init_point_source_location=None,
        use_diffusive_flux=None,
    ):
        self._validate_diffusive_configuration(
            self._is_linear,
            self._use_diffusive_flux
            if use_diffusive_flux is None
            else use_diffusive_flux,
            self._diffusive_source_term_implementation
            if diffusive_source_term is None
            else diffusive_source_term,
        )
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
        precision                             = False,
        predictor_computation_precisions      = False,
        corrector_computation_precision       = False,
        solution_persistent_storage_precision = False,
        precompute_picard_precision           = False,
        precision_criterion_implementation    = PDETerms.User_Defined_Implementation,
        initialise_patches                    = False
    ):
        self._validate_diffusive_configuration(
            is_linear,
            self._use_diffusive_flux,
            self._diffusive_source_term_implementation,
        )
        if polynomials is Polynomials.Gauss_Legendre:
            self._basis = GaussLegendreBasis(self._order + 1)
        elif polynomials is Polynomials.Gauss_Lobatto:
            self._basis = GaussLobattoBasis(self._order + 1)
        else:
            raise Exception(
                "No proper basis chosen: {}. Valid options are Gauss_Legendre and Gauss_Lobatto nodes.".format(
                    polynomials
                )
            )

        self._is_linear = is_linear
        self._polynomials = polynomials

        if precision != False:
            if precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision is not valid. Valid choices are: "
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
                    "The chosen precision is not valid. Valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._solution_persistent_storage_precision = PrecisionType[solution_persistent_storage_precision]

        if corrector_computation_precision != False:
            if corrector_computation_precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision is not valid. Valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._corrector_computation_precision = PrecisionType[corrector_computation_precision]

        if precompute_picard_precision != False:
            if precompute_picard_precision not in PrecisionType.keys():
                raise AssertionError(
                    "The chosen precision is not valid. Valid choices are: "
                    + ", ".join(PrecisionType.keys())
                )
            self._precompute_picard_precision = PrecisionType[precompute_picard_precision]

        if predictor_computation_precisions != False:
            self._predictor_computation_precisions = []
            for predictor_precision in predictor_computation_precisions:
                if predictor_precision not in PrecisionType.keys():
                    raise AssertionError(
                        "The chosen precision is not valid. Valid choices are: "
                        + ", ".join(PrecisionType.keys())
                    )
                self._predictor_computation_precisions.append(
                    PrecisionType[predictor_precision]
                )

        if len(self._predictor_computation_precisions) > 1:
            self._precision_criterion_implementation = precision_criterion_implementation

        self._initialise_patches = initialise_patches

        self.create_data_structures()
        self.create_action_sets()

    def add_face_plotter(self, guard="true"):
        """Adds a plotter that writes the projected face estimates to ``<name>-face``."""
        self._face_plotter = peano4.plotter.FacePlotter
        self._face_plotter_guard = guard


def get_face_overlap_merge_implementation(patch_overlap):
    template = """
  constexpr int SpaceFaceSize = Cardinality / 2;

  auto* dst = this->data();
  const auto* const src = neighbour.data();
  assertion(dst);
  assertion(src);

  const int faceDimension = marker.getSelectedFaceNumber() % DIMENSIONS;
  const int faceLR = (marker.outerNormal()[faceDimension] > 0.0 ? 0 : 1);

  std::copy_n(
    &src[(1 - faceLR) * SpaceFaceSize],
    SpaceFaceSize,
    &dst[(1 - faceLR) * SpaceFaceSize]
  );
"""
    return template
