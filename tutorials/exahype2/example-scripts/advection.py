import os
import sys

sys.path.insert(0, os.path.abspath("../../../python"))

import peano4
import exahype2

dimensions = 2

project = exahype2.Project( ["examples", "exahype2", "tutorial"], "advection", ".", executable="Advection" + str(dimensions) + "D" )

#
# Add the Finite Volumes solver using named arguments. This is the way
# you can add further PDE terms.
#
#
thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
  "advection",
  patch_size=3,
  unknowns=3, auxiliary_variables=0,
  min_volume_h=0.01, max_volume_h=0.03,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  time_step_relaxation = 0.1
)
thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )

# Add the solver to the peano project
project.add_solver( thesolver )


#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = 2,
  offset = [0.0,0.0],
  size = [1.0,1.0],
  min_end_time = 0.5,
  max_end_time = 0.5,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.025, # snapshots
  periodic_BC = [False, False, False]
)

project.set_output_path("solutions")
if not os.path.exists("solutions"):
  os.makedirs("solutions")

#
# This generates the project according to the parameters defined before
#
build_mode = peano4.output.CompileMode.Release
project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../", build_mode )
peano4_project = project.generate_Peano4_project(False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: Advection"+str(dimensions)+"D")
print( "Convert any output via pvpython ../../../python/peano4/visualisation/render.py solutions/solution-advection"+str(dimensions)+"d.peano-patch-file")
