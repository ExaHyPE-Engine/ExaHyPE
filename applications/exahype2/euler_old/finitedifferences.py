"""

 A very simple example which demonstrates how to configure a patch-based
 Finite Volume solver in ExaHyPE 2. The solver simulates the simple Euler 
 equations.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os, sys
import peano4
import exahype2


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "dg", ".", executable="finitedifferences" )


#
# Add the Finite Volumes solver
#
dimensions     = 2
max_h          = 0.01
min_h          = max_h
patch_size     = 8
unknowns       = 1 + dimensions + 1  # density, velocity, energy

thesolver = exahype2.solvers.rkfd.fd4.GlobalAdaptiveTimeStep(
    "EulerFD",
    patch_size,
    unknowns = unknowns, auxiliary_variables = 0,
    min_meshcell_h = min_h, max_meshcell_h = max_h,
    rk_order = 4,
    flux = exahype2.solvers.rkfd.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.rkfd.PDETerms.User_Defined_Implementation,
    time_step_relaxation = 0.01
  )


project.add_solver( thesolver )


#
# Configure some global parameters
#
end_time   = 0.1
plot_snapshot_interval = 0.001
periodic_BC = [False, False, False]


project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  min_end_time = end_time,
  max_end_time = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_snapshot_interval,      # snapshots
  periodic_BC = periodic_BC
)


#
# Here's the parallel stuff. Will be ignored if you switch off both
# shared and distributed memory parallelisation
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )


#
# Parse configure settings, generate all glue code, and compile code
#
verbose    = False
build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Asserts
#build_mode = peano4.output.CompileMode.Debug
peano_dir  = "../../.."
project.set_Peano4_installation( peano_dir, build_mode )
peano4_project = project.generate_Peano4_project(verbose)
peano4_project.build(make_clean_first=True)

print( """

ExaHyPE 2's Finite Differences example built successfully. Next, run the code with

./finitedifferences

To convert the written files into native Paraview files, call

pvpython ../../../python/peano4/visualisation/render.py --filter-fine-grid solution-EulerFD.peano-patch-file

""")
