import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))

import peano4
import exahype2

cell_size             = 0.01
patch_size            = 5
max_patches           = 1024*2
end_time              = 3
output_path           = "solution"
make                  = True
with_scaling          = False
plot_interval         = end_time/100 # Set to 0 to disable I/O
accuracy              = 0 # Set to value <0 to set to machine precision. Set to 0 to disable accuracy checks.

scenarios = [
  ""
]
scenario = ""

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}
build_mode = "Release"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Finite Volumes Rusanov Kernel Benchmarking Script')
parser.add_argument("-o",       "--output",             dest="output_path",       type=str,                     default=output_path,        help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.")
parser.add_argument("-m",       "--build-mode",         dest="build_mode",        choices=build_modes.keys(),   default=build_mode,         help="|".join(build_modes.keys()))
parser.add_argument("-sc",      "--scenario",           dest="scenario",          choices=scenarios,            default=scenario,           help="|".join(scenarios))
parser.add_argument("-maxh",    "--max-cell-size",      dest="max_cell_size",     type=float,                   default=cell_size,          help="Maximum size of a single finite volume cell on each axis.")
parser.add_argument("-ps",      "--patch-size",         dest="patch_size",        type=int,                     default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-mp",      "--max-patches",        dest="max_patches",       type=int,                     default=max_patches,        help="Maximal number of patches to study")
parser.add_argument("-s" ,      "--samples",            dest="samples",           type=int,                     default=16,                 help="Number of samples per measurement")
parser.add_argument("-et",      "--end-time",           dest="end_time",          type=int,                     default=end_time,           help="End time of simulation")
parser.add_argument("-pi",      "--plot-interval",      dest="plot_interval",     type=float,                   default=plot_interval,      help="Time between plots and also between database entries when probes are used.")
parser.add_argument("-a",       "--accuracy",           dest="accuracy",          type=float,                   default=accuracy,           help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).")
parser.add_argument(            "--with-scaling",       dest="with_scaling",      action="store_true",          default=with_scaling,       help="Benchmark kernels according to scaling laws. When switched off, each thread replicates the problem size (off by default)." )
parser.add_argument(            "--make",               dest="make",              action="store_true",          default=make,               help="Compile the code after generation")
args = parser.parse_args()

if args.build_mode not in build_modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ".join(list(build_modes.keys())[:-1]), list(build_modes.keys())[-1],args.build_mode))
    sys.exit(1)

if args.scenario not in scenarios:
    print("Error, scenario must be {} or {}, you supplied {}".format(", ".join(scenarios[:-1]), scenarios[-1], args.scenario))
    sys.exit(1)

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

my_project = exahype2.Project(
  namespace     = ["benchmarks", "exahype2", "kernelbenchmarks"],
  project_name  = "KernelBenchmarksFVRusanov",
  directory     = ".",
  executable    = "kernel-benchmarks-fv-rusanov-3d-patch-size-"+ str(args.patch_size)
)

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
  min_volume_h            = args.max_cell_size,
  max_volume_h            = args.max_cell_size,
  time_step_relaxation    = 0.5,
  #normalised_time_step_size = args.max_cell_size/args.patch_size*0.5,
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

my_project.add_solver(my_solver)

my_project.set_global_simulation_parameters(
  dimensions            = 3,
  offset                = [-0.5, -0.5, -0.5],
  size                  = [   1,    1,    1],
  min_end_time=args.end_time,
  max_end_time=args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots=args.plot_interval,
  periodic_BC           = [True, True, True]
)

output_path = str(args.output_path)
my_project.set_output_path(output_path)
if not os.path.exists(output_path):
  os.makedirs(output_path)

my_project.set_load_balancing("toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()")
my_project.set_Peano4_installation("../../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)

my_project.constants.export_boolean("WithScaling", args.with_scaling)
my_project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")
my_project.constants.export_constexpr_with_type("MaxPatches", str(args.max_patches), "int")
my_project.constants.export_constexpr_with_type("NumberOfSamples", str(args.samples), "int")
my_project.constants.export_constexpr_with_type("EndTime", str(args.end_time), "int")

my_project.output.makefile.add_cpp_file("../../../../applications/exahype2/ccz4/InitialValues.cpp")
my_project.output.makefile.add_cpp_file("../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp")
my_project.output.makefile.add_header_search_path("../../../../applications/exahype2/ccz4")
benchmark_main_file = "../../euler/kernel-benchmarks/KernelBenchmarksFVRusanov-main.cpp"
os.system("cp {} KernelBenchmarksFVRusanov-main.cpp".format(benchmark_main_file))

my_project.build(make=args.make, make_clean_first=True, throw_away_data_after_build=True)

print("Executable is kernel-benchmarks-fv-rusanov-3d-patch-size-"+ str(args.patch_size))
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 kernel-benchmarks-fv-rusanov.py' again")
