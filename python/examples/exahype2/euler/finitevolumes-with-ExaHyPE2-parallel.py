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
# Lets first clean up all plot files, so we don't get a mismatch
#
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") or f=="peano4"]
for f in output_files:
  os.remove(f)


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", "." )


#
# Add the Finite Volumes solver
#
patch_size     = 13
#patch_size     = 3
unknowns       = 5
time_step_size = 0.0001
#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#project.add_solver(  exahype2.solvers.GenericRusanovFVFixedTimeStepSizeWithEnclaves(
project.add_solver(  exahype2.solvers.GenericRusanovFVFixedTimeStepSize(
  "Euler", 
  patch_size, 
  unknowns, time_step_size,
  flux = True,
  ncp  = False,
  plot_grid_properties = True
))



dimensions = 2
#build_mode = peano4.output.CompileMode.Release
build_mode = peano4.output.CompileMode.Asserts


#
# Lets configure some global parameters
#
if dimensions==2:
  project.set_global_simulation_parameters(
    dimensions,  [0.0,0.0],  [1.0,1.0],
    0.01,           # end time
    0.0, 0.001      # snapshots
  )
else:
  project.set_global_simulation_parameters(
    dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
    0.01,           # end time
    0.0, 0.001      # snapshots
  )


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(0.6)" )
#project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision" )


peano4_project = project.generate_Peano4_project()
if dimensions==2 and build_mode == peano4.output.CompileMode.Release:
  peano4_project.constants.export( "MaxHOfVolume", 0.001 )
elif dimensions==2:
  #peano4_project.constants.export( "MaxHOfVolume", 0.01 )
  peano4_project.constants.export( "MaxHOfVolume", 0.1 )
  #peano4_project.constants.export( "MaxHOfVolume", 0.2 )
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.add_library( "ToolboxLoadBalancing" + project.get_library_postfix(build_mode), "../../../../src/toolbox/loadbalancing" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.generate( throw_away_data_after_generation=True )
peano4_project.build( number_of_parallel_builds=1 )



# Use this variant if you build without MPI. In principle, you can start
# your MPI test from the Python script. My Python installation however
# often crashes for bigger runs. It also struggles with the postprocessing
# for large parallel runs (see below).
#
if build_mode == peano4.output.CompileMode.Asserts:
  success = True
  if success:
    success = peano4_project.run( ["--threads", "1"] )
  if success:
    success = peano4_project.run( ["--threads", "2"] )
  if success:
    success = peano4_project.run( ["--threads", "4"] )
  #if success:
  #  success = peano4_project.run( ["--threads", "1"], ["mpirun", "-n", "2"] )
  #if success:
  #  success = peano4_project.run( ["--threads", "1"], ["mpirun", "-n", "4"] )
  #if success:
  #  success = peano4_project.run( ["--threads", "2"], ["mpirun", "-n", "4"] )
  #if success:
  #  success = peano4_project.run( ["--threads", "4"], ["mpirun", "-n", "4"] )
else:
  #success = peano4_project.run( ["--threads", "8"] )
  pass
  
  
#success = peano4_project.run( ["--threads", "2"], ["mpirun", "-n", "4"] )
#success = peano4_project.run( ["--threads", "28"], ["mpirun", "-n", "4"] )



success = False
if success:
  #
  # Command line version of these steps:
  #
  # ../../../../src/visualisation/convert apply-filter solutionEuler.peano-patch-file EulerQ . extract-fine-grid finegrid
  # ../../../../src/visualisation/convert convert-file solutionEuler.peano-patch-file finegrid . vtu
  #
  # The above command line options give you the solution. They do not plot the 
  # domain decomposition. If you are interested in the latter, you have to do 
  # the steps below. You have to run the first extraction step prior to any other
  # step. The convert script builds up a database and throws away meta data to get
  # the file size down, i.e. after the first filter applications you don't have 
  # the information anymore which tree wrote which part of the domain; unless you
  # explicitly extract it beforehand. 
  #
  # ../../../../src/visualisation/convert apply-filter solutionEuler.peano-patch-file EulerQ . plot-domain-decomposition DD
  # ../../../../src/visualisation/convert apply-filter solutionEuler.peano-patch-file DD     . extract-fine-grid finegridDD
  # ../../../../src/visualisation/convert convert-file solutionEuler.peano-patch-file finegridDD . vtu
  #
  # Here's the variant postprocessing script variant that I prefer on the command
  # line. It dumps all postprocessed data into a separate folder. This way, the 
  # original dump is not enriched/overwritten:
  #
  # mkdir output
  # rm output/*; ../../../../src/visualisation/convert apply-filter solutionEuler.peano-patch-file EulerQ output plot-domain-decomposition DD; ../../../../src/visualisation/convert apply-filter output/solutionEuler.peano-patch-file DD     output extract-fine-grid finegridDD; ../../../../src/visualisation/convert apply-filter output/solutionEuler.peano-patch-file EulerQ output extract-fine-grid finegridQ; ../../../../src/visualisation/convert convert-file output/solutionEuler.peano-patch-file all    output vtu
  #
  # ../../../../src/visualisation/convert apply-filter gridEuler.peano-patch-file cell-marker\(refined,local,enclave\) output extract-fine-grid finegrid-cell-marker\(refined,local,enclave\); ../../../../src/visualisation/convert convert-file output/gridEuler.peano-patch-file all    output vtu
  #
  convert = peano4.visualisation.Convert( "solutionEuler", True )
  convert.set_visualisation_tools_path( "../../../../src/visualisation" )
  convert.plot_domain_decomposition()
  convert.extract_fine_grid()
  convert.convert_to_vtk()

