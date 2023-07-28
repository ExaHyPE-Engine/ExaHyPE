import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))

import peano4
import exahype2

dimensions                = 2
cell_size                 = 0.01
patch_size                = 17
max_patches               = 1024*2*2*2*2
end_time                  = 3
output_path               = "solution"
use_make                  = True
with_scaling              = False
use_symbolic_description  = False
plot_interval             = end_time/100 # Set to 0 to disable I/O
accuracy                  = -1.0 # Set to value <0 to set to machine precision. Set to 0 to disable accuracy checks.

scenarios = [
  "PointExplosion",
  "PointExplosionWithDynamicAMR",
  "GaussianExplosion",
  "GaussianExplosionWithDynamicAMR",
  "BreakingDam",
  "BreakingDamWithDynamicAMR",
]
scenario = "GaussianExplosion"

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
parser.add_argument("-d",       "--dim",                dest="dim",               type=int,                     default=dimensions,         help="Number of space dimensions")
parser.add_argument("-sc",      "--scenario",           dest="scenario",          choices=scenarios,            default=scenario,           help="|".join(scenarios))
parser.add_argument("-maxh",    "--max-cell-size",      dest="max_cell_size",     type=float,                   default=cell_size,          help="Maximum size of a single finite volume cell on each axis.")
parser.add_argument("-ps",      "--patch-size",         dest="patch_size",        type=int,                     default=patch_size,         help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-mp",      "--max-patches",        dest="max_patches",       type=int,                     default=max_patches,        help="Maximal number of patches to study")
parser.add_argument("-et",      "--end-time",           dest="end_time",          type=int,                     default=end_time,           help="End time of simulation")
parser.add_argument("-pi",      "--plot-interval",      dest="plot_interval",     type=float,                   default=plot_interval,      help="Time between plots and also between database entries when probes are used.")
parser.add_argument("-a",       "--accuracy",           dest="accuracy",          type=float,                   default=accuracy,           help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).")
parser.add_argument(            "--with-scaling",       dest="with_scaling",      action="store_true",          default=with_scaling,       help="Benchmark kernels according to scaling laws. When switched off, each thread replicates the problem size (off by default).")
parser.add_argument("-sym",     "--symbolic",           dest="symbolic",          action="store_true",          default=use_symbolic_description, help="Use the symbolic PDE description of the Euler equations.")
parser.add_argument(            "--make",               dest="use_make",          action="store_true",          default=use_make,           help="Compile the code after generation")
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

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
  executable    = "kernel-benchmarks-fv-rusanov-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size) + "-" + str(args.scenario)
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name                    = "FVRusanovSolver",
  patch_size              = args.patch_size,
  unknowns                = args.dim + 2,
  auxiliary_variables     = 0,
  min_volume_h            = args.max_cell_size,
  max_volume_h            = args.max_cell_size,
  time_step_relaxation    = 0.5,
  #normalised_time_step_size = args.max_cell_size/args.patch_size*0.5,
  pde_terms_without_state = True
)

sys.path.append('../../../../tutorials/exahype2/euler')
sys.path.append('../../../../applications/exahype2/euler')
if args.scenario == "PointExplosion":
  from point_explosion import PointExplosion
  my_scenario = PointExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "PointExplosionWithDynamicAMR":
  from point_explosion_with_dynamic_amr import PointExplosionWithDynamicAMR
  my_scenario = PointExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "GaussianExplosion":
  from gaussian_explosion import GaussianExplosion
  my_scenario = GaussianExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "GaussianExplosionWithDynamicAMR":
  from gaussian_explosion_with_dynamic_amr import GaussianExplosionWithDynamicAMR
  my_scenario = GaussianExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "BreakingDam":
  from breaking_dam import BreakingDam
  my_scenario = BreakingDam(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "BreakingDamWithDynamicAMR":
  from breaking_dam_with_dynamic_amr import BreakingDamWithDynamicAMR
  my_scenario = BreakingDamWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)

my_solver.set_implementation(
    initial_conditions    = my_scenario.implementation_of_initial_conditions(),
    boundary_conditions   = my_scenario.implementation_of_boundary_conditions(),
    flux                  = my_scenario.implementation_of_flux(),
    eigenvalues           = my_scenario.implementation_of_max_eigenvalue(),
    refinement_criterion  = my_scenario.implementation_of_refinement_criterion(),
    source_term           = my_scenario.implementation_of_sources(),
)

if args.symbolic:
  my_solver.plot_description = my_scenario.pde.unknown_identifier_for_plotter()
else:
  if args.dim == 2:
    my_solver.plot_description = "rho, u, v, e"
  else:
    my_solver.plot_description = "rho, u, v, w, e"
  my_solver.add_user_solver_includes("""
#include "../../../../tutorials/exahype2/euler/EulerKernels.h"
""")

my_project.add_solver(my_solver)

if args.dim == 2:
  size                    = [1.0, 1.0]
  offset                  = [0.0, 0.0]
else:
  size                    = [1.0, 1.0, 1.0]
  offset                  = [0.0, 0.0, 0.0]

my_project.set_global_simulation_parameters(
    dimensions=args.dim,
    size=size,
    offset=offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=args.plot_interval,
    periodic_BC           = [False, False, False]
)

output_path = str(args.output_path) + "-" + str(args.scenario)
my_project.set_output_path(output_path)
if not os.path.exists(output_path):
  os.makedirs(output_path)

my_project.set_Peano4_installation("../../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)
my_project.output.makefile.add_cpp_file("KernelBenchmarksFVRusanov-main.cpp")
my_project.constants.export_boolean("WithScaling", args.with_scaling)
my_project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")
my_project.constants.export_constexpr_with_type("MaxPatches", str(args.max_patches), "int")
my_project.constants.export_constexpr_with_type("EndTime", str(args.end_time), "int")
my_project.build(make=args.use_make, make_clean_first=True, throw_away_data_after_build=True)

if args.symbolic:
  print("\nPDE is: ")
  print(my_scenario.pde.__str__())

print("Executable is kernel-benchmarks-fv-rusanov-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size) + "-" + str(args.scenario))
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 kernel-benchmarks-fv-rusanov.py' again")
