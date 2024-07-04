import peano4, exahype2
import os, sys
import argparse, string

order = 3

max_h = 1.0/9.0
min_h = 1.0/27.0

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(["exahype2", "aderdg"], ".", executable="SWE")

theSolver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
  name  = "swe", order = order,
  min_cell_h = min_h, max_cell_h = max_h,
  time_step_relaxation  = 0.8,
  unknowns              = 4,
  auxiliary_variables   = 0,
  refinement_criterion = exahype2.solvers.PDETerms.User_Defined_Implementation,
  flux        = exahype2.solvers.PDETerms.User_Defined_Implementation,
  ncp         = exahype2.solvers.PDETerms.User_Defined_Implementation
)

project.set_output_path("solutions")

project.add_solver(theSolver)

project.set_global_simulation_parameters(
    dimensions=2,
    offset=[0., 0.],
    size=[1., 1.],
    min_end_time=0.2,
    max_end_time=0.2,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.01,
    periodic_BC=[False, False],
)

project.set_load_balancing(
  "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
  "new ::exahype2::LoadBalancingConfiguration(0.98)"
)
project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release)
peano4_project = project.generate_Peano4_project("False")
peano4_project.output.makefile.add_cpp_file("swe.cpp")

peano4_project.build(make_clean_first=True, number_of_parallel_builds=16)
