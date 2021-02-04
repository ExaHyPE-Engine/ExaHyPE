"""

 A simple benchmark script for the LOH.1 problem solved via a generic
 Rusanov solver.

 This file should be invoked from the directory above. Ensure you've 
 set PYTHONPATH correctly before you kick off:

 export PYTHONPATH=../../../python/

"""


#
# We import Peano4 as project. If this step fails, ensure that your environment
# variable PYTHONPATH points to Peano4's python directory.
#
import os
import peano4
import exahype2
import argparse



parser = argparse.ArgumentParser(description='ExaHyPE 2 - LOH1')
parser.add_argument("--t",              dest="timestep",              type=float, help="Size of time step",  default=0.000001 )
parser.add_argument("--h",              dest="h",              type=float, help="Mesh size",  default=3 )
parser.add_argument("--d",              dest="dim",            type=int,   help="Dimensions", default=3 )
parser.add_argument("--s",              dest="patchsize",      type=int,   help="Patch size", default=5 )
parser.add_argument("--p",              dest="plot",     type=float,  help="Plot interval", default=None )
parser.add_argument("--n",              dest="number_of_time_steps",  type=int,  help="Number of timesteps", default=10 )
parser.add_argument("--gpu",            dest="gpu",    action="store_true",      help="Use GPU", default=False )
parser.add_argument("--nmax",           dest="nmax",           type=int,          default=0, help="Number of maximum tasks to grab" )
parser.add_argument("--m",              dest="mode",                     default="release", help="release|trace|asserts" )
args = parser.parse_args()


#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory).
#
project = exahype2.Project( namespace    = ["examples", "exahype2", "loh1"],   project_name = "loh1",   directory    = "." )


#
# Add the Finite Volumes solver
#
mesh_size      = args.h
time_step_size = args.timestep

if args.gpu:
  solver = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithAccelerator(
    name           = "LOH1OnGPU",
    patch_size     = args.patchsize,
    unknowns       = 3+6,      # vel(3) + stress(6)
    auxiliary_variables = 4,   #material parameters(3) + diffuse interface(1)
    time_step_size = time_step_size,
    min_h          = mesh_size,
    max_h          = mesh_size,
    flux = None,
    ncp            = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    ngrabmax=args.nmax
    )
else:
  solver = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves(
    name           = "LOH1",
    patch_size     = args.patchsize,
    unknowns       = 3+6,      # vel(3) + stress(6)
    auxiliary_variables = 4,   #material parameters(3) + diffuse interface(1)
    time_step_size = time_step_size,
    min_h          = mesh_size,
    max_h          = mesh_size,
    flux = None,
    ncp            = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
    )



project.add_solver( solver );






# project.add_solver(
  # exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves(
    # name           = "LOH1",
    # patch_size     = args.patchsize,
    # unknowns       = 3+6,      # vel(3) + stress(6)
    # auxiliary_variables = 4,   #material parameters(3) + diffuse interface(1)
    # time_step_size = time_step_size,
    # min_h          = mesh_size,
    # max_h          = mesh_size,
    # flux = None,
    # ncp            = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,

    # ngrabmax=args.nmax
    # ))


plot_interval = 0.0
if args.plot is not None:
  plot_interval = args.plot
  print("Will to {} snapshots ({} / {})".format(time_step_size * args.number_of_time_steps/args.plot, args.plot, time_step_size * args.number_of_time_steps))
end_time = time_step_size * args.number_of_time_steps


project.set_global_simulation_parameters(
  dimensions            = args.dim,
  offset                = [0.0]*args.dim,
  size                  = [30.0]*args.dim,
  end_time              = end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = plot_interval
)


project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(0.9)" )



if (args.mode=="release"):
  build_mode = peano4.output.CompileMode.Release
elif (args.mode=="trace"):
  build_mode = peano4.output.CompileMode.Trace
else:
  build_mode = peano4.output.CompileMode.Asserts


project.set_Peano4_installation("../../..", build_mode)
peano4_project = project.generate_Peano4_project()
peano4_project.generate( throw_away_data_after_generation=False )

peano4_project.build(make_clean_first=True, number_of_parallel_builds=6)

