"""

 A very simple example for a geometric Jacobi solver. This is the simplest 
 variant of an elliptic solver that you can think of in Peano. It is reasonable
 to study this example in detail before you do more sophisticated stuff, as 
 all of my multigrid solvers kind of build upon this solver.
 
 Ensure your python path is set properly before you invoke this script. On 
 my bash system, I can set it for example through
 
 export PYTHONPATH=../..
 
 @author Tobias Weinzierl

"""

import os
import time

import peano4
import peano4.visualisation
import peano4.toolbox

import dastgen2


#
# Lets first clean up all plot files, so we don't get a mismatch.
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f.endswith(".log") ]
for f in output_files:
  os.remove(f)



#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). I use the namespace algebraicmg here. This is what we wanna do
# eventually. So I use this namespace for all of my examples.
#
project = peano4.Project( ["examples", "jacobi"], "amr-parallel", "." )


#
# There's different ways to tell the project which data are associated to the 
# grid entities. One way to associate stuff is to use our DaStGen legacy format.
# This is our strategy here.
#
use_legacy_dastgen = False
if use_legacy_dastgen:
  dastgen_model = peano4.datamodel.DaStGen( "MG" )
  dastgen_model.add_double_scalar( "u" )
  dastgen_model.add_double_scalar( "rhs" )
  dastgen_model.add_double_scalar( "eps" )
  dastgen_model.add_double_scalar( "diag" )
  dastgen_model.add_double_scalar( "res" )
  dastgen_model.add_enum( "VertexType", ["Boundary", "Inside"] )
  #peano4.datamodel.DaStGenToLegacyTool.DaStGenJarFile = "/ddn/data/frmh84/DaStGen/DaStGen.jar"
  peano4.datamodel.DaStGenToLegacyTool.DaStGenJarFile = "~/git/DaStGen/DaStGen.jar"
  project.datamodel.add_vertex( dastgen_model )
else:
  dastgen_model = peano4.datamodel.DaStGen2( "MG" )
  dastgen_model.data.add_attribute( dastgen2.attributes.Double("u") )
  dastgen_model.data.add_attribute( dastgen2.attributes.Double("rhs") )
  dastgen_model.data.add_attribute( dastgen2.attributes.Double("eps") )
  dastgen_model.data.add_attribute( dastgen2.attributes.Double("diag") )
  dastgen_model.data.add_attribute( dastgen2.attributes.Double("res") )
  dastgen_model.data.add_attribute( dastgen2.attributes.Enumeration("vertexType", ["Boundary", "Inside"]) )

  project.datamodel.add_vertex( dastgen_model )



#
# First, lets create the initial grid (which is regular).
#
create_grid = peano4.solversteps.Step( "CreateGrid", False )
create_grid.use_vertex( dastgen_model )
#create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(0.9) )
create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(0.05) )
#create_grid.add_action_set( peano4.toolbox.CreateRegularGrid(0.02) )
project.solversteps.add_step(create_grid)


#
# Set up the rhs and the material parameters, boundary conditions, and, finally,
# also the initial guess. For this, we use the default constructor value True,
# i.e. we ask the API to give us a C++ routine.
#
setup_scenario = peano4.solversteps.Step( "SetupScenario" )
setup_scenario.use_vertex( dastgen_model )
project.solversteps.add_step(setup_scenario)


plot_material_parameter = peano4.solversteps.Step( "PlotMaterialParameter", False )
plot_material_parameter.use_vertex( dastgen_model )
plot_material_parameter.add_action_set( peano4.toolbox.PlotScalarNodalFieldInPeanoBlockFormat("epsilon",dastgen_model,"getEps()") )
project.solversteps.add_step(plot_material_parameter)


#
# We create the steps to compute the residual and to update the solution. This 
# time, we create our corresponding user mappings explicitly, as we will use 
# them later on again.
#
compute_residual = peano4.solversteps.Step( "ComputeResidualWithGeometricOperators" )
compute_residual.use_vertex( dastgen_model )
project.solversteps.add_step( compute_residual )

jacobi_update = peano4.solversteps.Step( "JacobiUpdate" )
jacobi_update.use_vertex( dastgen_model )
project.solversteps.add_step( jacobi_update )


#
# As we solve only benchmarks with an analytical solution, we can really
# compute the residual plus the error 
#
compute_global_residual_and_error = peano4.solversteps.Step( "ComputeGlobalResidualAndError" )
compute_global_residual_and_error.use_vertex( dastgen_model )
project.solversteps.add_step( compute_global_residual_and_error )


#
# Finally, plot the grid, and plot the solution, too.
#
plot_solution = peano4.solversteps.Step( "PlotSolution", False )
plot_solution.use_vertex( dastgen_model )
plot_solution.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat("grid",None) )
plot_solution.add_action_set( peano4.toolbox.PlotScalarNodalFieldInPeanoBlockFormat("solution",dastgen_model,"getU()") )
project.solversteps.add_step(plot_solution)



#
# Created the fused version of the steps
#    
fused_step = peano4.solversteps.Step( "FusedSolverSteps", False )
fused_step.use_vertex( dastgen_model )
fused_step.copy_action_sets_from_other_step( jacobi_update )
fused_step.copy_action_sets_from_other_step( compute_residual )
fused_step.copy_action_sets_from_other_step( compute_global_residual_and_error )
project.solversteps.add_step( fused_step )



#
# Peano's API does not know which settings to use on the present system. To 
# make it copy/clone the settings identified by ./configure, we ask it to 
# parse the generated configuration scripts. The makefile would also offer a
# routine to set the dimension. We take the default here.
#
project.output.makefile.parse_configure_script_outcome( "../../.." )

#
# Standard triad of operations. You can skip the first two steps if you want as 
# the script then will automatically invoke the previous steps. The other way 
# round, it is always admissible to only generate stuff, e.g., but to build and
# run the project through a command line
#
#project.output.makefile.add_library( "ToolboxFiniteElements2d_trace", project.output.makefile.get_source_path() + "/toolbox/finiteelements" )
#project.output.makefile.add_library( "ToolboxMultiprecision_trace", project.output.makefile.get_source_path() + "/toolbox/multiprecision" )
project.output.makefile.add_library( "ToolboxFiniteElements2d", project.output.makefile.get_source_path() + "/toolbox/finiteelements" )
project.output.makefile.add_library( "ToolboxMultiprecision", project.output.makefile.get_source_path() + "/toolbox/multiprecision" )
project.output.makefile.add_header_search_path( "/opt/tbb/include" ) 
project.output.makefile.set_dimension( 2 )
#project.output.makefile.set_mode( peano4.output.CompileMode.Debug )
project.output.makefile.set_mode( peano4.output.CompileMode.Asserts )
#project.output.makefile.set_mode( peano4.output.CompileMode.Release )
project.generate(peano4.output.Overwrite.Default)
project.build(True)
start_time_stamp = time.time()
#run_successful = project.run( ["4"] )
run_successful = project.run( ["1"], ["mpirun", "-n", "2"] )
print( "Runtime: " + str(time.time()-start_time_stamp) + "s" )





if run_successful:
  #
  # Convert data into vtk, so we can open it in Paraview  
  #
  convert = peano4.visualisation.Convert( "epsilon" )
  convert.set_visualisation_tools_path( "../../../visualisation" )
  #convert.set_visualisation_tools_path( "/home/tobias/git/Peano/src/visualisation", "/opt/mpi/mpirun" )
  convert.extract_fine_grid()
  convert.convert_to_vtk()

  convert.set_input_file_name( "solution" )
  convert.extract_fine_grid()
  convert.convert_to_vtk()


