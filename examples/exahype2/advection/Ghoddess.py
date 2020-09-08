"""

 A very simple example which demonstrates how to configure a Ghoddess
 solver in ExaHyPE 2. The solver solves the advection equations with 
 rather low order and fixed time steps, but it gives a good first idea
 of the underlying mechanics.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os
import peano4
import peano4.visualisation
import exahype2
import exahype2.ghoddess


#
# Lets first clean up all plot files, so we don't get a mismatch
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") ]
for f in output_files:
  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "advection"], "ghoddess", "." )


#
# Our solver is simplistic as we prescribe the vector field of the equation. It thus 
# hosts only one vector field.
#
dimensions     = 2
order          = 1
unknowns       = 2 # set to 1 for scalar PDEs
time_step_size = 0.0001
project.add_solver(  exahype2.ghoddess.RusanovLegendreWithFixedTimeStepSize("Advection", order, unknowns, dimensions, 0.0001) )


build_mode = peano4.output.CompileMode.Asserts

#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = 2,  
  offset = [0.0,0.0],  
  size = [1.0,1.0],
  end_time=0.1,          
  first_plot_time_stamp = 0.0, 
  time_in_between_plots = 0.01     
)



peano4_project = project.generate_Peano4_project()
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.generate(peano4.output.Overwrite.Default)
peano4_project.build()
success = peano4_project.run( [] )


if success:
  convert = peano4.visualisation.Convert( "solutionAdvection", True )
  convert.set_visualisation_tools_path( "../../../../src/visualisation" )
  convert.extract_fine_grid()
  convert.convert_to_vtk()



