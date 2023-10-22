# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))
import peano4
import exahype2

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}
build_mode = "Release"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Finite Volumes Rusanov Kernel Benchmarking Script')
parser.add_argument("-m",       "--build-mode",         dest="build_mode",        choices=build_modes.keys(),   default="Release",          help="|".join(build_modes.keys()))
parser.add_argument("-d",       "--dim",                dest="dim",               type=int,                     required=True,              help="Number of space dimensions")
parser.add_argument("-ps",      "--patch-size",         dest="patch_size",        type=int,                     required=True,              help="Number of finite volumes per axis (dimension) per patch")
parser.add_argument("-mp",      "--max-patches",        dest="max_patches",       type=int,                     default=1024+1,             help="Maximal number of patches to study")
parser.add_argument("-s" ,      "--samples",            dest="samples",           type=int,                     default=16,                 help="Number of samples per measurement")
parser.add_argument("-a",       "--accuracy",           dest="accuracy",          type=float,                   default=0.0,                help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).")
parser.add_argument(            "--make",                                         action="store_true",                                      help="Compile the code after generation")
parser.add_argument(            "--no-make",            dest="use_make",          action="store_false",                                     help="Do not compile the code after generation")
parser.set_defaults(use_make=True)
parser.add_argument(            "--executable",         dest="executable",        type=str,                     default="kernel-benchmark-fv",  help="Specify the executable name. If none is chosen, the name is automatically deduced by some parameters.")
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

if args.build_mode not in build_modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ".join(list(build_modes.keys())[:-1]), list(build_modes.keys())[-1],args.build_mode))
    sys.exit(1)

accuracy = args.accuracy
if(args.accuracy < 0):
  import numpy
  accuracy = default=numpy.finfo(float).eps

executable_name = "kernel-benchmarks-fv-rusanov-" + str(args.dim) + "d-patch-size-"+ str(args.patch_size)

my_project = exahype2.Project(
  namespace     = ["benchmarks", "exahype2", "kernelbenchmarks"],
  project_name  = "KernelBenchmarksFVRusanov",
  directory     = ".",
  executable    = executable_name
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
  name                    = "FVRusanovSolver",
  patch_size              = args.patch_size,
  unknowns                = {"rho":1, "v":args.dim, "e":1},
  auxiliary_variables     = 0,
  min_volume_h            = 0.001,  # max_cell_size -> arbitrary value
  max_volume_h            = 0.001,  # min_cell_size -> arbitrary value
  time_step_relaxation    = 0.5,
  pde_terms_without_state = True
)

my_solver.set_implementation(
  initial_conditions    = exahype2.solvers.PDETerms.User_Defined_Implementation,
  boundary_conditions   = exahype2.solvers.PDETerms.User_Defined_Implementation,
  flux                  = "applications::exahype2::euler::flux(Q, normal, F);",
  eigenvalues           = "return applications::exahype2::euler::maxEigenvalue(Q, normal);",
  refinement_criterion  = exahype2.solvers.PDETerms.User_Defined_Implementation,
  source_term           = exahype2.solvers.PDETerms.None_Implementation,
  ncp                   = exahype2.solvers.PDETerms.None_Implementation,
)

my_solver.add_user_solver_includes("""
#include "../../../../applications/exahype2/euler/EulerKernels.h"
""")

my_project.add_solver(my_solver)

my_project.set_global_simulation_parameters(
    dimensions              = args.dim,
    size=[1.0, 1.0, 1.0][0:args.dim],
    offset=[0.0, 0.0, 0.0][0:args.dim],
    min_end_time            = 0.1,
    max_end_time            = 0.1,
    first_plot_time_stamp   = 0.0,
    time_in_between_plots   = 0.0,
    periodic_BC             = [False, False, False]
)

my_project.set_Peano4_installation("../../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)

my_project.output.makefile.add_cpp_file("KernelBenchmarksFVRusanov-main.cpp")

my_project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")
my_project.constants.export_constexpr_with_type("MaxPatches", str(args.max_patches), "int")
my_project.constants.export_constexpr_with_type("NumberOfSamples", str(args.samples), "int")
my_project.constants.define_value("GAMMA", str(1))

my_project.build(make=args.use_make, make_clean_first=True, throw_away_data_after_build=True)

print("Executable is ", executable_name)
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 kernel-benchmarks-fv-rusanov.py' again")
