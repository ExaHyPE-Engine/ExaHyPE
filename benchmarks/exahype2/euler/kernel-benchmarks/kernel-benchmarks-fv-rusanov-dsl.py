# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

import peano4
import exahype2

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Shallow Water Finite Volume Rusanov Kernel Benchmarking Script"
)
parser.add_argument(
    "-m",
    "--build-mode",
    choices=peano4.output.CompileModes,
    default=peano4.output.CompileModes[0], # Release
    help="|".join(peano4.output.CompileModes),
)
parser.add_argument(
    "-t",
    "--num-threads",
    default=1,
    type=int,
    help="Number of launching threads",
)
parser.add_argument(
    "-ps",
    "--patch-size",
    default=16,
    type=int,
    help="Number of finite volumes per axis (dimension) per patch.",
)
parser.add_argument(
    "-p",
    "--num-patches",
    nargs="+",
    default=[32, 64, 128, 256, 512, 1024, 2048],
    type=int,
    help="Number of patches to study",
)
parser.add_argument(
    "-samples",
    "--samples",
    default=10,
    type=int,
    help="Number of samples per measurement",
)
parser.add_argument(
    "-a",
    "--accuracy",
    default=0.0,
    type=float,
    help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).",
)
parser.add_argument(
    "-e",
    "--eval-eig",
    action="store_true",
    help="Evaluate max. eigenvalue",
)
parser.add_argument(
    "-cpu",
    "--cpu",
    action="store_true",
    help="Assess host kernels",
)
parser.add_argument(
    "-gpu",
    "--gpu",
    action="store_true",
    help="Assess device kernels",
)
parser.add_argument(
    "-fpe",
    "--fpe",
    action="store_true",
    help="Enable a floating-point exception handler.",
)
parser.add_argument(
    "-d",
    "--dimensions",
    default=2,
    choices = [2, 3],
    type=int,
    help="Number of dimensions to use: 2 or 3",
)
parser.add_argument(
    "-j",
    default=-1,
    type=int,
    help="Number of parallel builds to use",
)
args = parser.parse_args()

fv_rusanov_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking(
    name="FVRusanov",
    patch_size=args.patch_size,
    unknowns={"rho": 1,  "u": 1, "v": 1, "e": 1} if args.dimensions == 2 else {"rho": 1,  "u": 1, "v": 1, "w": 1, "e": 1},
    auxiliary_variables = {},
    min_volume_h=0.001,  # max_cell_size -> arbitrary value
    max_volume_h=0.001,  # min_cell_size -> arbitrary value
    time_step_relaxation=0.5,
    pde_terms_without_state=True,
)

fv_rusanov_solver.set_implementation(
    initial_conditions="",
    boundary_conditions=exahype2.solvers.PDETerms.Empty_Implementation,
    refinement_criterion=exahype2.solvers.PDETerms.Empty_Implementation,
    flux="applications::exahype2::euler::flux(Q, faceCentre, volumeH, t, dt, normal, F);",
    ncp="",
    eigenvalues="applications::exahype2::euler::maxEigenvalue(Q, faceCentre, volumeH, t, dt, normal, maxEigenvalue);",
    source_term="",
)
fv_rusanov_solver.add_user_solver_includes(
        """
    #include "../../../../applications/exahype2/euler/Euler.h"
    """
)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarksFVRusanovDSL",
    directory=".",
    executable=f"KernelBenchmarksFVRusanovDSL.{args.build_mode}",
)
project.add_solver(fv_rusanov_solver)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0] * args.dimensions,
    offset=[0.0] * args.dimensions,
    min_end_time=0.1,
    max_end_time=0.1,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False] * args.dimensions
)

project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)

accuracy = args.accuracy
if accuracy < 0:
    import numpy
    accuracy = default = numpy.finfo(float).eps
project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")

project.constants.export_constexpr_with_type("NumberOfSamples", str(args.samples), "int")

formatted_num_patches = "{{{}}}".format(", ".join(str(val) for val in args.num_patches))
project.constants.export_const_with_type("NumberOfPatchesToStudy", str(formatted_num_patches), "tarch::la::Vector<%s, int>" % len(args.num_patches))

project.constants.export_constexpr_with_type("NumberOfLaunchingThreads", str(args.num_threads), "int")

if args.fpe:
    project.constants.export_boolean("EnableFPE", True)
else:
    project.constants.export_boolean("EnableFPE", False)

if args.cpu == False and args.gpu == False:
    project.constants.export_boolean("AssessHostKernels", True)
    project.constants.export_boolean("AssessDeviceKernels", True)
else:
    project.constants.export_boolean("AssessHostKernels", True if args.cpu else False)
    project.constants.export_boolean("AssessDeviceKernels", True if args.gpu else False)

project.constants.export_boolean("EvaluateFlux", True)
project.constants.export_boolean("EvaluateNonconservativeProduct", False)
project.constants.export_boolean("EvaluateSource", False)
project.constants.export_boolean("EvaluateMaximumEigenvalueAfterTimeStep", True if args.eval_eig else False)  # Reduction

project.constants.define_value("GAMMA", str(1.0))

benchmark_main_file = "../../kernel-benchmarks/KernelBenchmarksFVRusanovDSL-main.cpp"
os.system("cp {} KernelBenchmarksFVRusanovDSL-main.cpp".format(benchmark_main_file))

# Add the necessary Euler implementation files
#project.output.makefile.add_cpp_file("AbstractEuler.cpp")

project.output.makefile.add_CXX_flag("-DGRAV=9.81")

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True, number_of_parallel_builds=32)

print(project)
print(fv_rusanov_solver)
