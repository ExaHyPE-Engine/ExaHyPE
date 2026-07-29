# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG  import RungeKuttaDG


class SeparateSweeps(RungeKuttaDG):
  """
  Runge-Kutta DG solver that realises each Runge-Kutta step with two grid sweeps.

  We run over the grid twice per Runge-Kutta step. In the first sweep, we compute
  the linear reconstruction, project this one onto the faces and also compute the
  volumetric prediction. In the second sweep, we solve the Riemann problem and add
  it to the prediction.

  In the attribute terminology, we call the two runs over the grid sweeps,
  i.e., we work with two grid sweeps per Runge-Kutta step.
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
    Set up the guards before the superclass constructor runs.

    The super call invokes the creation of the data sets, where the guards have
    to be known already.
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
    Call the superclass' create_data_structures() and refine the exchange rules.

    The estimate projection is exchanged from a primary sweep to the secondary
    sweep that consumes it, while the Riemann solution and the linear combination
    stay on the tree that computed them.
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
    We invoke the superclass to create the action sets and then set the guards.

    - _action_set_update_face_label - can always be done.
    - _action_set_update_cell_label - can always be done.
    - _action_set_linear_combination_of_estimates - primary sweeps only, as
      these start the Runge-Kutta step.
    - _action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement -
      primary only, as the secondary sweep is only a clean-up.
    - _action_set_project_linear_combination_onto_faces - only required in the
      primary sweep. In the secondary sweep we collect the results, so the
      boundary data is not needed there.
    - _action_set_solve_volume_integral - only done in the primary sweep.
    - _action_set_handle_boundary - only in the secondary sweep, where the left
      and right face data are available and hence the Riemann solver can be
      invoked.
    - _action_set_solve_Riemann_problem - secondary sweep. See discussion of
      previous item.
    - _action_set_interpolate_Riemann_solution_onto_hanging_faces - secondary
      sweep, since it needs the coarse face's Riemann solution to interpolate
      onto the hanging faces below it.
    - _action_set_add_volume_and_face_solution - secondary sweep. The
      Riemann solve plugs into the first touch of a face in the secondary
      sweep, so the solutions can be summed up.
    - _action_set_compute_final_linear_combination_and_project_solution_onto_faces -
      very last secondary sweep.
    - _action_set_AMR - very last secondary sweep, i.e., after we have
      the final solution.
    - _action_set_postprocess_solution - same as for the item above.

    :: Guards around linear combination of estimates

    We rely on the action set LinearCombination as it is initialised in the
    superclass. So the only thing we have to do is to ensure that the
    linear combination over the estimates is computed exactly where it is
    required. For the separate sweeps, we compute the linear combinations
    if the cell holds data, i.e., is on the finest level, and if it is the
    primary of the two sweeps which realise an RK step, i.e., the one where
    we later on project onto the faces.
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
