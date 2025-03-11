import os, sys
import peano4
import exahype2

project = exahype2.Project(namespace=["benchmarks", "exahype2", "euler", "sod_shocktube"], project_name="sod-shocktube-validation", directory=".", executable="sod-shocktube-validation")

solver_name = "EulerFVSod"
unknowns    = 4
auxiliary_variables = 0
mesh_size   = 0.01
patch_size  = 10
dimensions  = 2

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  solver_name,
  patch_size,
  unknowns, auxiliary_variables,
  mesh_size,
  mesh_size,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.PDETerms.User_Defined_Implementation,
  eigenvalues = exahype2.solvers.PDETerms.User_Defined_Implementation,
)

project.add_solver(thesolver)

build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0],
  size = [1.0,1.0],
  min_end_time = 0.1,
  max_end_time = 0.1,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.001,
  periodic_BC           = [False, False, False]
)

project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()")
project.set_Peano4_installation("../../../../", build_mode)
peano4_project = project.generate_Peano4_project(False)
peano4_project.output.makefile.add_h_file("EulerFVSod.h")
peano4_project.output.makefile.add_cpp_file("EulerFVSod.cpp")
peano4_project.build(make_clean_first=True)
