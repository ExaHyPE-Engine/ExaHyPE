# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG  import RungeKuttaDG


class SeparateSweeps(RungeKuttaDG):
  """
  Runge-Kutta DG solver that realises each Runge-Kutta step with two grid sweeps.

  The primary sweep computes the linear combination, projects it onto the faces
  and computes the volumetric prediction. The secondary sweep solves the Riemann
  problem and adds it to the prediction.
  """

  def __init__(self,
               name,
               rk_order,
               polynomial_basis,
               number_of_face_projections,
               unknowns,
               auxiliary_variables,
               pde_terms_without_state):
    """
    Sets up the guards before the superclass constructor runs.

    The super call creates the data sets, which read the guards.
    """
    self._name     = name
    self._rk_order = rk_order

    self._initialisation_sweep_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation" + \
      ")"
    self._first_iteration_after_initialisation_guard = "(" + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::TimeStepAfterGridInitialisation or " + \
      "repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::PlottingAfterGridInitialisation" + \
    ")"

    self._primary_sweeps_of_Runge_Kutta_step_on_cell = [
      "({} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{})".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    self._secondary_sweeps_of_Runge_Kutta_step_on_cell = [
      "({} and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{})".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]

    self._primary_sweeps_of_Runge_Kutta_step_on_face = [
      "({} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{})".format(self._store_face_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    self._secondary_sweeps_of_Runge_Kutta_step_on_face = [
      "({} and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{})".format(self._store_face_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]

    self._any_primary_sweep_of_a_Runge_Kutta_step_on_cell   = " or ".join( self._primary_sweeps_of_Runge_Kutta_step_on_cell )
    self._any_secondary_sweep_of_a_Runge_Kutta_step_on_cell = " or ".join( self._secondary_sweeps_of_Runge_Kutta_step_on_cell )

    self._any_primary_sweep_of_a_Runge_Kutta_step_on_face   = " or ".join( self._primary_sweeps_of_Runge_Kutta_step_on_face )
    self._any_secondary_sweep_of_a_Runge_Kutta_step_on_face = " or ".join( self._secondary_sweeps_of_Runge_Kutta_step_on_face )

    super(SeparateSweeps, self).__init__(name,
                                         rk_order,
                                         polynomial_basis,
                                         number_of_face_projections,
                                         unknowns,
                                         auxiliary_variables,
                                         pde_terms_without_state)

    self._solver_template_file_class_name = "SeparateSweeps"

    self.create_action_sets()
    self.create_data_structures()

  def create_data_structures(self):
    """
    Refines the exchange rules of the superclass.

    The estimate projection travels from a primary sweep to the secondary sweep
    that reads it, while the Riemann solution and the linear combination stay on
    the tree that computed them.
    """
    super(SeparateSweeps, self).create_data_structures()

    self._estimate_projection.generator.send_condition               = self._any_primary_sweep_of_a_Runge_Kutta_step_on_face
    self._estimate_projection.generator.receive_and_merge_condition  = self._any_secondary_sweep_of_a_Runge_Kutta_step_on_face

    self._Riemann_solution.generator.send_condition               = "false"
    self._Riemann_solution.generator.receive_and_merge_condition  = "false"

    self._linear_combination_of_estimates.generator.can_receive = "false"
    self._linear_combination_of_estimates.generator.can_send = "false"

  def create_action_sets(self):
    """
    Sets the sweep guards on the action sets the superclass creates.

    The primary sweep carries every action set that starts a Runge-Kutta step,
    and the secondary sweep carries every action set that needs both face sides.
    A guard holds only on the finest level, where the cell stores data.
    """
    super(SeparateSweeps, self).create_action_sets()
    self._action_set_linear_combination_of_estimates.guards                                  = self._primary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guards = self._primary_sweeps_of_Runge_Kutta_step_on_face
    self._action_set_project_linear_combination_onto_faces.guards                            = self._primary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_solve_volume_integral.guards                                            = self._primary_sweeps_of_Runge_Kutta_step_on_cell

    self._action_set_add_volume_and_face_solution.guards = self._secondary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_solve_Riemann_problem.guards        = self._secondary_sweeps_of_Runge_Kutta_step_on_face
    self._action_set_interpolate_Riemann_solution_onto_hanging_faces.guards = self._secondary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_handle_boundary.guards              = self._secondary_sweeps_of_Runge_Kutta_step_on_face

    self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,self.number_of_Runge_Kutta_steps()-1)
