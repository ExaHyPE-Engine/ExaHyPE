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


print( """
Please call this script from the directory hosting the Makefile and the 
sources. Typically, I invoke the script via

python3 example-scripts/finitevolumes-with-ExaHyPE2-benchmark.py arguments
""")


parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("--load-balancing-quality", dest="load_balancing_quality", type=float, required=True, help="Load balancing quality (something between 0 and 1; 1 is optimal)" )
parser.add_argument("--h",               dest="h",              type=float, required=True, help="Mesh size" )
parser.add_argument("--j",               dest="j",              type=int, default=4, help="Parallel builds" )
parser.add_argument("--d",               dest="dim",            type=int, default=2, help="Dimensions" )
parser.add_argument("--m",               dest="mode",                     default="release", help="release|trace|assert" )
args = parser.parse_args()

if args.dim not in [1,2]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    import sys
    sys.exit(1)

if args.mode not in ["release", "trace", "assert"]:
    print("Error, mode must be release, trace or assert, you supplied {}".format(args.mode))
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
patch_size     = 7
unknowns       = 5
time_step_size = 0.000001
min_h          = args.h
max_h          = args.h

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
project.add_solver(  exahype2.solvers.fv.GenericRusanovFixedTimeStepSizeWithEnclaves(
  "Euler",
  patch_size,
  unknowns, 0,
  min_h, max_h,
  time_step_size,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
))

# use_gpu =  False



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
project.set_global_simulation_parameters(
  dimensions, [0.0,0.0,0.0], [1.0,1.0,1.0],
  time_step_size * 50,           # end time
  0.0, 0                         # snapshots
)


#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(" + str(args.load_balancing_quality) + ")" )
project.set_Peano4_installation("../../..", build_mode)
peano4_project = project.generate_Peano4_project()
peano4_project.output.makefile.parse_configure_script_outcome( "../../.." )
from IPython import embed
embed()
peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)

