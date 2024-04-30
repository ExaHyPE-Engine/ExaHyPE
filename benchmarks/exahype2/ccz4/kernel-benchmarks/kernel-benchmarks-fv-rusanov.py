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
    default=[32, 64, 128],
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
    "kernel-benchmarks-fv-rusanov-ps-"
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

unknowns = {
    "G": 6,
    "K": 6,
    "theta": 1,
    "Z": 3,
    "lapse": 1,
    "shift": 3,
    "b": 3,
    "dLapse": 3,
    "dxShift": 3,
    "dyShift": 3,
    "dzShift": 3,
    "dxG": 6,
    "dyG": 6,
    "dzG": 6,
    "traceK": 1,
    "phi": 1,
    "P": 3,
    "K0": 1,
}

my_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
    name="FVRusanovSolver",
    patch_size=args.patch_size,
    unknowns=sum(unknowns.values()),
    auxiliary_variables=0,
    min_volume_h=0.001,  # max_cell_size -> arbitrary value
    max_volume_h=0.001,  # min_cell_size -> arbitrary value
    time_step_relaxation=0.5,
    pde_terms_without_state=True,
)

integer_constants = {
    "CCZ4LapseType": 0,
}
double_constants = {
    "CCZ4ds": 1.0,
    "CCZ4c": 1.0,
    "CCZ4e": 1.0,
    "CCZ4f": 0.75,
    "CCZ4bs": 0.0,
    "CCZ4sk": 0.0,
    "CCZ4xi": 1.0,
    "CCZ4itau": 1.0,
    "CCZ4eta": 1.0,
    "CCZ4k1": 0.1,
    "CCZ4k2": 0.0,
    "CCZ4k3": 0.5,
    "CCZ4GLMc": 1.2,
    "CCZ4GLMd": 2.0,
    "CCZ4mu": 0.2,
}

for key, value in integer_constants.items():
    my_solver.add_solver_constants("static constexpr int {} = {};".format(key, value))
for key, value in double_constants.items():
    my_solver.add_solver_constants(
        "static constexpr double {} = {};".format(key, value)
    )

my_solver.set_implementation(
    boundary_conditions="",
    ncp="""
  double deltaQSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    deltaQSerialised[i+0*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+1*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+2*NumberOfUnknowns] = 0.0;
    deltaQSerialised[i+normal*NumberOfUnknowns] = deltaQ[i];
  }
  ::applications::exahype2::ccz4::ncp(BTimesDeltaQ, Q, deltaQSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);
""",
    flux="",
    source_term="""
  //memset(S, 0.0, NumberOfUnknowns*sizeof(double));
  for (int i = 0; i < NumberOfUnknowns; i++) {
    S[i] = 0.0;
  }
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);
""",
    refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
    eigenvalues="""
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd);
""",
    initial_conditions="""
  printf("Initial conditions should not be called in this benchmark!");
  std::abort();
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0;
  ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
""",
)

my_solver.add_user_action_set_includes(
    """
#include "../../../../applications/exahype2/ccz4/CCZ4Kernels.h"
"""
)
my_solver.add_user_solver_includes(
    """
#include "../../../../applications/exahype2/ccz4/CCZ4Kernels.h"
#include "../../../../applications/exahype2/ccz4/InitialValues.h"
#include <cstring>
"""
)

my_project.add_solver(my_solver)

my_project.set_global_simulation_parameters(
    dimensions=3,
    offset=[-0.5, -0.5, -0.5],
    size=[1, 1, 1],
    min_end_time=0.1,
    max_end_time=0.1,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[True, True, True],
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

my_project.constants.export_boolean("EvaluateFlux", False)
my_project.constants.export_boolean("EvaluateNonconservativeProduct", True)
my_project.constants.export_boolean("EvaluateSource", True)
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

my_project.output.makefile.add_cpp_file(
    "../../../../applications/exahype2/ccz4/InitialValues.cpp"
)
my_project.output.makefile.add_cpp_file(
    "../../../../applications/exahype2/ccz4/CCZ4Kernels.cpp"
)
# my_project.output.makefile.add_header_search_path("../../../../applications/exahype2/ccz4")

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
