# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .RungeKuttaDG  import RungeKuttaDG
from exahype2.solvers.PDETerms      import PDETerms

import peano4
import exahype2

import jinja2

from peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor import ReconstructPatchAndApplyFunctor


class SeparateSweeps(RungeKuttaDG):
  """
  Probably the simplest solver you could think off.
  We run over the grid twice per Runge-Kutta step. In the first step,
  we compute the linear reconstruction, project this one onto the faces
  and also compute the volumetric prediction. In the second step, we
  solve the Riemann problem and add it to the prediction.

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
               min_cell_h,
               max_cell_h,
               plot_grid_properties,
               pde_terms_without_state):
    """
    The super call invokes the creation of the data sets, where the guards have
    to be known already. So we bring those guys forward and then call the superclass
    constructor.
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
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    self._secondary_sweeps_of_Runge_Kutta_step_on_cell = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self._store_cell_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]

    self._primary_sweeps_of_Runge_Kutta_step_on_face = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{}".format(self._store_face_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
    ]
    self._secondary_sweeps_of_Runge_Kutta_step_on_face = [
      "{} and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self._store_face_data_default_guard(),self.get_name_of_global_instance(),self._name,step) for step in range(0,self.number_of_Runge_Kutta_steps())
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
                                         min_cell_h,
                                         max_cell_h,
                                         plot_grid_properties,
                                         pde_terms_without_state)

    self._solver_template_file_class_name = "SeparateSweeps"

    self.create_action_sets()
    self.create_data_structures()

  def create_data_structures(self):
    """
    First, call the superclass' create_data_structures() to ensure that all
    the data structures are in place.

    The linear combination is to be computed if and only if
    """
    super(SeparateSweeps, self).create_data_structures()

    self._estimate_projection.generator.send_condition               = self._any_primary_sweep_of_a_Runge_Kutta_step_on_face
    self._estimate_projection.generator.receive_and_merge_condition  = self._any_secondary_sweep_of_a_Runge_Kutta_step_on_face

    self._Riemann_solution.generator.send_condition               = "false"
    self._Riemann_solution.generator.receive_and_merge_condition  = "false"

    any_primary_sweep_predicate = "(" + " or ".join( self._primary_sweeps_of_Runge_Kutta_step_on_cell ) + ")"
    self._linear_combination_of_estimates.generator.load_store_compute_flag = "::peano4::grid::constructLoadStoreComputeFlag({},{},{})".format(
        self._provide_cell_data_to_compute_kernels_default_guard() + " and " + any_primary_sweep_predicate,
        "false",
        "false",
    )        


  def create_action_sets(self):
    """
      We invoke the superclass which creates all the action sets, and then we
      set the appropriate guards.

        - _action_set_update_face_label - can be done always
        - _action_set_update_cell_label - can be done always
        - _action_set_linear_combination_of_estimates - primary sweeps only, as
          these kick off the Runge-Kutta step.
        - _action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement -
          primary only, as the secondary step is a sole clean-up
        - _action_set_project_linear_combination_onto_faces - only required in
          primary sweep. In the secondary, we collect the results together, i.e., we
          don't immediately need the boundary data anymore.
        - _action_set_solve_volume_integral - only done in the primary sweep.
        - _action_set_handle_boundary - only in secondary sweep, as we now have
          access to the left and right face data and hence can invoke the
          Riemann solver.
        - _action_set_solve_Riemann_problem - secondary sweep. See discussion of
          previous item.
        - _action_set_add_volume_and_face_solution - secondary sweep. The
          Riemann solve plugs into the first touch of a face in the secondary
          sweep, so now we can sum up the solutions.
        - _action_set_compute_final_linear_combination_and_project_solution_onto_faces -
          very last secondary sweep.
        - _action_set_AMR - very last secondary sweep, i.e., after we have
          the final solution.
        - _action_set_postprocess_solution - same as for item above.

      :: Guards around linear combination of estimates

      We rely on the action set LinearCombination as it is initialised in the
      superclass. So the only thing we have to do is to ensure that the
      linear combination over the estimates is computed if and only if this
      is required. For the separate sweeps, we compute the linear combinations
      if the cell holds data, i.e., is on the finest level, and if it is the
      primary of the two sweeps which realise a RK step, i.e., the one where
      we later on project onto the faces.
    """
    super(SeparateSweeps, self).create_action_sets()
    self._action_set_linear_combination_of_estimates.guards                                  = self._primary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.guards = self._primary_sweeps_of_Runge_Kutta_step_on_face
    self._action_set_project_linear_combination_onto_faces.guards                            = self._primary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_solve_volume_integral.guards                                            = self._primary_sweeps_of_Runge_Kutta_step_on_cell

    self._action_set_add_volume_and_face_solution.guards = self._secondary_sweeps_of_Runge_Kutta_step_on_cell
    self._action_set_solve_Riemann_problem.guards        = self._secondary_sweeps_of_Runge_Kutta_step_on_face
    self._action_set_handle_boundary.guards              = self._secondary_sweeps_of_Runge_Kutta_step_on_face

    self._action_set_compute_final_linear_combination_and_project_solution_onto_faces.guard = self._store_cell_data_default_guard() + " and repositories::{}.getSolverState()=={}::SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{}".format(self.get_name_of_global_instance(),self._name,self.number_of_Runge_Kutta_steps()-1)
