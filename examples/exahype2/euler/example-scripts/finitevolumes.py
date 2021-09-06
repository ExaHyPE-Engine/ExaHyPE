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
import os, sys
import peano4
import exahype2
import argparse


print( """
Please call this script from the directory hosting the Makefile and the
sources. Typically, I invoke the script via

python3 example-scripts/finitevolumes-with-ExaHyPE2-benchmark.py arguments
""")

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-lb", "--load-balancing-quality", dest="load_balancing_quality", type=float, default=0.9, help="Load balancing quality (something between 0 and 1; 1 is optimal)" )
parser.add_argument("-cs", "--cell-size",              dest="h",              type=float, required=True, help="Mesh size" )
parser.add_argument("-j",  "--parallel-builds",        dest="j",              type=int, default=4, help="Parallel builds" )
parser.add_argument("-d",  "--dim",                    dest="dim",            type=int, default=2, help="Dimensions" )
parser.add_argument("-m",  "--mode",                   dest="mode",                     default="release", help="|".join(modes.keys()) )
parser.add_argument("-et", "--end-time",               dest="end_time",                 type=float, default=1.0, help="Number of timesteps" )
parser.add_argument("-pd", "--peano-dir",              dest="peanodir",                 default="../../../", help="Peano4 directory" )
parser.add_argument("-cd", "--configure-dir",          dest="configuredir",             default="../../../", help="Location of configure" )
parser.add_argument("-o",  "--output",                 dest="out",                      default="peano4", help="Executable name" )
parser.add_argument("-f",  "--force",                  dest="force",                    default=False, action="store_true", help="Allow overwriting of output file" )
parser.add_argument("-t",   "--type",                  dest="type",                     choices=["global-fixed", "global-adaptive", "global-fixed-enclave", "global-adaptive-enclave", "global-fixed-enclave-gpu", "global-adaptive-enclave-gpu"], default="global-fixed", help="Pick implementation variant" )
parser.add_argument("-pdt", "--plot-dt",               dest="plot_snapshot_interval", default=0, help="Time interval in-between two snapshots (switched off by default")
parser.add_argument("-v",   "--verbose",               dest="verbose",          action="store_true", default=False, help="Verbose")
parser.add_argument("-ps",  "--patch-size",            dest="patch_size",       type=int, default=17, help="Dimensions" )
parser.add_argument("-amr", "--adaptive-levels",       dest="adaptivity_levels",        type=int, default=0, help="Number of AMR grid levels on top of hmax (0 by default)" )
parser.add_argument("-pbc", "--periodic-boundary-conditions",  dest="periodic_boundary_conditions",   action="store_true", help="Use periodic boundary conditions" )
parser.add_argument("--no-compile",                    dest="compile",          action="store_false", default="True", help="Compile (on by default)" )
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    import sys
    sys.exit(1)

if args.mode not in modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ",join(modes.keys()[:-1]),modes.keys()[-1],args.mode))
    import sys
    sys.exit(1)

if args.out is not None and os.path.exists(args.out) and not args.force:
    print("Not overwriting existing output file name {}. Use -f or --force to overwrite nevertheless.".format(args.out))
    sys.exit(1)

print("\nConfiguring {}D Euler problem. Buildmode is {}, nbuilds={}.\n".format(args.dim, args.mode, args.j))
print("Executable: {}".format(args.out))

#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", ".", executable=args.out )


#
# Add the Finite Volumes solver
#
unknowns       = 5
time_step_size = 0.000001
max_h          = args.h

min_h          = 0.9 * args.h * 3.0**(-args.adaptivity_levels)


admissible_time_step_size = min_h/args.patch_size*0.01

#
# Still the same solver, but this time we use named arguments. This is the way
# you can add further PDE terms btw.
#
#

auxiliary_variables = 0

thesolver = None
if args.type=="global-fixed":
  thesolver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    admissible_time_step_size,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
  )
elif args.type=="global-adaptive":
  thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    time_step_relaxation = 0.01
  )
elif args.type=="global-fixed-enclave":
  thesolver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    admissible_time_step_size,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation
  )
elif args.type=="global-adaptive-enclave":
  thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    time_step_relaxation = 0.01
  )
elif args.type=="global-fixed-enclave-gpu":
  thesolver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    admissible_time_step_size,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    use_gpu = True
  )
elif args.type=="global-adaptive-enclave-gpu":
  thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
    "Euler",
    args.patch_size,
    unknowns, auxiliary_variables,
    min_h, max_h,
    flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
    time_step_relaxation = 0.01,
    use_gpu = True
  )


thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )


project.add_solver( thesolver )


dimensions = args.dim
build_mode = modes[args.mode]


if args.periodic_boundary_conditions:
  periodic_BC = [True, True, True]
else:
  periodic_BC = [False, False, False]


#
# Lets configure some global parameters
#
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  end_time = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_snapshot_interval,      # snapshots
  periodic_BC = periodic_BC
)



#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "(" + str(args.load_balancing_quality) + ",false)" )
project.set_Peano4_installation( args.peanodir, build_mode )
peano4_project = project.generate_Peano4_project(args.verbose)
#peano4_project.output.makefile.parse_configure_script_outcome( args.configuredir )
if args.compile:
  peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
else:
  peano4_project.generate()


print("Done. Executable is: {}".format(args.out))
print( "Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-Euler.peano-patch-file")
