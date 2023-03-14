import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))

import peano4
import exahype2

dimensions            = 2
max_patches           = 1024*2
patch_size            = 5
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
parser.add_argument("-ps",      "--patch-size",         dest="patch_size",        type=int,                     default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-mp",      "--max-patches",        dest="max_patches",       type=int,                     default=max_patches,        help="Maximal number of patches to study")
parser.add_argument("-t",       "--timesteps",          dest="timesteps",         type=int,                     default=timesteps,          help="Number of timesteps")
parser.add_argument("-a",       "--accuracy",           dest="accuracy",          type=float,                   default=-1.0,               help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).")
parser.add_argument(            "--with-scaling",       dest="with_scaling",      action="store_true",          default=False,              help="Benchmark kernels according to scaling laws. When switched off, each thread replicates the problem size (off by default)." )
parser.add_argument(            "--plot-average",       dest="plot_average",      action="store_true",          default=False,              help="Plot the average time spent in compute kernels instead of the minimum time." )
parser.add_argument(            "--no-compile",         dest="compile",           action="store_false",         default=True,               help="Compile (on by default)" )
args = parser.parse_args()

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

unknowns = {
  "G":6,
  "K":6,
  "theta":1,
  "Z":3,
  "lapse":1,
  "shift":3,
  "b":3,
  "dLapse":3,
  "dxShift":3,
  "dyShift":3,
  "dzShift":3,
  "dxG":6,
  "dyG":6,
  "dzG":6,
  "traceK":1,
  "phi":1,
  "P":3,
  "K0":1,
}

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name                    = "FVRusanovSolver",
  patch_size              = args.patch_size,
  unknowns                = sum(unknowns.values()),
  auxiliary_variables     = 0,
  min_volume_h            = 0.01,
  max_volume_h            = 0.01,
  time_step_relaxation    = 0.01,
  pde_terms_without_state = True
)

integer_constants = {
  "CCZ4LapseType":0,
}
double_constants = {
  "CCZ4ds":   1.0,
  "CCZ4c":    1.0,
  "CCZ4e":    1.0,
  "CCZ4f":    0.75,
  "CCZ4bs":   0.0,
  "CCZ4sk":   0.0,
  "CCZ4xi":   1.0,
  "CCZ4itau": 1.0,
  "CCZ4eta":  1.0,
  "CCZ4k1":   0.1,
  "CCZ4k2":   0.0,
  "CCZ4k3":   0.5,
  "CCZ4GLMc": 1.2,
  "CCZ4GLMd": 2.0,
  "CCZ4mu":   0.2,
}

for key, value in integer_constants.items():
  my_solver.add_solver_constants("static constexpr int {} = {};".format(key, value))
for key, value in double_constants.items():
  my_solver.add_solver_constants("static constexpr double {} = {};".format(key, value))

my_solver.add_user_action_set_includes( """
#include "CCZ4Kernels.h"
""")
my_solver.add_user_solver_includes( """
#include "CCZ4Kernels.h"
#include "InitialValues.h"
#include <cstring>
""")

my_solver.set_implementation(
  boundary_conditions  = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  ncp                  = """
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
  flux                 = exahype2.solvers.fv.PDETerms.None_Implementation,
  source_term          = """
  memset(S, 0.0, NumberOfUnknowns*sizeof(double));
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
  refinement_criterion = exahype2.solvers.fv.PDETerms.Empty_Implementation,
  eigenvalues          = """
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );
""",
  initial_conditions   = """
  printf("Initial conditions should not be called in this benchmark!");
  std::abort();
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0;
  ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
"""
)

my_project = exahype2.Project(
  namespace     = ["benchmarks", "exahype2", "kernelbenchmarks"],
  project_name  = "KernelBenchmarksFVRusanov",
  directory     = ".",
  executable    = "kernel-benchmarks-fv-rusanov-3d-patch-size-"+ str(args.patch_size)
)

my_project.add_solver(my_solver)
my_project.set_load_balancing("toolbox::loadbalancing::RecursiveSubdivision","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )
my_project.set_Peano4_installation("../../../../", build_modes[args.build_mode])

my_project.set_global_simulation_parameters(
  dimensions            = 3,
  offset                = [-0.5, -0.5, -0.5],
  size                  = [   1,    1,    1],
  min_end_time          = 1.0,
  max_end_time          = 1.0,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = 0.0,
  periodic_BC           = [True,True,True]
)

peano4_project = my_project.generate_Peano4_project(verbose=True)
peano4_project.constants.export_boolean("WithScaling", args.with_scaling)
peano4_project.constants.export_boolean("PlotAverage", args.plot_average)
peano4_project.constants.export_constexpr_with_type( "Accuracy", str(accuracy), "double" )
peano4_project.constants.export_constexpr_with_type( "MaxPatches", str(args.max_patches), "int" )
peano4_project.constants.export_constexpr_with_type( "TimeSteps", str(args.timesteps), "int" )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/InitialValues.cpp", generated=False )
peano4_project.output.makefile.add_cpp_file( "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp", generated=False )
peano4_project.output.makefile.add_header_search_path( "../../../../applications/exahype2/ccz4" )
benchmark_main_file = "../../euler/kernel-benchmarks/KernelBenchmarksFVRusanov-main.cpp"
os.system( "cp {} KernelBenchmarksFVRusanov-main.cpp".format(benchmark_main_file) )
peano4_project.output.makefile.add_cpp_file( "KernelBenchmarksFVRusanov-main.cpp", generated=True )
peano4_project.generate()
if(args.compile):
  peano4_project.build( make_clean_first=True, throw_away_data_after_build=True)
