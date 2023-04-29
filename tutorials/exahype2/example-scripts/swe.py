import os
import sys

sys.path.insert(0, os.path.abspath("../../../python"))

import peano4
import exahype2

project = exahype2.Project( ["examples", "exahype2", "tutorial"], "swe", ".", executable="SWE" )

#
# Add the Finite Volumes solver using named arguments. This is the way
# you can add further PDE terms.
#
#

auxiliary_variables = 1

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  "swe",
  patch_size=3,
  unknowns=3, auxiliary_variables=1,
  min_volume_h=0.005, max_volume_h=0.05,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  ncp = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  time_step_relaxation = 0.5
)

thesolver.set_implementation(refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation)

project.add_solver( thesolver )




#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = 2,
  offset = [-1.0,-1.0],
  size = [2.0,2.0],
  min_end_time = 0.20,
  max_end_time = 0.20,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005,      # snapshots
  periodic_BC = [False, False, False]
)

# this defines where the output files should go
project.set_output_path("solutions")
if not os.path.exists("solutions"):
  os.makedirs("solutions")

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
build_mode = peano4.output.CompileMode.Release
project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../", build_mode )
peano4_project = project.generate_Peano4_project(False)

peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: SWE")
print( "Convert any output via pvpython ../../../python/peano4/visualisation/render.py solutions/solution-swe.peano-patch-file")
