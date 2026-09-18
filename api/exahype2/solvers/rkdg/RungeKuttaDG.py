# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.plotter
import peano4.datamodel
import exahype2.blockstructured
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

from exahype2.solvers.ButcherTableau import RungeKutta_steps

from exahype2.solvers.PDETerms import PDETerms
from exahype2.VariableShortcuts import VariableShortcuts

import jinja2

from abc import abstractmethod

import exahype2
import exahype2.solvers.rkdg.actionsets

from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import (
    FaceProjections,
)
from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import (
    compute_number_of_face_projection_quantities,
)

from peano4.Logger import log
from peano4.output.Helper import render_jinja2_template


class RungeKuttaDG(object):
    r"""!
    The base class of every Runge-Kutta discontinuous Galerkin solver.

    A subclass sets the guard of each data structure and each action set, and
    it sets `_compute_time_step_size` and `_compute_new_time_step_size`. A
    subclass calls this class' create_data_structures() and
    create_action_sets() before it alters what they set up.

    A face label carries three time stamps: The update stamp advances in every
    Runge-Kutta step, the new stamp belongs to the face's EstimateProjection
    after a time step, and the old stamp belongs to the old face projection.
    """

    def __init__(
        self,
        name,
        rk_order,
        polynomial_basis,
        face_projections: FaceProjections,
        unknowns,
        auxiliary_variables,
        pde_terms_without_state: bool,
        baseline_action_set_descend_invocation_order=0,
    ):
        """
        Constructs a Runge-Kutta discontinuous Galerkin solver.

        `name` is unique per solver and enters every generated class name.
        `polynomial_basis` fixes the spatial order, and `face_projections`
        selects the quantities the cell projects onto a face. `unknowns` and
        `auxiliary_variables` count entries per degree of freedom; only the
        unknowns follow a hyperbolic formulation.
        """
        assert rk_order >= 1, "The Runge-Kutta order must be at least one."

        self._name = name
        self._solver_number = 0

        self._rk_order = rk_order
        self._basis = polynomial_basis
        self._face_projections = face_projections

        self._volumetric_compute_kernel_call = "#error Please set the solver property _volumetric_compute_kernel_call in the Python class"
        self._volumetric_compute_kernel_call_stateless = "#error Please set the solver property _volumetric_compute_kernel_call_stateless in the Python class"
        self._Riemann_compute_kernel_call = (
            "#error Please set the solver property _Riemann_compute_kernel_call in the Python class"
        )
        self._Riemann_compute_kernel_call_stateless = (
            "#error Please set the solver property _Riemann_compute_kernel_call_stateless in the Python class"
        )
        self._pde_terms_without_state = pde_terms_without_state
        self._add_solver_contributions_call = "#error Please set the solver property _add_solver_contributions_call in the Python class"
        self._multiply_with_inverted_mass_matrix_call = "#error Please set the solver property _multiply_with_inverted_mass_matrix_call in the Python class"
        self._kernel_namespace = "#error Please set the solver property _kernel_namespace in the Python class"

        self._solver_constants = ""
        self._user_action_set_includes = ""
        self._user_solver_includes = ""

        self._plotter = peano4.plotter.PatchPlotter
        self._variable_shortcuts = VariableShortcuts(name, unknowns, auxiliary_variables)

        self._solver_template_file_class_name = None

        self._action_set_postprocess_solution = None
        self._action_set_preprocess_solution = None

        self._compute_eigenvalue = False

        self._compute_new_time_step_size = None

        self._preprocess_cell = ""
        self._postprocess_updated_cell_after_Runge_Kutta_step = ""
        self._postprocess_updated_cell_after_final_linear_combination = ""

        self._flux_implementation = None
        self._ncp_implementation = None
        self._eigenvalues_implementation = None
        self._source_term_implementation = None
        self._point_sources_implementation = None

        self._boundary_conditions_implementation = PDETerms.User_Defined_Implementation
        self._refinement_criterion_implementation = PDETerms.Empty_Implementation
        self._initial_conditions_implementation = PDETerms.User_Defined_Implementation

        self._compute_time_step_size = "#error Not yet defined"

        self._abstract_solver_user_declarations = ""
        self._abstract_solver_user_definitions = ""
        self._solver_user_declarations = ""
        self._solver_user_definitions = ""

        self._start_time_step_implementation = ""
        self._finish_time_step_implementation = ""

        self._constructor_implementation = ""

        self._start_simulation_implementation = ""
        self._finish_simulation_implementation = ""

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

Grid resolution is selected at runtime.

We use RK("""
            + str(self._rk_order)
            + """) for the time stepping.

"""
        )

    @property
    def user_action_set_includes(self):
        return self._user_action_set_includes

    @property
    def user_solver_includes(self):
        return self._user_solver_includes

    def add_user_action_set_includes(self, value):
        self._user_action_set_includes += value

    def add_user_solver_includes(self, value):
        self._user_solver_includes += value

    @abstractmethod
    def create_data_structures(self):
        r"""
        Sets up all data structures tied to the grid entities.

        Only the last of repeated calls matters, and a subclass calls this
        version first. The Runge-Kutta step count enters the first argument of
        _rhs_estimates, since a triple is truncated to a tuple in two
        dimensions. _linear_combination_of_estimates stays on the cell until
        the secondary sweep of the same step has read it. A face travels
        between two trees or two domain partitions only when it is stored
        persistently and its exchange flag is set as well.
        """
        self._current_time_step = peano4.datamodel.Patch(
            (
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
            ),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier(),
        )
        self._rhs_estimates = peano4.datamodel.Patch(
            (
                self.number_of_Runge_Kutta_steps() * (self._basis.dofs_per_axis),
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
            ),
            self._variable_shortcuts.unknowns,
            self._unknown_identifier() + "RhsEstimates",
        )
        self._linear_combination_of_estimates = peano4.datamodel.Patch(
            (
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
            ),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier() + "LinearCombination",
        )
        self._estimate_projection = peano4.datamodel.Patch(
            (
                compute_number_of_face_projection_quantities(self._face_projections)
                * 2,
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
            ),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier() + "EstimateProjection",
        )
        self._old_solution_projection = peano4.datamodel.Patch(
            (
                compute_number_of_face_projection_quantities(self._face_projections)
                * 2,
                self._basis.dofs_per_axis,
                self._basis.dofs_per_axis,
            ),
            self._variable_shortcuts.unknowns + self._variable_shortcuts.auxiliary_variables,
            self._unknown_identifier() + "OldSolutionProjection",
        )
        self._Riemann_solution = peano4.datamodel.Patch(
            (2, self._basis.dofs_per_axis, self._basis.dofs_per_axis),
            self._variable_shortcuts.unknowns,
            self._unknown_identifier() + "RiemannSolution",
        )

        self._current_time_step.generator = peano4.datamodel.PatchToData(
            self._current_time_step, "double"
        )
        self._rhs_estimates.generator = peano4.datamodel.PatchToData(
            self._rhs_estimates, "double"
        )
        self._linear_combination_of_estimates.generator = peano4.datamodel.PatchToData(
            self._linear_combination_of_estimates, "double"
        )
        self._estimate_projection.generator = peano4.datamodel.PatchToData(
            self._estimate_projection, "double"
        )
        self._old_solution_projection.generator = peano4.datamodel.PatchToData(
            self._old_solution_projection, "double"
        )
        self._Riemann_solution.generator = peano4.datamodel.PatchToData(
            self._Riemann_solution, "double"
        )

        self._estimate_projection.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._old_solution_projection.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._current_time_step.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._rhs_estimates.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""

        self._linear_combination_of_estimates.generator.includes += """
#include "peano4/Loop.h"
#include "repositories/SolverRepository.h"
"""
        self._estimate_projection.generator.send_condition              = "false"
        self._estimate_projection.generator.receive_and_merge_condition = "false"
        self._estimate_projection.generator.can_receive      = self._load_face_data_default_guard()
        self._estimate_projection.generator.can_send         = self._store_face_data_default_guard()
        self._estimate_projection.generator.halo_layout      = "overlapping_volumes"

        self._old_solution_projection.generator.send_condition              = "false"
        self._old_solution_projection.generator.receive_and_merge_condition = "false"
        self._old_solution_projection.generator.can_receive      = self._load_face_data_default_guard()
        self._old_solution_projection.generator.can_send         = self._store_face_data_default_guard()

        self._current_time_step.generator.can_receive  = self._load_cell_data_default_guard()
        self._current_time_step.generator.can_send     = self._store_cell_data_default_guard()

        self._rhs_estimates.generator.can_receive      = self._load_cell_data_default_guard()
        self._rhs_estimates.generator.can_send         = self._store_cell_data_default_guard()
        self._rhs_estimates.generator.send_condition              = "false"
        self._rhs_estimates.generator.receive_and_merge_condition = "false"

        self._linear_combination_of_estimates.generator.can_receive = self._load_cell_data_default_guard()
        self._linear_combination_of_estimates.generator.can_send = self._store_cell_data_default_guard()

        self._Riemann_solution.generator.can_receive       = self._load_face_data_default_guard()
        self._Riemann_solution.generator.can_send          = self._store_face_data_default_guard()
        self._Riemann_solution.generator.send_condition               = "false"
        self._Riemann_solution.generator.receive_and_merge_condition  = "false"

        self._Riemann_solution.generator.includes += """
#include "repositories/SolverRepository.h"
"""

        self._cell_label = exahype2.grid.create_cell_label(self._name)
        self._face_label = exahype2.grid.create_face_label(self._name)

    def _query_fine_grid_cell_in_action_set_if_it_holds_solution(self):
        return self._store_cell_data_default_guard()

    @abstractmethod
    def create_action_sets(self):
        """
        Creates the action sets of the solver.

        Only the last of repeated calls matters, and a subclass calls this
        version first.
        """
        self._action_set_initial_conditions = (
            exahype2.solvers.rkdg.actionsets.InitialCondition(
                self,
                self._query_fine_grid_cell_in_action_set_if_it_holds_solution(),
            )
        )
        self._action_set_AMR = exahype2.solvers.rkdg.actionsets.RefinementCriterion(
            solver=self,
            guard=self._store_cell_data_default_guard()
            + " and (repositories::"
            + self.get_name_of_global_instance()
            + ".isLastGridSweepOfTimeStep() or repositories::"
            + self.get_name_of_global_instance()
            + ".getSolverState()=="
            + self._name
            + "::SolverState::GridInitialisation)",
            build_up_new_refinement_instructions=True,
            # A time step never realises topology changes; it only commits the
            # refine/coarsen requests. Realisation happens in the DynamicAMR
            # observer, whose DynamicAMRTransfer delivers the committed events.
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=True,
        )
        self._action_set_AMR_throughout_grid_construction = (
            exahype2.solvers.rkdg.actionsets.RefinementCriterion(
                solver=self,
                guard=self._store_cell_data_default_guard(),
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
                skip_after_topology_change=False,
            )
        )
        # Only the cells the realise sweeps created are evaluated here. Every other
        # cell committed its request in the preceding time step, and re-issuing that
        # request each sweep would keep the sweeps running forever.
        self._action_set_AMR_throughout_dynamic_amr = (
            exahype2.solvers.rkdg.actionsets.RefinementCriterion(
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
        )
        self._action_set_linear_combination_of_estimates = (
            exahype2.solvers.rkdg.actionsets.LinearCombinationOfEstimates(self)
        )
        # @todo TW The following two action sets should run in parallel.
        self._action_set_project_linear_combination_onto_faces = exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces(
            solver=self, face_projections=self._face_projections
        )
        self._action_set_solve_volume_integral = (
            exahype2.solvers.rkdg.actionsets.SolveVolumeIntegral(self)
        )
        self._action_set_apply_boundary_conditions = (
            exahype2.solvers.rkdg.actionsets.ApplyBoundaryConditions(
                self, self._store_face_data_default_guard()
            )
        )
        self._action_set_solve_Riemann_problem = (
            exahype2.solvers.rkdg.actionsets.SolveRiemannProblem(self)
        )
        self._action_set_interpolate_Riemann_solution_onto_hanging_faces = (
            exahype2.solvers.rkdg.actionsets.InterpolateRiemannSolutionOntoHangingFaces(self)
        )
        self._action_set_add_volume_and_face_solution = (
            exahype2.solvers.rkdg.actionsets.AddVolumeAndFaceSolution(self)
        )
        self._action_set_compute_final_linear_combination_and_project_solution_onto_faces = exahype2.solvers.rkdg.actionsets.ComputeFinalLinearCombination(
            solver=self,
            guard=self._store_cell_data_default_guard(),
            face_projections=self._face_projections,
        )
        self._action_set_adaptive_mesh_face_coupling = (
            exahype2.solvers.rkdg.actionsets.AdaptiveMeshFaceCoupling(
                solver=self, face_projections=self._face_projections
            )
        )
        self._action_set_dynamic_amr_transfer = exahype2.solvers.rkdg.actionsets.DynamicAMRTransfer(
            solver=self, face_projections=self._face_projections
        )

        if self._action_set_postprocess_solution is None:
            self._action_set_postprocess_solution = (
                exahype2.solvers.rkdg.actionsets.EmptyPostprocessSolution(solver=self)
            )
        if self._action_set_preprocess_solution is None:
            self._action_set_preprocess_solution = (
                exahype2.solvers.rkdg.actionsets.EmptyPreprocessSolution(solver=self)
            )

        self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel(self._name)
        self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel(self._name)

        self._action_set_update_face_label.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_update_cell_label.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )
        self._action_set_linear_combination_of_estimates.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_adaptive_mesh_face_coupling.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        # UpdateCellLabel resets requiresCriterionEvaluation and UpdateFaceLabel zeroes the
        # face time stamps, so the transfer has to descend behind both to keep the
        # criterion-evaluation flag and the stamps it inherits.
        self._action_set_dynamic_amr_transfer.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_project_linear_combination_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 3
        )  # Depends on the linear combination of estimates
        self._action_set_solve_volume_integral.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 3
        )  # The two 3s could run in parallel
        self._action_set_apply_boundary_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )
        self._action_set_solve_Riemann_problem.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 3
        )  # Boundary has to be known
        self._action_set_interpolate_Riemann_solution_onto_hanging_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 2
        )  # Before the face integrals pick up the Riemann solutions
        self._action_set_add_volume_and_face_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 3
        )  # Can also run in parallel, as it never occurs at the same time
        self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 4
        )  # Depends on the preceding action sets
        self._action_set_AMR.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 5
        self._action_set_AMR_throughout_grid_construction.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 5
        self._action_set_AMR_throughout_dynamic_amr.descend_invocation_order = self._baseline_action_set_descend_invocation_order + 5

        self._action_set_initial_conditions.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 1
        )

        self._action_set_preprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )  # Touch cell first time, i.e. enter cell
        self._action_set_postprocess_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )  # Touch cell last time, i.e. leave cell

    def _provide_cell_data_to_compute_kernels_default_guard(self):
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

    def _store_cell_data_default_guard(self):
        """!
        Extend the guard via ands only.

        Never use an or, as subclasses might extend the guard as well and will
        append further ands.
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
        Extend the guard via ands only.

        Never use an or, as subclasses might extend the guard as well and will
        append further ands.
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
        Extend the guard via ands only.

        Never use an or, as subclasses might extend the guard as well and will
        append further ands.
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
        Extend the guard via ands only.

        Never use an or, as subclasses might extend the guard as well and will
        append further ands.

        Receiving consumes the batch the previous sweep sent, so it is enabled
        from the first sweep that follows a face-data-storing sweep on. A guard
        keyed on the solver state alone cannot express that: the state stays at
        GridInitialisation across the initialise-drain loop, whose every sweep
        sends.
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

    def get_name_of_global_instance(self):
        return "instanceOf" + self._name

    def add_to_Peano4_datamodel(self, datamodel):
        log.debug("Polynomial basis")
        log.debug("----------")
        log.debug(str(self._basis))
        log.debug("Face projections")
        log.debug("----------")
        log.debug(str(self._face_projections))
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._current_time_step)
        datamodel.add_cell(self._rhs_estimates)
        datamodel.add_cell(self._linear_combination_of_estimates)
        datamodel.add_face(self._face_label)
        datamodel.add_face(self._estimate_projection)
        datamodel.add_face(self._old_solution_projection)
        datamodel.add_face(self._Riemann_solution)

    def add_use_data_statements_to_Peano4_solver_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        step.use_cell(self._rhs_estimates)
        step.use_cell(self._linear_combination_of_estimates)
        step.use_face(self._face_label)
        step.use_face(self._estimate_projection)
        step.use_face(self._old_solution_projection)
        step.use_face(self._Riemann_solution)

    def _get_default_includes(self):
        return """
#include "tarch/la/Vector.h"

#include "peano4/Loop.h"

#include "repositories/SolverRepository.h"
"""

    def add_actions_to_init_grid(self, step):
        """!
        Adds the action sets to the grid initialisation.

        The initial condition runs before anything reads the solution, and the
        labels are current before the refinement criterion judges a cell. The
        traversal keeps this order while it descends and inverts it while it
        ascends, so an action set that acts on the way up is added early.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_initial_conditions)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_create_grid(self, step):
        """
        @todo The boundary information is set only once, so the face label has
        to be used and initialised properly.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_AMR_throughout_grid_construction)

    def add_actions_to_plot_solution(self, step):
        mapping = []
        for z in self._basis.quadrature_points:
            for y in self._basis.quadrature_points:
                for x in self._basis.quadrature_points:
                    mapping.append((x, y, z))

        plotter = self._plotter(
            filename=self._name,
            patch=self._current_time_step,
            dataset_name=self._unknown_identifier(),
            description=self._variable_shortcuts.plot_description,
            representation="nodal",
            mapping=mapping,
            time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
            guard=self._load_cell_data_default_guard(),
        )
        plotter.descend_invocation_order = self._baseline_action_set_descend_invocation_order
        step.add_action_set(plotter)

    def add_actions_to_perform_time_step(self, step):
        """
        Adds the action sets of one Runge-Kutta sweep in their required order.

        The linear combination opens the step, the resolution coupling reads
        it, the boundary conditions run after the inter-grid transfers, the
        Riemann solve after the boundary conditions, and the interpolation onto
        the hanging faces after the Riemann solve.
        """
        step.add_action_set(self._action_set_preprocess_solution)
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_linear_combination_of_estimates)
        step.add_action_set(
            self._action_set_adaptive_mesh_face_coupling
        )
        step.add_action_set(self._action_set_project_linear_combination_onto_faces)
        step.add_action_set(self._action_set_solve_volume_integral)
        step.add_action_set(self._action_set_apply_boundary_conditions)
        step.add_action_set(self._action_set_solve_Riemann_problem)
        step.add_action_set(self._action_set_interpolate_Riemann_solution_onto_hanging_faces)
        step.add_action_set(self._action_set_add_volume_and_face_solution)
        step.add_action_set(
            self._action_set_compute_final_linear_combination_and_project_solution_onto_faces
        )
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)

    def add_actions_to_dynamic_mesh_refinement(self, step):
        """!
        Adds the action sets to the dedicated DynamicAMR observer.

        This observer alone changes the topology, and it transfers the nodal
        solution across every resolution transition Peano creates or destroys.
        The next time step rebuilds the Runge-Kutta estimates and the face
        projections on the settled grid.
        """
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(self._action_set_dynamic_amr_transfer)
        step.add_action_set(self._action_set_AMR_throughout_dynamic_amr)

    @abstractmethod
    def add_entries_to_text_replacement_dictionary(self, d):
        pass

    def add_implementation_files_to_project(self, namespace, output, dimensions):
        """
        Adds the generated solver files to `output`, a peano4.output.Output.
        """
        templatefile_prefix = os.path.dirname(os.path.realpath(__file__)) + "/"

        if self._solver_template_file_class_name is None:
            templatefile_prefix += self.__class__.__name__
        else:
            templatefile_prefix += self._solver_template_file_class_name

        # TODO: Combine to one dictionary
        abstractHeaderDictionary = {}
        implementationDictionary = {}
        self._init_dictionary_with_default_parameters(abstractHeaderDictionary)
        self._init_dictionary_with_default_parameters(implementationDictionary)
        self.add_entries_to_text_replacement_dictionary(abstractHeaderDictionary)
        self.add_entries_to_text_replacement_dictionary(implementationDictionary)

        self._variable_shortcuts.generate_shortcut_file(abstractHeaderDictionary, namespace, output)

        generated_abstract_header_file = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + "Abstract.template.h",
                templatefile_prefix + "Abstract.template.cpp",
                "Abstract" + self._name,
                namespace,
                ".",
                abstractHeaderDictionary,
                True,
            )
        )
        hasSolverDefinitions = bool(
            implementationDictionary["SOLVER_USER_DEFINITIONS"].strip()
        ) or PDETerms.User_Defined_Implementation in (
            implementationDictionary["REFINEMENT_CRITERION_IMPLEMENTATION"],
            implementationDictionary["INITIAL_CONDITIONS_IMPLEMENTATION"],
            implementationDictionary["BOUNDARY_CONDITIONS_IMPLEMENTATION"],
        )
        solverImplementationTemplate = templatefile_prefix + ".template.cpp" if hasSolverDefinitions else None
        generated_solver_files = (
            peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
                templatefile_prefix + ".template.h",
                solverImplementationTemplate,
                self._name,
                namespace,
                ".",
                implementationDictionary,
                True,
            )
        )

        output.add(generated_abstract_header_file)
        output.add(generated_solver_files)
        output.makefile.add_h_file("Abstract" + self._name + ".h", generated=True)
        output.makefile.add_h_file(self._name + ".h", generated=True)
        output.makefile.add_cpp_file("Abstract" + self._name + ".cpp", generated=True)
        if hasSolverDefinitions:
            output.makefile.add_cpp_file(self._name + ".cpp", generated=True)

    def set_solver_constants(self, datastring):
        self._solver_constants = datastring

    def add_solver_constants(self, datastring):
        self._solver_constants += datastring

    def _init_dictionary_with_default_parameters(self, d):
        """
        Fills the text replacement dictionary `d` with the default entries.

        Every algorithmic step calls this before
        add_entries_to_text_replacement_dictionary().
        """

        def render_template(template):
            return render_jinja2_template(template, d, jinja2.DebugUndefined)

        d["DG_ORDER"] = self._basis.order
        d["RK_ORDER"] = self._rk_order
        d["RK_STEPS"] = self.number_of_Runge_Kutta_steps()

        d["SOLVER_INSTANCE"] = self.get_name_of_global_instance()
        d["SOLVER_NAME"] = self._name
        d["SOLVER_NUMBER"] = self._solver_number
        d["UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["NUMBER_OF_UNKNOWNS"] = self._variable_shortcuts.unknowns
        d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._variable_shortcuts.auxiliary_variables
        d["MEASURED_UNKNOWNS"] = getattr(self, "_error_norm_measured_unknowns", None) or (
            "{" + ", ".join(str(i) for i in range(self._variable_shortcuts.unknowns)) + "}"
        )

        d["NUMBER_OF_DOFS_PER_CELL_2D"] = (self._basis.dofs_per_axis) * (
            self._basis.dofs_per_axis
        )
        d["NUMBER_OF_DOFS_PER_CELL_3D"] = (
            (self._basis.dofs_per_axis)
            * (self._basis.dofs_per_axis)
            * (self._basis.dofs_per_axis)
        )

        d["NUMBER_OF_DOFS_PER_FACE_2D"] = self._basis.dofs_per_axis
        d["NUMBER_OF_DOFS_PER_FACE_3D"] = (self._basis.dofs_per_axis) * (
            self._basis.dofs_per_axis
        )

        d["SOLVER_CONSTANTS"] = self._solver_constants

        d["SOLVER_INCLUDES"] = self.user_solver_includes

        d["PREPROCESS_RECONSTRUCTED_CELL"] = render_template(self._preprocess_cell)
        d["POSTPROCESS_UPDATED_CELL_AFTER_RUNGE_KUTTA_STEP"] = render_template(
            self._postprocess_updated_cell_after_Runge_Kutta_step
        )
        d["POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION"] = render_template(
            self._postprocess_updated_cell_after_final_linear_combination
        )
        d[
            "BOUNDARY_CONDITIONS_IMPLEMENTATION"
        ] = self._boundary_conditions_implementation
        d[
            "REFINEMENT_CRITERION_IMPLEMENTATION"
        ] = self._refinement_criterion_implementation
        d["INITIAL_CONDITIONS_IMPLEMENTATION"] = self._initial_conditions_implementation
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

        d["FLUX_IMPLEMENTATION"] = self._flux_implementation
        d["NCP_IMPLEMENTATION"] = self._ncp_implementation
        d["SOURCE_TERM_IMPLEMENTATION"] = self._source_term_implementation
        d["POINT_SOURCES_IMPLEMENTATION"] = self._point_sources_implementation

        d["COMPUTE_MAX_EIGENVALUE"] = self._compute_eigenvalue

        d["STATELESS_PDE_TERMS"] = self._pde_terms_without_state
        d["KERNEL_NAMESPACE"] = self._kernel_namespace

        self._basis.init_dictionary_with_default_parameters(d, False)

        # Has to come last, as we already use the other entries
        d["VOLUMETRIC_COMPUTE_KERNEL_CALL"] = render_template(
            self._volumetric_compute_kernel_call
        )
        d["VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS"] = render_template(
            self._volumetric_compute_kernel_call_stateless
        )
        d["RIEMANN_COMPUTE_KERNEL_CALL"] = render_template(
            self._Riemann_compute_kernel_call
        )
        d["RIEMANN_COMPUTE_KERNEL_CALL_STATELESS"] = render_template(
            self._Riemann_compute_kernel_call_stateless
        )
        d["ADD_SOLVER_CONTRIBUTIONS_CALL"] = render_template(
            self._add_solver_contributions_call
        )
        d["MULTIPLY_WITH_INVERTED_MASS_MATRIX_CALL"] = render_template(
            self._multiply_with_inverted_mass_matrix_call
        )

    def get_solver_statistics_metadata(self):
        return [
            ("Unknowns", self._variable_shortcuts.unknowns),
            ("Auxiliary Variables", self._variable_shortcuts.auxiliary_variables),
            ("DG-Order", self._basis.order),
            ("RK-Order", self._rk_order),
        ]

    @property
    def unknowns(self):
        return self._variable_shortcuts.unknowns

    @property
    def auxiliary_variables(self):
        return self._variable_shortcuts.auxiliary_variables

    def number_of_Runge_Kutta_steps(self):
        return RungeKutta_steps(self._rk_order)

    def set_implementation(
        self,
        flux=None,
        ncp=None,
        eigenvalues=None,
        boundary_conditions=None,
        refinement_criterion=None,
        initial_conditions=None,
        source_term=None,
        point_source=None,
        additional_action_set_includes="",
        additional_user_includes="",
    ):
        """
        Sets the implementation of the PDE terms and the solver callbacks.

        User_Defined creates a C++ stub, None_Implementation creates an empty
        body, any other string is copied into the implementation, and None
        keeps the value the solver holds.
        """
        if boundary_conditions is not None:
            self._boundary_conditions_implementation = boundary_conditions
        if refinement_criterion is not None:
            self._refinement_criterion_implementation = refinement_criterion
        if initial_conditions is not None:
            self._initial_conditions_implementation = initial_conditions

        if refinement_criterion == exahype2.solvers.PDETerms.None_Implementation:
            raise ValueError(
                "The refinement criterion must not be None_Implementation."
            )
        if initial_conditions == exahype2.solvers.PDETerms.None_Implementation:
            raise ValueError(
                "The initial conditions must not be None_Implementation."
            )

        if flux is not None:
            self._flux_implementation = flux
        if ncp is not None:
            self._ncp_implementation = ncp
        if eigenvalues is not None:
            self._eigenvalues_implementation = eigenvalues
        if source_term is not None:
            self._source_term_implementation = source_term
        if point_source is not None:
            self._point_sources_implementation = point_source

        self.add_user_action_set_includes(additional_action_set_includes)
        self.add_user_solver_includes(additional_user_includes)

        self.create_data_structures()
        self.create_action_sets()

    @property
    def postprocess_updated_cell_after_Runge_Kutta_step(self):
        return self._postprocess_updated_cell_after_Runge_Kutta_step

    @property
    def postprocess_updated_cell_after_final_linear_combination(self):
        return self._postprocess_updated_cell_after_final_linear_combination

    @postprocess_updated_cell_after_Runge_Kutta_step.setter
    def postprocess_updated_cell_after_Runge_Kutta_step(self, kernel):
        """
        Sets C++ code that runs after each Runge-Kutta step.

        The snippet reads the time derivative `dQdt` of the whole cell together
        with `timeStamp` and `timeStepSize`. `{{DG_ORDER}}` and
        `{{NUMBER_OF_UNKNOWNS}}` are available as template constants.
        """
        self._postprocess_updated_cell_after_Runge_Kutta_step = kernel
        self.create_data_structures()
        self.create_action_sets()

    @postprocess_updated_cell_after_final_linear_combination.setter
    def postprocess_updated_cell_after_final_linear_combination(self, kernel):
        """Sets C++ code that runs after the final Runge-Kutta combination.

        The snippet updates the full nodal state through `newQ` and reads
        `timeStamp` and `timeStepSize`. It runs once per completed time step,
        before the final state is projected onto the faces.
        """
        self._postprocess_updated_cell_after_final_linear_combination = kernel
        self.create_data_structures()
        self.create_action_sets()
