# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import jinja2

import exahype2.actionsets
import exahype2.datamodel
import exahype2.dg.rk.actionsets
import exahype2.pde_terms
import exahype2.plotter
import exahype2.solver

from exahype2.codegen.helper import render_jinja2_template
from exahype2.dg.basis import GaussLegendreBasis
from exahype2.dg.basis import GaussLobattoBasis
from exahype2.layout import Layout
from exahype2.layout import node_loop

from .butcher_tableau import number_of_stages
from .generate_kernels import generate_kernels


def _pde_terms():
    return {
        "initial_condition": exahype2.pde_terms.initial_condition(),
        "boundary_conditions": exahype2.pde_terms.boundary_conditions("compute"),
        "refinement_criterion": exahype2.pde_terms.refinement_criterion(
            "[(DGOrder + 1)^DIMENSIONS * ({N} + {A})]"
        ),
        "max_eigenvalue": exahype2.pde_terms.max_eigenvalue(),
        "flux": exahype2.pde_terms.flux(),
        "diffusive_flux": exahype2.pde_terms.diffusive_flux(),
        "ncp": exahype2.pde_terms.ncp(),
        "source": exahype2.pde_terms.source(),
        "riemann_solver": exahype2.pde_terms.galerkin_riemann_solver(),
        "analytical_solution": exahype2.pde_terms.analytical_solution(),
    }


_DESCEND_INVOCATION_ORDER = {
    "update_face_label": 1,
    "update_cell_label": 1,
    "initial_conditions": 1,
    "adaptive_mesh_face_coupling": 2,
    "interpolate_riemann_solution_onto_hanging_faces": 2,
    "dynamic_amr_transfer": 2,
    "update_solution": 3,
    "amr": 5,
    "amr_throughout_grid_construction": 5,
    "amr_throughout_dynamic_amr": 5,
}


class Solver(exahype2.solver.Solver):
    """A Runge-Kutta DG solver of polynomial order ``dg_order`` with one grid sweep per
    Runge-Kutta stage of order ``rk_order``.

    ``basis`` is :class:`exahype2.dg.GaussLegendre` or :class:`exahype2.dg.GaussLobatto`;
    ``linear`` selects the linear volume integral and Riemann solver.
    """

    FAMILY = "rk"
    FAMILY_SOLVER_INCLUDE = "exahype2/dg/rk/Solver.h"
    FAMILY_SOLVER_CONCEPT = "::exahype2::dg::rk::SolverConcept"
    FAMILY_SOLVER_CONCEPT_PRECISIONS = ("STORAGE_PRECISION", "COMPUTE_PRECISION")
    PDE_TERMS = _pde_terms()

    def __init__(
        self,
        name,
        rk_order,
        dg_order,
        unknowns,
        auxiliary_variables,
        time_stepping,
        basis=GaussLegendreBasis,
        linear=False,
        baseline_action_set_descend_invocation_order=0,
    ):
        super().__init__(name, unknowns, auxiliary_variables, time_stepping)
        if basis not in (GaussLegendreBasis, GaussLobattoBasis):
            raise ValueError(
                "basis is exahype2.dg.GaussLegendre or exahype2.dg.GaussLobatto."
            )
        self._rk_order = rk_order
        self._dg_order = dg_order
        self._basis = basis(dg_order + 1)
        self._is_linear = linear
        self._baseline_action_set_descend_invocation_order = (
            baseline_action_set_descend_invocation_order
        )

        self._plotter = exahype2.plotter.PatchPlotter
        self._postprocess_updated_cell_after_runge_kutta_step = ""
        self._postprocess_updated_cell_after_final_linear_combination = ""

    @property
    def rk_order(self):
        return self._rk_order

    @property
    def basis(self):
        return self._basis

    @property
    def number_of_stages(self):
        return number_of_stages(self._rk_order)

    def statistics_metadata(self):
        return super().statistics_metadata() + [
            ("DG-Order", self._dg_order),
            ("RK-Order", self._rk_order),
        ]

    def _runge_kutta_sweep_guard(self):
        return "(" + self._sweep_is("isRungeKuttaSweep") + ")"

    def _solver_sweep_guard(self):
        return (
            "("
            + self._runge_kutta_sweep_guard()
            + " or "
            + self._sweep_is("isTimeStepAfterGridInitialisationSweep")
            + ")"
        )

    def _projection_exchange_guard(self):
        return (
            "("
            + self._runge_kutta_sweep_guard()
            + " or "
            + self._sweep_is("isDynamicMeshRefinementSweep")
            + " or "
            + self._sweep_is("isPlottingSweep")
            + ")"
        )

    def _create_layout(self):
        quadrature_points = [float(point) for point in self._basis.quadrature_points]
        return Layout(
            dofs_per_axis=self._basis.dofs_per_axis,
            representation="nodal",
            loop=node_loop(),
            solutions={
                "current": self._unknown_identifier(),
                "previous": self._unknown_identifier(),
            },
            includes=["exahype2/dg/DGUtils.h", "exahype2/dg/ForEachNode.h"],
            tracer_include="exahype2/dg/Tracer.h",
            tracer_arguments=(
                "marker,\n"
                "{{DG_ORDER}},\n"
                "kernels::{{SOLVER_NAME}}::Quadrature<double>::QuadraturePoints1D,\n"
                "{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},\n"
                "fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),\n"
                "*particle"
            ),
            plot_mapping=[
                (x, y, z)
                for z in quadrature_points
                for y in quadrature_points
                for x in quadrature_points
            ],
        )

    def _riemann_solve_guard(self):
        return (
            "("
            + self._sweep_is("isDynamicMeshRefinementSweep")
            + " or ("
            + self._runge_kutta_sweep_guard()
            + " and not repositories::"
            + self.instance_name
            + ".usesDynamicMeshRefinementSweeps()))"
        )

    def create_data_structures(self):
        """A face patch holds all nodes of the left trace as its first contiguous half, followed
        by the right trace; the estimates of every stage lie one after the other in one cell
        patch."""
        dofs = self._basis.dofs_per_axis
        nodes = (dofs, dofs, dofs)
        self._current_time_step = exahype2.datamodel.Patch(
            nodes, self.state.number_of_values, self._unknown_identifier()
        )
        self._rhs_estimates = exahype2.datamodel.Patch(
            (self.number_of_stages * dofs, dofs, dofs),
            self.state.unknowns,
            self._unknown_identifier() + "RhsEstimates",
        )
        self._estimate_projection = exahype2.datamodel.Patch(
            (2, dofs, dofs),
            self.state.number_of_values,
            self._unknown_identifier() + "EstimateProjection",
        )
        self._riemann_solution = exahype2.datamodel.Patch(
            (2, dofs, dofs),
            self.state.unknowns,
            self._unknown_identifier() + "RiemannSolution",
        )

        for patch in (
            self._current_time_step,
            self._rhs_estimates,
            self._estimate_projection,
            self._riemann_solution,
        ):
            patch.generator = exahype2.datamodel.PatchGenerator(patch, "double")

        for patch in (self._current_time_step, self._rhs_estimates):
            patch.generator.includes += [
                "peano4/Loop.h",
                "repositories/SolverRepository.h",
            ]
            patch.generator.can_receive = self._load_cell_data_default_guard()
            patch.generator.can_send = self._store_cell_data_default_guard()
        self._estimate_projection.generator.includes += [
            "peano4/Loop.h",
            "repositories/SolverRepository.h",
        ]
        self._riemann_solution.generator.includes.append(
            "repositories/SolverRepository.h"
        )
        self._rhs_estimates.generator.send_condition = "false"
        self._rhs_estimates.generator.receive_and_merge_condition = "false"

        exchange = self._projection_exchange_guard()
        send = "(" + self._solver_sweep_guard() + " or " + exchange + ")"
        for projection in (self._estimate_projection, self._riemann_solution):
            projection.generator.halo_layout = "projection_halves"
            projection.generator.can_receive = self._load_face_data_default_guard()
            projection.generator.can_send = self._store_face_data_default_guard()
            projection.generator.send_condition = send
            projection.generator.receive_and_merge_condition = exchange

        self._cell_label = exahype2.actionsets.create_cell_label(self._name)
        self._face_label = exahype2.actionsets.create_face_label(self._name)

    def create_action_sets(self):
        order = self._baseline_action_set_descend_invocation_order
        instance = "repositories::" + self.instance_name
        store_cell = self._store_cell_data_default_guard()
        store_face = self._store_face_data_default_guard()

        self._action_set_initial_conditions = exahype2.actionsets.InitialCondition(
            self, store_cell
        )
        self._action_set_amr = exahype2.actionsets.RefinementCriterion(
            solver=self,
            guard=store_cell
            + " and ("
            + instance
            + ".isLastGridSweepOfTimeStep() or "
            + self._sweep_is("isGridInitialisationSweep")
            + ")",
            build_up_new_refinement_instructions=True,
            implement_previous_refinement_instructions=False,
            called_by_grid_construction=False,
            skip_after_topology_change=True,
        )
        self._action_set_amr_throughout_grid_construction = (
            exahype2.actionsets.RefinementCriterion(
                solver=self,
                guard=store_cell,
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=True,
                called_by_grid_construction=True,
                skip_after_topology_change=False,
            )
        )
        self._action_set_amr_throughout_dynamic_amr = (
            exahype2.actionsets.RefinementCriterion(
                solver=self,
                guard=store_cell
                + " and fineGridCell"
                + self._cell_label.name
                + ".getRequiresCriterionEvaluation()",
                build_up_new_refinement_instructions=True,
                implement_previous_refinement_instructions=False,
                called_by_grid_construction=False,
                skip_after_topology_change=False,
            )
        )
        self._action_set_adaptive_mesh_face_coupling = (
            exahype2.dg.rk.actionsets.AdaptiveMeshFaceCoupling(solver=self)
        )
        self._action_set_interpolate_riemann_solution_onto_hanging_faces = (
            exahype2.dg.rk.actionsets.InterpolateRiemannSolutionOntoHangingFaces(self)
        )
        self._action_set_dynamic_amr_transfer = (
            exahype2.dg.rk.actionsets.DynamicAMRTransfer(solver=self)
        )
        self._action_set_update_solution = exahype2.dg.rk.actionsets.UpdateSolution(
            self,
            store_cell + " and " + self._solver_sweep_guard(),
            store_face + " and " + self._riemann_solve_guard(),
            store_face + " and " + self._solver_sweep_guard(),
        )
        self._action_set_update_face_label = exahype2.actionsets.UpdateFaceLabel(
            self._name
        )
        self._action_set_update_cell_label = exahype2.actionsets.UpdateCellLabel(
            self._name
        )

        uses_sweeps = instance + ".usesDynamicMeshRefinementSweeps()"
        stage_guards = []
        for stage in range(self.number_of_stages):
            guard = "({} and {} and {}.isRungeKuttaStage({}))".format(
                store_face, uses_sweeps, instance, stage
            )
            if stage == 0:
                guard = "({} or ({} and {}))".format(
                    guard,
                    store_face,
                    self._sweep_is("isTimeStepAfterGridInitialisationSweep"),
                )
            stage_guards.append(guard)
        self._action_set_adaptive_mesh_face_coupling.guards = stage_guards
        self._action_set_interpolate_riemann_solution_onto_hanging_faces.guards = [
            "{} and {} and {}.getRkStageAfterDynamicMeshRefinement()=={}".format(
                store_cell,
                self._sweep_is("isDynamicMeshRefinementSweep"),
                instance,
                stage,
            )
            for stage in range(self.number_of_stages)
        ]

        self._apply_descend_invocation_orders(_DESCEND_INVOCATION_ORDER, order)
        self.postprocess_solution.descend_invocation_order = (
            1 if self._error_measurement is not None else order
        )

    def register_data(self, datamodel):
        datamodel.add_cell(self._cell_label)
        datamodel.add_cell(self._current_time_step)
        datamodel.add_cell(self._rhs_estimates)
        datamodel.add_face(self._face_label)
        datamodel.add_face(self._estimate_projection)
        datamodel.add_face(self._riemann_solution)

    def use_data_in_step(self, step):
        step.use_cell(self._cell_label)
        step.use_cell(self._current_time_step)
        step.use_cell(self._rhs_estimates)
        step.use_face(self._face_label)
        step.use_face(self._estimate_projection)
        step.use_face(self._riemann_solution)

    def action_sets_per_step(self):
        """The dynamic mesh refinement sweep alone changes the topology; the next time step
        rebuilds the estimates and the face projections on the settled grid."""
        plotter = self._plotter(
            filename=self._name,
            patch=self._current_time_step,
            dataset_name=self._unknown_identifier(),
            description=self.state.plot_description,
            representation=self.layout.representation,
            mapping=self.layout.plot_mapping,
            time_stamp_evaluation=self._plot_time_stamp(),
            guard=self._load_cell_data_default_guard(),
        )
        plotter.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order
        )
        return {
            "create_grid": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr_throughout_grid_construction,
            ],
            "init_grid": [
                self._action_set_initial_conditions,
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets(),
            "plot_solution": [plotter],
            "perform_time_step": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_adaptive_mesh_face_coupling,
                self._action_set_interpolate_riemann_solution_onto_hanging_faces,
                self._action_set_update_solution,
                self._action_set_amr,
            ]
            + self._postprocess_action_sets(),
            "dynamic_amr": [
                self._action_set_update_face_label,
                self._action_set_update_cell_label,
                self._action_set_dynamic_amr_transfer,
                self._action_set_amr_throughout_dynamic_amr,
            ],
        }

    @property
    def postprocess_updated_cell_after_runge_kutta_step(self):
        return self._postprocess_updated_cell_after_runge_kutta_step

    @postprocess_updated_cell_after_runge_kutta_step.setter
    def postprocess_updated_cell_after_runge_kutta_step(self, kernel):
        """Sets the C++ code that runs after every completed stage; it updates the stage's
        right-hand side through ``dQdt`` and reads the solution through ``oldQ``."""
        self._postprocess_updated_cell_after_runge_kutta_step = kernel

    @property
    def postprocess_updated_cell_after_final_linear_combination(self):
        return self._postprocess_updated_cell_after_final_linear_combination

    @postprocess_updated_cell_after_final_linear_combination.setter
    def postprocess_updated_cell_after_final_linear_combination(self, kernel):
        """Sets the C++ code that runs once per completed time step; it updates the nodal state
        through ``newQ`` and reads ``timeStamp``."""
        self._postprocess_updated_cell_after_final_linear_combination = kernel

    def _validate(self):
        super()._validate()
        if self._is_linear and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError("A linear system carries no diffusive flux.")
        if self.pde_terms.is_set("flux") and self.pde_terms.is_set("diffusive_flux"):
            raise ValueError("A solver sets flux or diffusive_flux, not both.")

    def _error_condition(self):
        return (
            "not marker.hasBeenRefined() and "
            "(repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep() or "
            "repositories::{{SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()) and "
            + self._provide_cell_data_to_compute_kernels_default_guard()
        )

    def use_time_step_size(self, expression):
        raise NotImplementedError(
            "[{}] use_time_step_size applies to ADER-DG solvers only.".format(
                self._name
            )
        )

    def _cpp_base_class(self):
        return "::exahype2::dg::rk::Solver<{}, {}, {}, {}, {}, {}, {}>".format(
            self._abstract_class_name(),
            self._dg_order,
            self._rk_order,
            self.number_of_stages,
            self.state.unknowns,
            self.state.auxiliary_variables,
            self.time_stepping.cpp,
        )

    def _kernel_table_includes(self):
        return [
            "kernels/{}/Quadrature.h".format(self._name),
            "kernels/{}/DGMatrices.h".format(self._name),
            "kernels/{}/ButcherTableau.h".format(self._name),
        ]

    def _kernel_tables(self):
        return [
            "kernels::{}::Quadrature<double>".format(self._name),
            "kernels::{}::DGMatrices<double>".format(self._name),
            "kernels::{}::ButcherTableau<double>".format(self._name),
        ]

    def _generate_kernels(self, output, dictionary):
        generate_kernels(solver=self, dictionary=dictionary, output=output)

    def _add_family_entries(self, d):
        d["DG_ORDER"] = self._dg_order
        d["IS_LINEAR"] = self._is_linear
        d["NUMBER_OF_DOFS_PER_CELL"] = self._basis.dofs_per_axis**self._dimensions
        d["CURRENT_UNKNOWN_IDENTIFIER"] = self._unknown_identifier()
        d["QUADRATURE_TABLE"] = "kernels::{}::Quadrature<double>".format(self._name)
        d["POSTPROCESS_UPDATED_CELL_AFTER_RUNGE_KUTTA_STEP"] = textwrap.dedent(
            render_jinja2_template(
                self._postprocess_updated_cell_after_runge_kutta_step,
                d,
                jinja2.DebugUndefined,
            )
        ).strip()
        d["POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION"] = textwrap.dedent(
            render_jinja2_template(
                self._postprocess_updated_cell_after_final_linear_combination,
                d,
                jinja2.DebugUndefined,
            )
        ).strip()
