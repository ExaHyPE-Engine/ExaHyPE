# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import argparse
import peano4
import exahype2

build_modes = {
    "Release": peano4.output.CompileMode.Release,
    "Trace": peano4.output.CompileMode.Trace,
    "Assert": peano4.output.CompileMode.Asserts,
    "Stats": peano4.output.CompileMode.Stats,
    "Debug": peano4.output.CompileMode.Debug,
}
build_mode = "Release"

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Finite Volumes Rusanov Kernel Benchmarking Script"
)
parser.add_argument(
    "-m",
    "--build-mode",
    dest="build_mode",
    choices=build_modes.keys(),
    default=build_mode,
    help="|".join(build_modes.keys()),
)
parser.add_argument(
    "-d",
    "--dim",
    dest="dim",
    type=int,
    required=True,
    help="Number of space dimensions",
)
parser.add_argument(
    "-t",
    "--num-threads",
    dest="num_threads",
    type=int,
    default=1,
    help="Number of launching threads",
)
parser.add_argument(
    "-p",
    "--num-patches",
    dest="num_patches",
    nargs="+",
    default=[32, 64, 128, 256, 512, 1024, 2048],
    type=int,
    help="Number of patches to study",
)
parser.add_argument(
    "-ps",
    "--patch-size",
    dest="patch_size",
    type=int,
    required=True,
    help="Number of finite volumes per axis (dimension) per patch",
)
parser.add_argument(
    "-s",
    "--samples",
    dest="samples",
    type=int,
    default=10,
    help="Number of samples per measurement",
)
parser.add_argument(
    "-a",
    "--accuracy",
    dest="accuracy",
    type=float,
    default=0.0,
    help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).",
)
parser.add_argument(
    "-e",
    "--eval-eig",
    dest="eval_eig",
    action="store_true",
    help="Evaluate max. eigenvalue",
)
parser.add_argument(
    "-cpu",
    "--cpu",
    dest="eval_cpu",
    action="store_true",
    help="Evaluate host kernels",
)
parser.add_argument(
    "-gpu",
    "--gpu",
    dest="eval_gpu",
    action="store_true",
    help="Evaluate device kernels",
)
parser.add_argument(
    "-fpe",
    "--fp-exception",
    dest="enable_fpe",
    action="store_true",
    help="Enable a floating-point exception handler",
)
parser.add_argument(
    "--no-make",
    dest="no_make",
    action="store_true",
    help="Do not compile the code after generation",
)
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

if args.build_mode not in build_modes:
    print(
        "Error, mode must be {} or {}, you supplied {}".format(
            ", ".join(list(build_modes.keys())[:-1]),
            list(build_modes.keys())[-1],
            args.build_mode,
        )
    )
    sys.exit(1)

accuracy = args.accuracy
if args.accuracy < 0:
    import numpy

    accuracy = default = numpy.finfo(float).eps

executable_name = (
    "kernel-benchmarks-fv-rusanov-"
    + str(args.dim)
    + "d-ps-"
    + str(args.patch_size)
    + "-"
    + str(args.build_mode)
)
my_project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarksFVRusanov",
    directory=".",
    executable=executable_name,
)

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
    name="FVRusanovSolver",
    patch_size=args.patch_size,
    unknowns={"v": args.dim},
    auxiliary_variables=0,
    min_volume_h=0.001,  # max_cell_size -> arbitrary value
    max_volume_h=0.001,  # min_cell_size -> arbitrary value
    time_step_relaxation=0.5,
    pde_terms_without_state=True,
)

my_solver.set_implementation(
    initial_conditions="",
    boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
    flux="::applications::exahype2::advection::flux(Q, normal, F);",
    eigenvalues="return ::applications::exahype2::advection::maxEigenvalue();",
    refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
    source_term="",
    ncp="",
)

my_solver.add_user_solver_includes(
    """
#include "../../../../applications/exahype2/advection/AdvectionKernels.h"
"""
)

my_project.add_solver(my_solver)

my_project.set_global_simulation_parameters(
    dimensions=args.dim,
    size=[1.0, 1.0, 1.0][0 : args.dim],
    offset=[0.0, 0.0, 0.0][0 : args.dim],
    min_end_time=0.1,
    max_end_time=0.1,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False, False, False],
)

my_project.set_Peano4_installation("../../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)

my_project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")
my_project.constants.export_constexpr_with_type(
    "NumberOfSamples", str(args.samples), "int"
)
formatted_num_patches = "{{{}}}".format(", ".join(str(val) for val in args.num_patches))
my_project.constants.export_const_with_type(
    "NumberOfPatchesToStudy",
    str(formatted_num_patches),
    "::tarch::la::Vector<%s, int>" % len(args.num_patches),
)
my_project.constants.export_constexpr_with_type(
    "NumberOfLaunchingThreads", str(args.num_threads), "int"
)

if args.enable_fpe:
    my_project.constants.export_boolean("EnableFPE", True)
else:
    my_project.constants.export_boolean("EnableFPE", False)

my_project.constants.export_boolean("EvaluateFlux", True)
my_project.constants.export_boolean("EvaluateNonconservativeProduct", False)
my_project.constants.export_boolean("EvaluateSource", False)
my_project.constants.export_boolean(
    "EvaluateMaximumEigenvalueAfterTimeStep", True if args.eval_eig else False
)  # Reduction

if args.eval_cpu == False and args.eval_gpu == False:
    my_project.constants.export_boolean("EvaluateHostKernels", True)
    my_project.constants.export_boolean("EvaluateDeviceKernels", True)
else:
    my_project.constants.export_boolean(
        "EvaluateHostKernels", True if args.eval_cpu else False
    )
    my_project.constants.export_boolean(
        "EvaluateDeviceKernels", True if args.eval_gpu else False
    )

my_project.constants.define_value("ADVECTION_SPEED", str(1.0))

os.system(
    "cp {} KernelBenchmarksFVRusanov-main.cpp".format(
        "../../euler/kernel-benchmarks/KernelBenchmarksFVRusanov-main.cpp"
    )
)

my_project.build(
    make=not args.no_make, make_clean_first=True, throw_away_data_after_build=True
)

print("Executable is ", executable_name)
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 kernel-benchmarks-fv-rusanov.py' again")
