import os, sys
import argparse

import peano4
import exahype2

print("""

ExaHyPE 2 - Euler Finite Volume Rusanov Kernel Benchmarks

@author 2022 Tobias Weinzierl, Mario Wille

""")

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler FV Rusanov Kernel Benchmarking Script')
parser.add_argument("-d",   "--dim",               dest="dim",            type=int, default=2, help="Number of space dimensions")
parser.add_argument("-v",   "--verbose",           dest="verbose",        action="store_true", default=False, help="Verbose mode")
parser.add_argument(        "--debug",             dest="debug"  ,        action="store_true", default=False, help="Debug mode")
parser.add_argument(        "--managed",           dest="managed_memory", action="store_true", default=False, help="Use managed memory kernel variants")
parser.add_argument("-ps",  "--patch-size",        dest="patch_size",     type=int, default=17, help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-a",   "--accuracy",          dest="accuracy",       type=float, default=0.0, help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon.")
parser.add_argument("-i",   "--iterations",        dest="iterations",     type=int, default=100, help="Number of iterations to be done for each kernel invocation. Used for warmup and to give a realistic measurement.")
args = parser.parse_args()

if args.dim not in [2,3]:
    print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
    sys.exit(1)

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

project = exahype2.Project( ["benchmarks", "exahype2", "euler"], "EulerFVRusanovKernelBenchmarks", ".", executable="kernel-benchmark-fv-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size) )

fv_rusanov_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name = "EulerFVRusanov",
  patch_size = args.patch_size,
  unknowns = 1+args.dim+1,
  auxiliary_variables = 0,
  min_volume_h = 0.01,
  max_volume_h = 0.01,
  time_step_relaxation = 0.01,
  pde_terms_without_state = True
)

fv_rusanov_solver.add_user_action_set_includes( """
#include "EulerKernels.h"
""")
fv_rusanov_solver.add_user_solver_includes( """
#include "EulerKernels.h"
""")

fv_rusanov_solver.set_implementation(
  boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  ncp                  = exahype2.solvers.fv.PDETerms.None_Implementation,
  flux                 = """
  ::applications::exahype2::euler::flux(
    Q,
    faceCentre, volumeH, t, dt, normal,
    NumberOfUnknowns, NumberOfAuxiliaryVariables,
    F
  );
""",
  source_term          = exahype2.solvers.fv.PDETerms.None_Implementation,
  refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  eigenvalues          = """
  return ::applications::exahype2::euler::maxEigenvalue(
    Q,
    faceCentre, volumeH, t, dt, normal,
    NumberOfUnknowns, NumberOfAuxiliaryVariables
  );
""",
  initial_conditions   = """
  printf("Initial conditions should not be called in this benchmark!");
  std::abort();
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
"""
)

project.add_solver(fv_rusanov_solver)

if args.dim==2:
  offset = [0.0,0.0]
  size = [1.0,1.0]
else:
  offset = [0.0,0.0,0.0]
  size = [1.0,1.0,1.0]
  
project.set_global_simulation_parameters(
  dimensions   = args.dim,
  offset       = offset,
  size         = size,
  min_end_time = 1.0,
  max_end_time = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC = [False, False, False]
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )

if args.debug:
  build_mode = peano4.output.CompileMode.Debug
else:
  build_mode = peano4.output.CompileMode.Release
project.set_Peano4_installation( "../../../../", build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)

peano4_project.constants.export_constexpr_with_type( "Accuracy", str(accuracy), "double" )
peano4_project.constants.export_constexpr_with_type( "Iterations", str(args.iterations), "int" )
peano4_project.constants.export_boolean( "EnableManagedMemory", args.managed_memory )

peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/euler" )

peano4_project.generate()
peano4_project.build( make_clean_first = True)

print("Done")
