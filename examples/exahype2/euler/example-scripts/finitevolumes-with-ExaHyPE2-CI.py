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
import exahype2
import argparse



parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("--h",              dest="h",              type=float, help="Mesh size",  default=0.01 )
parser.add_argument("--d",              dest="dim",            type=int,   help="Dimensions", default=2 )
parser.add_argument("--s",              dest="patchsize",      type=int,   help="Patch size", default=17 )
parser.add_argument("--p",              dest="plot",     action="store_true",  help="Plot final solution", default=False )
parser.add_argument("--n",              dest="number_of_time_steps",  type=int,  help="Number of timesteps", default=10 )
parser.add_argument("--gpu",            dest="gpu",    action="store_true",      help="Use GPU", default=False )
parser.add_argument("--m",               dest="mode",                     default="release", help="release|trace|asserts" )
args = parser.parse_args()

if args.mode not in ["release", "trace", "asserts"]:
    print("Error, mode must be release, trace or assert, you supplied {}".format(args.mode))
    parser.print_help()
    import sys
    sys.exit(1)

#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", "." )


#
# Add the Finite Volumes solver
#
patch_size     = args.patchsize
unknowns       = 5
time_step_size = 0.000001
min_h          = args.h
max_h          = args.h

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
solver = None

if args.gpu:
  solver = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithAccelerator(
    "EulerOnGPU",
    patch_size,
    unknowns, 0,
    min_h, max_h,
    time_step_size,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
  )
else:
  solver = exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves(
    "Euler",
    patch_size,
    unknowns, 0,
    min_h, max_h,
    time_step_size,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
  )



project.add_solver( solver );


dimensions = args.dim
if (args.mode=="release"):
  build_mode = peano4.output.CompileMode.Release
elif (args.mode=="trace"):
  build_mode = peano4.output.CompileMode.Trace
else:
  build_mode = peano4.output.CompileMode.Asserts

#
# Lets configure some global parameters
#
plot_interval = 0.0
if args.plot:
  plot_interval = 0.1
end_time = time_step_size * args.number_of_time_steps
project.set_global_simulation_parameters(
  dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
  end_time,           # end time
  end_time, plot_interval                        # snapshots
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision" )
project.set_Peano4_installation("../../..", build_mode)
peano4_project = project.generate_Peano4_project()
peano4_project.output.makefile.parse_configure_script_outcome( "../../.." )
if args.gpu:
  if args.mode != "release":
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/fv/libExaHyPE2Core{}d_{}_a-Generic.o".format(args.dim, args.mode) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/fv/libExaHyPE2Core{}d_{}_a-Rusanov.o".format(args.dim, args.mode) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/libExaHyPE2Core{}d_{}_a-PatchUtils.o".format(args.dim, args.mode) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/tarch/multicore/omp/libTarch_{}_a-multicore.o".format(args.mode) )
  else:
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/fv/libExaHyPE2Core{}d_a-Generic.o".format(args.dim) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/fv/libExaHyPE2Core{}d_a-Rusanov.o".format(args.dim) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/exahype2/libExaHyPE2Core{}d_a-PatchUtils.o".format(args.dim) )
    peano4_project.output.makefile.add_gpu_object( "../../../src/tarch/multicore/omp/libTarch_a-multicore.o")

peano4_project.generate()
#peano4_project.build(make_clean_first=True, number_of_parallel_builds=1)


