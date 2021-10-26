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

import peano4
import exahype2


project = exahype2.Project( ["examples", "exahype2", "euler2D"],
                             "euler2D", ".", executable="Euler2D" )


#
# Add the Finite Volumes solver
#
unknowns       = 4
time_step_size = 0.001
max_h          = 0.01
min_h          = 0.01

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#

auxiliary_variables = 0

thesolver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
  name="euler2D",
  patch_size=3,
  unknowns=4, auxiliary_variables=0,
  min_volume_h=0, max_volume_h=0.01,
  time_step_size=0.001,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
)

project.add_solver( thesolver )

#
# Lets configure some global parameters
#
build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = 2,
  offset = [0.0,0.0],
  size = [1.0,1.0],
  end_time = 0.1,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.001
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../../", build_mode )
peano4_project = project.generate_Peano4_project(False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: Euler2D")
print( "Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-euler2D.peano-patch-file")
