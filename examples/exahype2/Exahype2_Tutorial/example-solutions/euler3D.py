"""

 A very simple example which demonstrates how to configure a patch-based
 Finite Volume solver in Peano4. The code relies on snippets from ExaHyPE2.
 However, it relies only on ExaHyPE's C/FORTRAN compute kernels, i.e. the
 sophisticated build environment of this H2020 project is not used. The
 solver simulates the simple Euler equations.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os, sys
import peano4
import exahype2
import argparse


project = exahype2.Project( ["examples", "exahype2", "euler3D"], "euler3D", ".", executable="Euler3D" )


#
# Add the Finite Volumes solver
#
unknowns       = 0 #TODO
time_step_size = 0.001
max_h          = 0.03
min_h          = 0.0


#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#

auxiliary_variables = 0

thesolver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
  "euler3D",
  3,
  unknowns, auxiliary_variables,
  min_h, max_h,
  time_step_size,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)


#thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )


project.add_solver( thesolver )


dimensions = 3
build_mode = peano4.output.CompileMode.Release



#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = 0, #TODO
  offset = [0.0,0.0,0.0],
  size = [], #TODO
  end_time = 0.1,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005,      # snapshots
  periodic_BC = [False, False, False]
)



#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../", build_mode )
peano4_project = project.generate_Peano4_project(False)

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: Euler3D")
print( "Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-euler3D.peano-patch-file")
