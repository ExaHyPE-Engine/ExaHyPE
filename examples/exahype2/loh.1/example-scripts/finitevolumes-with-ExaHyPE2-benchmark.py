"""

 A simple benchmark script for the LOH.1 problem solved via a generic
 Rusanov solver.

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os
import peano4
import exahype2
import argparse



parser = argparse.ArgumentParser(description='ExaHyPE 2 - Rusanov benchmarking script')
parser.add_argument("--h",              dest="h",              type=float, required=True, help="Mesh size" )
args = parser.parse_args()


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( 
  namespace    = ["examples", "exahype2", "loh1"], 
  project_name = "loh1", 
  directory    = "." )


#
# Add the Finite Volumes solver
#
mesh_size      = args.h
time_step_size = 0.0001

project.add_solver(  
  exahype2.solvers.GenericRusanovFVFixedTimeStepSizeWithEnclaves(
    name           = "LOH1", 
    patch_size     = 5, 
    unknowns       = 3+6,      # vel(3) + stress(6)
    auxiliary_variables = 4,   #material parameters(3) + diffuse interface(1)  
    time_step_size = 0.01, 
    min_h          = mesh_size,
    max_h          = mesh_size,
    flux           = False, 
    ncp            = True,
    use_gpu        = False) )



dimensions=3 
project.set_global_simulation_parameters(
  dimensions            = dimensions,
  offset                = [0.0]*dimensions, 
  size                  = [30.0]*dimensions,
  end_time              = 0.1,
  first_plot_time_stamp = 0.0, 
  time_in_between_plots = 0
)


project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(0.9)" )


build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation("../../..", build_mode)
peano4_project = project.generate_Peano4_project()
peano4_project.generate( throw_away_data_after_generation=False )

peano4_project.build(make_clean_first=True)

