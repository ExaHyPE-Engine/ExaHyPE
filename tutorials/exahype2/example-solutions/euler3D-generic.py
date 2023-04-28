#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import peano4
import exahype2
import os #used to create the directory which the solutions are stored in

dimensions = 3

project = exahype2.Project( ["examples", "exahype2", "tutorial"], "euler", ".", executable="Euler" + str(dimensions) + "D" )

#
# Add the Ader-DG solver using named arguments. This is the way
# you can add further PDE terms.
#
#
theSolver=exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep(
  name="euler"+str(dimensions)+"D",  order=3,
  unknowns=5, auxiliary_variables=0,
  min_cell_h=0.03, max_cell_h=0.1, time_step_relaxation=0.9
)
theSolver.add_kernel_optimizations(use_kernel_generator=False)
theSolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )

# Add the solver to the peano project
project.add_solver( theSolver )


#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0, 0.0, 0.0],
  size = [1.0, 1.0, 1.0],
  min_end_time = 0.2,
  max_end_time = 0.2,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.005, # snapshots
  periodic_BC = [False, False, False]
)

project.set_output_path("solutions")
if not os.path.exists("solutions"):
  os.makedirs("solutions")

#
# This generates the project according to the parameters defined before
#
build_mode = peano4.output.CompileMode.Release
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( "../../", build_mode )
peano4_project = project.generate_Peano4_project(False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=4)

print("Done. Executable is: Euler"+str(dimensions)+"D")
print( "Convert any output via pvpython ../../../python/peano4/visualisation/render.py solutions/solution-euler"+str(dimensions)+"d.peano-patch-file")
