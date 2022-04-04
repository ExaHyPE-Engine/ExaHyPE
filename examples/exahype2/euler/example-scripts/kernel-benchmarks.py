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

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",  "--parallel-builds",        dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-d",  "--dim",                    dest="dim",            type=int, default=2, help="Dimensions" )
parser.add_argument("-pd", "--peano-dir",              dest="peanodir",                 default="../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",               dest="verbose",          action="store_true", default=False, help="Verbose")
parser.add_argument("-ps",  "--patch-size",            dest="patch_size",       type=int, default=17, help="Dimensions" )
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    import sys
    sys.exit(1)

#
# Create a project and configure it to end up in a subnamespace (and thus
# subdirectory). 
#
project = exahype2.Project( ["examples", "exahype2", "euler"], "finitevolumes", ".", executable="kernel-benchmark" )


#
# Add the Finite Volumes solver
#
solver_name = "EulerOnGPU"
unknowns       = 5
max_h          = 0.01
min_h          = 0.01
auxiliary_variables = 0

thesolver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  solver_name,
  args.patch_size,
  unknowns, auxiliary_variables,
  min_volume_h = 0.01, 
  max_volume_h = 0.01,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  pde_terms_without_state = True
)

thesolver.set_implementation( refinement_criterion=exahype2.solvers.fv.PDETerms.User_Defined_Implementation )

project.add_solver( thesolver )


#
# Configure build
#
dimensions = args.dim
build_mode = peano4.output.CompileMode.Release
project.set_global_simulation_parameters(
  dimensions = dimensions,
  offset = [0.0,0.0,0.0],
  size = [1.0,1.0,1.0],
  min_end_time = 1.0,
  max_end_time = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC           = [False, False, False]
)
project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )


#
# Generate code and then copy cpp file over main
#
peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.generate()
benchmark_main_file = os.path.dirname(os.path.realpath(__file__)) + "/kernel-benchmarks.cpp"
print( "replace generated main with benchmark main file {}".format(benchmark_main_file) )
os.system( "cp {} finitevolumes-main.cpp".format(benchmark_main_file) )
os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )



print("Done")
