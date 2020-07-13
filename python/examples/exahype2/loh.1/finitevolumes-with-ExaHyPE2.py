#!/usr/bin/env python3
"""
A more sophisticated example, which uses an FV solver with
nonconservative product. This example simulates elastic wave 
equations plus a diffuse interface 
model that takes material interfaces into account.

Currently, we impose a Gaussian velocity pulse as initial solution
but we will eventually simulate the LOH.1 benchmark.

We solve the governing equations using (a very diffusive) first-order FV solver.
The code relies on snippets from the first version 
of ExaHyPE and from ExaSeis.

* Fore more details on the underlying mathematical modelling, see:
  https://www.sciencedirect.com/science/article/pii/S0021999119300786 

* For more details on the benchmark problem, see:
  http://sismowine.org/model/WP2_LOH1.pdf

* For more details on ExaSeis, see e.g.:
  https://mediatum.ub.tum.de/doc/1483700/1483700.pdf 
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
output_files = [ f for f in os.listdir(".") if f.endswith(".peano-patch-file") or f.endswith(".vtu") ]
for f in output_files:
  os.remove(f)

#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( 
  namespace    = ["examples", "exahype2", "finitevolumes"], 
  project_name = "finitevolumes", 
  directory    = "." )

#
# Add the Finite Volumes solver with (flux and ncp)
#
project.add_solver(  
  exahype2.solvers.GenericRusanovFVFixedTimeStepSizeWithEnclaves(
  #exahype2.solvers.GenericRusanovFVFixedTimeStepSize(
    name           = "LOH1", 
    patch_size     = 5, 
    #patch_size     = 14,
    unknowns       = 3+6+3+1, # 13, vel(3) + stress(6) + material parameters(3) + diffuse interface(1)  
    time_step_size = 0.01, 
    flux           = True, 
    ncp            = True) )


#build_mode = peano4.output.CompileMode.Asserts
build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Trace


#
# Lets configure some global parameters
#
dimensions=3 
project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = [0.0]*dimensions, 
  size                  = [30.0]*dimensions,
  #end_time              = 2.0,        
  end_time              = 0.1,
  first_plot_time_stamp = 0.0, 
  #time_in_between_plots = 0.1
  time_in_between_plots = 0.0
)


#
# Parallelise
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(0.5)" )



peano4_project = project.generate_Peano4_project()
peano4_project.output.makefile.parse_configure_script_outcome( "../../../.." )
peano4_project.output.makefile.add_CXX_flag( "-g3" )
peano4_project.output.makefile.add_library( project.get_core_library(build_mode), "../../../../src/exahype2" )
peano4_project.output.makefile.add_library( "ToolboxLoadBalancing" + project.get_library_postfix(build_mode), "../../../../src/toolbox/loadbalancing" )
peano4_project.output.makefile.set_mode(build_mode)
peano4_project.generate( peano4.output.Overwrite.Default )

peano4_project.build( 
  make_clean_first = True 
)
success = peano4_project.run( ["--threads", "4"] )

#
# I usually prefer the variant through the command line:
# mkdir output
# rm output/*; ../../../../src/visualisation/convert apply-filter solutionLOH1.peano-patch-file LOH1Q output extract-fine-grid finegridLOH1Q; ../../../../src/visualisation/convert convert-file output/solutionLOH1.peano-patch-file finegridLOH1Q output vtu
#
#if success:
#  convert = peano4.visualisation.Convert( "solutionLOH1", True )
#  convert.set_visualisation_tools_path( "../../../../src/visualisation" )
#  convert.extract_fine_grid()
#  convert.convert_to_vtk()
