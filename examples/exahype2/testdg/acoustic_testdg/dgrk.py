


import peano4
import exahype2

project = exahype2.Project( ["examples", "exahype2", "testing"], "testing", ".", executable="peano4" )

theSolver=exahype2.solvers.rkdg.rusanov.GlobalFixedTimeStep(
  name="testing",
  dg_order=3, rk_order=2,
  polynomials=exahype2.solvers.rkdg.Polynomials.Gauss_Legendre,
  unknowns=3, auxiliary_variables=2,
  min_cell_h=0.1, max_cell_h=0.1,
  time_step_size=0.0001)

theSolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation)

project.add_solver(theSolver)

project.set_global_simulation_parameters(
  dimensions = 2,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  end_time = 0.03,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0003,      # snapshots
  periodic_BC = [False,False,False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../..", peano4.output.CompileMode.Release )

peano4_project = project.generate_Peano4_project(False)

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)
