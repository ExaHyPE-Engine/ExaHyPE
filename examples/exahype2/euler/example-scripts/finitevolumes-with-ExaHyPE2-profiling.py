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
project.add_solver(  exahype2.solvers.GenericRusanovFVFixedTimeStepSize(
  "Euler", 
  patch_size, 
  unknowns, time_step_size,
  flux = True,
  ncp  = False
))



dimensions = 3
build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Asserts



#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
  time_step_size * 1000,           # end time
  0.0, 0                          # snapshots
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision" )

import sys

peano4_project = project.generate_Peano4_project()
if len(sys.argv)>1:
  volume_max = float( sys.argv[1] )
  print( "use max FV size of " + str(volume_max) )
  peano4_project.constants.export( "MaxHOfVolume", volume_max )
else:
  peano4_project.constants.export( "MaxHOfVolume", 0.1 )
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.add_library( "ToolboxLoadBalancing" + project.get_library_postfix(build_mode), "../../../../src/toolbox/loadbalancing" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.build(make_clean_first=True)

