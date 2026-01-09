# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import sys, os, argparse
import peano4, exahype2

sys.path.insert(0, os.path.abspath("../../../tests/aderdg"))
import scenarios

available_scenarios = {
    "AcousticPlanarWaves": scenarios.AcousticPlanarWaves(dimensions=2),
    "AdvectionLinear": scenarios.AdvectionLinear(),
    "ElasticPlanarWaves": scenarios.ElasticPlanarWaves(dimensions=2),
    "EulerGaussianBell": scenarios.EulerGaussianBell(),
    "EulerIsentropicVortex": scenarios.EulerIsentropicVortex(),
    "SWERadialDamBreak": scenarios.SWERadialDamBreak(),
    "SWERestingLake": scenarios.SWERestingLake(),
}

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Fused ADER-DG Kernel Benchmarking Script"
)
parser.add_argument(
    "-m",
    "--build-mode",
    choices=peano4.output.CompileModes,
    default=peano4.output.CompileModes[0],  # Release
    help="|".join(peano4.output.CompileModes),
)
parser.add_argument(
    "-t",
    "--num-threads",
    type=int,
    nargs="+",
    default=[1, 2, 4, 8, 16],
    help="Number of launching threads",
)
parser.add_argument(
    "-o",
    "--order",
    type=int,
    default=5,
    help="Order of the underlying ADER-DG solver.",
)
parser.add_argument(
    "-c",
    "--num-cells",
    type=int,
    nargs="+",
    default=[
        64,
        81,
        128,
        256,
        512,
        729,
        1024,
        2048,
    ],  # , 4096, 6561, 16384, 32768, 59049, 65536],
    help="Number of patches to study",
)
parser.add_argument(
    "-samples",
    "--samples",
    type=int,
    default=10,
    help="Number of samples per measurement",
)
parser.add_argument(
    "-a",
    "--accuracy",
    type=float,
    default=0.0,
    help="Floating point accuracy to which the different kernel variants have to match (absolute). Pass in 0 to disable correctness check. Pass in values < 0 to use machine epsilon (default).",
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
    "-pr",
    "--precision",
    choices=["double", "float", "fp16", "bf16"],
    default="double",
    help="Precision in which the solver should be computed.",
)
parser.add_argument(
    "-s",
    "--scenario",
    choices=available_scenarios.keys(),
    default="ElasticPlanarWaves",
    help="Scenario which should be used as a base for the benchmarking",
)
parser.add_argument(
    "-md",
    "--min-depth",
    type=int,
    default=6,
    help="Determines maximum size of a single cell on each axis.",
)

args = parser.parse_args()

scenario = available_scenarios[args.scenario]

ader_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="AderSolver",
    order=args.order,
    unknowns=scenario._equation.num_unknowns,
    auxiliary_variables=scenario._equation.num_auxiliary_variables,
    min_cell_h=0.001,  # max_cell_size -> arbitrary value
    max_cell_h=0.001,  # min_cell_size -> arbitrary value
    time_step_relaxation=0.5,
)

ader_solver.add_kernel_optimisations(
    polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre,
    is_linear=scenario._equation.is_linear,
    precision=args.precision,
)

ader_solver.set_implementation(
    initial_conditions=scenario.initial_conditions(),
    boundary_conditions=scenario.boundary_conditions(),
    max_eigenvalue=scenario._equation.eigenvalues(),
    flux=scenario._equation.flux(),
    ncp=scenario._equation.ncp(),
    riemann_solver=scenario._equation.riemann_solver(),
)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "kernelbenchmarks"],
    project_name="KernelBenchmarks",
    directory=".",
    executable="KernelBenchmarks",
)
project.add_solver(ader_solver)

project.set_global_simulation_parameters(
    dimensions=2,
    size=[1.0, 1.0],
    offset=[0.0, 0.0],
    min_end_time=0.1,
    max_end_time=0.1,
    first_plot_time_stamp=0.0,
    time_in_between_plots=0.0,
    periodic_BC=[False, False],
)

project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)

accuracy = args.accuracy
if accuracy < 0:
    import numpy

    accuracy = default = numpy.finfo(float).eps
project.constants.export_constexpr_with_type("Accuracy", str(accuracy), "double")

project.constants.export_constexpr_with_type(
    "NumberOfSamples", str(args.samples), "int"
)

formatted_num_cells = "{{{}}}".format(", ".join(str(val) for val in args.num_cells))
project.constants.export_const_with_type(
    "NumberOfCellsToStudy",
    str(formatted_num_cells),
    "tarch::la::Vector<%s, int>" % len(args.num_cells),
)

formatted_num_threads = "{{{}}}".format(", ".join(str(val) for val in args.num_threads))
project.constants.export_const_with_type(
    "NumberOfLaunchingThreads",
    str(formatted_num_threads),
    "tarch::la::Vector<%s, int>" % len(args.num_threads),
)

if args.cpu == False and args.gpu == False:
    project.constants.export_boolean("AssessHostKernels", True)
    project.constants.export_boolean("AssessDeviceKernels", True)
else:
    project.constants.export_boolean("AssessHostKernels", True if args.cpu else False)
    project.constants.export_boolean("AssessDeviceKernels", True if args.gpu else False)

makefile = project.output.makefile

makefile.add_CXX_flag("-DSolverPrecision=" + args.precision)

for i in range(1, 7):
    makefile.add_cpp_file("Variant" + str(i) + ".cpp")
makefile.add_h_file("Variants.h")
makefile.add_cpp_file("KernelBenchmarks-main.cpp")

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
