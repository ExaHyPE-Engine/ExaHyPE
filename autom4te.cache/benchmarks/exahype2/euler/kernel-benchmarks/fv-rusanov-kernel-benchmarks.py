import os, sys
import argparse

import peano4
import exahype2

print("""

ExaHyPE 2 - Euler Finite Volume Rusanov Kernel Benchmarks

@author 2022 Tobias Weinzierl, Mario Wille

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler FV Rusanov Kernel Benchmarking Script')
parser.add_argument("-j",   "--parallel-builds",   dest="j",              type=int, default=8, help="Parallel builds")
parser.add_argument("-d",   "--dim",               dest="dim",            type=int, default=2, help="Number of space dimensions")
parser.add_argument("-pd",  "--peano-dir",         dest="peanodir",       default="../../../../", help="Peano4 directory")
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Verbose")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=17, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-a",   "--accuracy",          dest="accuracy",       type=float, default=0.0, help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check.")
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

dimensions = args.dim
project = exahype2.Project( ["benchmarks", "exahype2", "euler"], "EulerFVRusanovKernelBenchmarks", ".", executable="kernel-benchmark-fv-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size) )

unknowns            = 1+dimensions+1
max_h               = 0.01
min_h               = 0.01
auxiliary_variables = 0

solver_name = "EulerFVRusanov"

fv_rusanov_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  solver_name,
  args.patch_size,
  unknowns, auxiliary_variables,
  min_volume_h = min_h,
  max_volume_h = max_h,
  time_step_relaxation = 0.01,
  flux = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  eigenvalues = exahype2.solvers.fv.PDETerms.User_Defined_Implementation,
  pde_terms_without_state = True
)

project.add_solver(fv_rusanov_solver)

if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release

if args.dim==2:
  offset = [0.0,0.0]
  size = [1.0,1.0]
else:
  offset = [0.0,0.0,0.0]
  size = [1.0,1.0,1.0]
  
project.set_global_simulation_parameters(
  dimensions   = dimensions,
  offset       = offset,
  size         = size,
  min_end_time = 1.0,
  max_end_time = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.constants.export_const_with_type( "Accuracy", str(args.accuracy), "double" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/euler/EulerKernels.cpp" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/euler" )
peano4_project.generate()
#os.system( "make clean" )
#if args.j>0:
#  os.system( "make -j{}".format(args.j) )
#else:
#  os.system( "make -j" )
