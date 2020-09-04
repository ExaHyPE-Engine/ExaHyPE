"""

 An empty project. I use it for debugging to ensure that startup/shutdown 
 processes for example pass through without problems.

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
# Lets first clean up all plot files, so we don't get a mismatch
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f=="peano4"]
for f in output_files:
  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "empty"], "empty", "." )




dimensions = 2
#build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Asserts
build_mode = peano4.output.CompileMode.Debug


#
# Lets configure some global parameters
#
if dimensions==2:
  project.set_global_simulation_parameters(
    dimensions,  [0.0,0.0],  [1.0,1.0],
    1.0,          # end time
    0.0, 0.0      # this artificial test problem fails if you ask for plots
  )
else:
  project.set_global_simulation_parameters(
    dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
    1.0, 0.0, 0.0
  )


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with. Domain decomposition does not
# make sense here, but I keep it to validate proepr startup/shutdown
# behaviour
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision" )


peano4_project = project.generate_Peano4_project()
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.add_library( "ToolboxLoadBalancing" + project.get_library_postfix(build_mode), "../../../../src/toolbox/loadbalancing" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.build()
#
# Use this variant if you build without MPI. In principle, you can start
# your MPI test from the Python script. My Python installation however
# often crashes for bigger runs. It also struggles with the postprocessing
# for large parallel runs (see below).
#
#success = peano4_project.run( ["threads", "1"] )
success = peano4_project.run( [], ["mpirun", "-n", "2"] )

