import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))

import peano4
import exahype2

dimensions            = 2
max_patches           = 1024*2*2*2*2
patch_size            = 17
timesteps             = 100

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Finite Volumes Rusanov Kernel Benchmarking Script')
parser.add_argument("-m",       "--build-mode",         dest="build_mode",        choices=build_modes.keys(),   default="Release",          help="|".join(build_modes.keys()) )
parser.add_argument("-d",       "--dim",                dest="dim",               type=int,                     default=dimensions,         help="Number of space dimensions")
parser.add_argument("-ps",      "--patch-size",         dest="patch_size",        type=int,                     default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-mp",      "--max-patches",        dest="max_patches",       type=int,                     default=max_patches,        help="Maximal number of patches to study")
parser.add_argument("-t",       "--timesteps",          dest="timesteps",         type=int,                     default=timesteps,          help="Number of timesteps")
parser.add_argument("-a",       "--accuracy",           dest="accuracy",          type=float,                   default=-1.0,               help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).")
parser.add_argument(            "--with-scaling",       dest="with_scaling",      action="store_true",          default=False,              help="Benchmark kernels according to scaling laws. When switched off, each thread replicates the problem size (off by default)." )
parser.add_argument(            "--plot-average",       dest="plot_average",      action="store_true",          default=True,               help="Plot the average time spent in compute kernels instead of the minimum time." )
parser.add_argument(            "--no-compile",         dest="compile",           action="store_false",         default=True,               help="Compile (on by default)" )
args = parser.parse_args()

if args.dim not in [2,3]:
  print("Number of dimensions must be 2 or 3, but you supplied {}".format(args.dim))
  sys.exit(1)

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name                    = "FVRusanovSolver",
  patch_size              = args.patch_size,
  unknowns                = 1+args.dim+1,
  auxiliary_variables     = 0,
  min_volume_h            = 0.01,
  max_volume_h            = 0.01,
  time_step_relaxation    = 0.01,
  pde_terms_without_state = True
)

my_solver.add_user_action_set_includes( """
#include "EulerKernels.h"
""")
my_solver.add_user_solver_includes( """
#include "EulerKernels.h"
""")

my_solver.set_implementation(
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

my_project = exahype2.Project(
  namespace     = ["benchmarks", "exahype2", "kernelbenchmarks"],
  project_name  = "KernelBenchmarksFVRusanov",
  directory     = ".",
  executable    = "kernel-benchmarks-fv-rusanov-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size)
)

my_project.add_solver(my_solver)
my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../../", build_modes[args.build_mode])

if args.dim==2:
  width   = [1.0,1.0]
  offset  = [0.0,0.0]
else:
  width   = [1.0,1.0,1.0]
  offset  = [0.0,0.0,0.0]

my_project.set_global_simulation_parameters(
  dimensions            = args.dim,
  size                  = width,
  offset                = offset,
  min_end_time          = 1.0,
  max_end_time          = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC           = [False, False, False]
)

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.constants.export_boolean("WithScaling", args.with_scaling)
peano4_project.constants.export_boolean("PlotAverage", args.plot_average)
peano4_project.constants.export_constexpr_with_type( "Accuracy", str(accuracy), "double" )
peano4_project.constants.export_constexpr_with_type( "MaxPatches", str(args.max_patches), "int" )
peano4_project.constants.export_constexpr_with_type( "TimeSteps", str(args.timesteps), "int" )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/euler" )
peano4_project.output.makefile.add_cpp_file( "KernelBenchmarksFVRusanov-main.cpp", generated=False )
peano4_project.generate()
if(args.compile):
  peano4_project.build( make_clean_first=True, throw_away_data_after_build=True)
