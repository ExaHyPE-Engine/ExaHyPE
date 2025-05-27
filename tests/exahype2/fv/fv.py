# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import peano4
import exahype2

parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - Testing Script for Various Finite Volumes Solvers"
)

available_solvers = {
    "FVRusanovGlobalAdaptive": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
    "FVRusanovGlobalAdaptiveEnclave": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
}
parser.add_argument(
    "-s",
    "--solver",
    default="FVRusanovGlobalAdaptive",
    choices=available_solvers.keys(),
    help="|".join(available_solvers.keys()),
)
parser.add_argument(
    "-d",
    "--dimensions",
    type=int,
    default=2,
    help="Number of space dimensions.",
)
parser.add_argument(
    "-m",
    "--build-mode",
    choices=peano4.output.CompileModes,
    default=peano4.output.CompileModes[0], # Release
    help="|".join(peano4.output.CompileModes),
)
parser.add_argument(
    "-et",
    "--end-time",
    type=float,
    default=1.0,
    help="End time of the simulation.",
)
parser.add_argument(
    "-ns",
    "--number-of-snapshots",
    type=int,
    default=20,
    help="Number of snapshots (plots).",
)
parser.add_argument(
    "-ps",
    "--patch-size",
    type=int,
    default=16,
    help="Number of finite volumes per axis (dimension) per patch.",
)
parser.add_argument(
    "-md",
    "--min-depth",
    type=float,
    default=6,
    help="Determines maximum size of a single cell on each axis.",
)
parser.add_argument(
    "-amr",
    "--amr-levels",
    type=int,
    default=0,
    help="Number of AMR grid levels on top of max. size of a cell.",
)
parser.add_argument(
    "--storage",
    type=str,
    choices=[e.name for e in exahype2.solvers.Storage],
    default="Heap",
    help="The storage scheme to use."
)
available_load_balancing_strategies = [
    "None",
    "SpreadOut",
    "SpreadOutHierarchically",
    "SpreadOutOnceGridStagnates",
    "RecursiveBipartition",
    "SplitOversizedTree",
    "cascade::SpreadOut_RecursiveBipartition",
    "cascade::SpreadOut_SplitOversizedTree",
]
parser.add_argument(
    "-lbs",
    "--load-balancing-strategy",
    choices=available_load_balancing_strategies,
    default="SpreadOutOnceGridStagnates",
    help="|".join(available_load_balancing_strategies),
)
parser.add_argument(
    "-lbq",
    "--load-balancing-quality",
    type=float,
    default=0.99,
    help="The quality of the load balancing.",
)
parser.add_argument(
    "--trees",
    type=int,
    default=-1,
    help="Number of trees (partitions) per rank after initial decomposition.",
)
parser.add_argument(
    "--threads",
    type=int,
    default=0,
    help="Number of threads per rank.",
)
parser.add_argument(
    "-fuse",
    "--fuse",
    action="store_true",
    default=False,
    help="Fuse enclave tasks on the CPU (requires an enclave solver).",
)
parser.add_argument(
    "-gpu",
    "--gpu",
    action="store_true",
    default=False,
    help="Fuse enclave tasks on the GPU (requires an enclave solver).",
)
parser.add_argument(
    "-timeout",
    "--timeout",
    type=int,
    default=3600,
    help="MPI timeout in seconds.",
)
parser.add_argument(
    "-fpe",
    "--fpe",
    action="store_true",
    help="Enable a floating-point exception handler.",
)
parser.add_argument(
    "-pbc-x",
    "--periodic-boundary-conditions-x",
    action="store_true",
    default=True,
    help="Use periodic boundary conditions in the x-axis.",
)
parser.add_argument(
    "-pbc-y",
    "--periodic-boundary-conditions-y",
    action="store_true",
    default=False,
    help="Use periodic boundary conditions in the y-axis.",
)
parser.add_argument(
    "-pbc-z",
    "--periodic-boundary-conditions-z",
    action="store_true",
    default=False,
    help="Use periodic boundary conditions in the z-axis.",
)
parser.add_argument(
    "-o",
    "--output",
    type=str,
    default="solutions",
    help="Output path for project solutions output. The project will create a new folder at the given path. Default is 'solutions'.",
)

args = parser.parse_args()

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = std::sin(volumeCentre(0) * tarch::la::PI) * std::sin(volumeCentre(1) * tarch::la::PI)
    #if Dimensions == 3
      * std::sin(volumeCentre(2) * tarch::la::PI)
    #endif
    ;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

  if (volumeCentre(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
"""

max_h = 1.1 / 3.0**args.min_depth
min_h = max_h * 3.0**(-args.amr_levels)

fv_solver = available_solvers[args.solver](
    name=args.solver,
    patch_size=16,
    unknowns={"v": args.dimensions},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
    pde_terms_without_state=True,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  F[normal] = Q[normal];
""",
    eigenvalues="maxEigenvalue[0] = 1.0;",
)
fv_solver.plot_description = ", ".join({"v": args.dimensions}.keys())
fv_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=args.solver,
    directory=".",
    executable=args.solver,
)
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0, 1.0, 1.0][0:args.dimensions],
    offset=[0.0, 0.0, 0.0][0:args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
        args.periodic_boundary_conditions_z,
    ],
)

if args.load_balancing_strategy != "None":
    strategy = f"toolbox::loadbalancing::strategies::{args.load_balancing_strategy}"
    assume_periodic_boundary_conditions = any([
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
        args.periodic_boundary_conditions_z,
    ])
    configuration = (
        f"new ::exahype2::LoadBalancingConfiguration("
        f"{args.load_balancing_quality},"
        f"0,"
        f"{'true' if assume_periodic_boundary_conditions else 'false'},"
        f"{args.trees},"
        f"{args.trees})"
    )
    project.set_load_balancing(strategy, configuration)

project.set_number_of_threads(args.threads)
project.set_timeout(args.timeout)
project.set_Peano4_installation("../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)
if args.fpe:
    project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)

print(args)
print(project)
print(fv_solver)
