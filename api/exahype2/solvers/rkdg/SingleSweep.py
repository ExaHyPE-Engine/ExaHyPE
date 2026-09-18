# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG import RungeKuttaDG
import exahype2.solvers.rkdg.actionsets


class SingleSweep(RungeKuttaDG):
    """RKDG using one traversal per Runge-Kutta stage."""

    def __init__(
        self,
        name,
        rk_order,
        dg_order,
        unknowns,
        auxiliary_variables,
    ):
        super().__init__(
            name,
            rk_order,
            dg_order,
            unknowns,
            auxiliary_variables,
        )
        self._solver_template_file_class_name = "SingleSweep"

    def _solver_sweep_guard(self):
        return (
            "(" + self._runge_kutta_sweep_guard()
            + f" or repositories::{self.get_name_of_global_instance()}.getSolverState()=="
            + f"{self._name}::SolverState::TimeStepAfterGridInitialisation)"
        )

    def _runge_kutta_sweep_guard(self):
        state = f"repositories::{self.get_name_of_global_instance()}.getSolverState()"
        return "(" + " or ".join(
            f"{state}=={self._name}::SolverState::RungeKuttaStep{step}"
            for step in range(self.number_of_Runge_Kutta_steps())
        ) + ")"

    def _projection_exchange_guard(self):
        return (
            "(" + self._runge_kutta_sweep_guard()
            + f" or repositories::{self.get_name_of_global_instance()}.getSolverState()=="
            + f"{self._name}::SolverState::DynamicMeshRefinement"
            + f" or repositories::{self.get_name_of_global_instance()}.getSolverState()=="
            + f"{self._name}::SolverState::Plotting)"
        )

    def create_data_structures(self):
        super().create_data_structures()
        solver_sweep = self._solver_sweep_guard()
        self._estimate_projection.generator.send_condition = (
            "(" + solver_sweep + " or " + self._projection_exchange_guard() + ")"
        )
        self._estimate_projection.generator.receive_and_merge_condition = self._projection_exchange_guard()
        self._Riemann_solution.generator.send_condition = (
            "(" + solver_sweep + " or " + self._projection_exchange_guard() + ")"
        )
        self._Riemann_solution.generator.receive_and_merge_condition = self._projection_exchange_guard()
        self._linear_combination_of_estimates.generator.can_receive = "false"
        self._linear_combination_of_estimates.generator.can_send = "false"

    def _Riemann_solve_guard(self):
        """Selects the sweep that computes the Riemann solution.

        Fine cells read the coarse trace before the next Runge-Kutta stage overwrites it.
        """
        instance = f"repositories::{self.get_name_of_global_instance()}"
        return (
            "(" + f"{instance}.getSolverState()=={self._name}::SolverState::DynamicMeshRefinement"
            + " or (" + self._runge_kutta_sweep_guard()
            + f" and not {instance}.usesDynamicMeshRefinementSweeps()))"
        )

    def create_action_sets(self):
        super().create_action_sets()
        cell_guard = self._store_cell_data_default_guard() + " and " + self._solver_sweep_guard()
        riemann_solve_guard = (
            self._store_face_data_default_guard() + " and " + self._Riemann_solve_guard()
        )

        face_guard = self._store_face_data_default_guard() + " and " + self._solver_sweep_guard()

        self._action_set_update_solution = exahype2.solvers.rkdg.actionsets.UpdateSolution(
            self, cell_guard, riemann_solve_guard, face_guard
        )
        self._action_set_update_solution.descend_invocation_order = (
            self._baseline_action_set_descend_invocation_order + 3
        )

        adaptive = (
            f"repositories::{self.get_name_of_global_instance()}.usesDynamicMeshRefinementSweeps()"
        )
        stage_guards = []
        for step in range(self.number_of_Runge_Kutta_steps()):
            guard = "(" + (
                self._store_face_data_default_guard()
                + " and " + adaptive
                + f" and repositories::{self.get_name_of_global_instance()}.getSolverState()=="
                + f"{self._name}::SolverState::RungeKuttaStep{step}"
            ) + ")"
            if step == 0:
                guard = (
                    "(" + guard + " or (" + self._store_face_data_default_guard()
                    + f" and repositories::{self.get_name_of_global_instance()}.getSolverState()=="
                    + f"{self._name}::SolverState::TimeStepAfterGridInitialisation))"
                )
            stage_guards.append(guard)
        self._action_set_adaptive_mesh_face_coupling.guards = stage_guards
        self._action_set_interpolate_Riemann_solution_onto_hanging_faces.guards = [
            (
                self._store_cell_data_default_guard()
                + f" and repositories::{self.get_name_of_global_instance()}.getSolverState()=="
                + f"{self._name}::SolverState::DynamicMeshRefinement"
                + f" and repositories::{self.get_name_of_global_instance()}.getRkStageAfterDynamicMeshRefinement()=={step}"
            )
            for step in range(self.number_of_Runge_Kutta_steps())
        ]

    def add_actions_to_perform_time_step(self, step):
        step.add_action_set(self._action_set_update_face_label)
        step.add_action_set(self._action_set_update_cell_label)
        step.add_action_set(
            self._action_set_adaptive_mesh_face_coupling
        )
        step.add_action_set(
            self._action_set_interpolate_Riemann_solution_onto_hanging_faces
        )
        step.add_action_set(self._action_set_update_solution)
        step.add_action_set(self._action_set_AMR)
        step.add_action_set(self._action_set_postprocess_solution)
