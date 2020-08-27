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
import os
import peano4
import peano4.datamodel
import peano4.solversteps
import peano4.output
import peano4.visualisation
import peano4.toolbox.blockstructured


import exahype2


import argparse



parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("--load-balancing-quality", dest="load_balancing_quality", type=float, required=True, help="Load balancing quality (something between 0 and 1; 1 is optimal)" )
parser.add_argument("--h",              dest="h",              type=float, required=True, help="Mesh size" )
args = parser.parse_args()


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", "." )


#
# Add the Finite Volumes solver
#
patch_size     = 7
unknowns       = 5
time_step_size = 0.000001

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#project.add_solver(  exahype2.solvers.GenericRusanovFVFixedTimeStepSize(
project.add_solver(  exahype2.solvers.GenericRusanovFVFixedTimeStepSizeWithEnclaves(
  "Euler", 
  patch_size, 
  unknowns, time_step_size,
  flux = True,
  ncp  = False,
  use_gpu =  False
))



dimensions = 2
build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Trace



#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
  time_step_size * 50,           # end time
  0.0, 0                         # snapshots
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(" + str(args.load_balancing_quality) + ")" )

peano4_project = project.generate_Peano4_project()
peano4_project.constants.export( "MaxHOfVolume", args.h )
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.add_library( "ToolboxLoadBalancing" + project.get_library_postfix(build_mode), "../../../../src/toolbox/loadbalancing" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.build(make_clean_first=True,number_of_parallel_builds=12)

