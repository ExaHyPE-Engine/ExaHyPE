import os, sys
import peano4
import exahype2

project = exahype2.Project( ["applications","exahype2", "ElasticWave"], "ElasticWaveFV", ".", executable="ElasticWaveFV" )

solver_name = "ElasticWaveFV"
dimensions  = 2
unknowns    = 5
auxiliary_variables = 3
max_h          = 0.2
min_h          = 0.06
patch_size  = 3

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  solver_name,
  patch_size,
  unknowns, auxiliary_variables,
  min_h, max_h,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  ncp = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )

project.add_solver( thesolver )

build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0],
  size = [10.0,10.0],
  min_end_time = 0.04,
  max_end_time = 0.04,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.001,
  periodic_BC           = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation("../../../", build_mode )
peano4_project = project.generate_Peano4_project(False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=12)
