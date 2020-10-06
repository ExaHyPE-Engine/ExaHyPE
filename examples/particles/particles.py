"""

 A very simple demonstration how to use the pidt (particle in dual tree) scheme
 within Peano 4.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os

import peano4
import peano4.dastgen2
import dastgen2


#
# Lets first clean up all plot files, so we don't get a mismatch
#
#output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") ]
#for f in output_files:
#  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = peano4.Project( ["examples", "particles"], "particles", "." )




#
# First of all, we determine what a particle is
#
particle = peano4.datamodel.DaStGen2( "Particle" )
particle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("x","Dimensions") )


project.datamodel.add_vertex(particle)


#
# Peano's API does not know which settings to use on the present system. To 
# make it copy/clone the settings identified by ./configure, we ask it to 
# parse the generated configuration scripts. The makefile would also offer a
# routine to set the dimension. We take the default here.
#
# @todo Adopt to your particular system settings
#
project.output.makefile.parse_configure_script_outcome( "../.." )


#
# Standard triad of operations. You can skip the first two steps if you want as 
# the script then will automatically invoke the previous steps. The other way 
# round, it is always admissible to only generate stuff, e.g., but to build and
# run the project through a command line
#
project.generate(peano4.output.Overwrite.Default)
project.build()
success = project.run( args=[] )

