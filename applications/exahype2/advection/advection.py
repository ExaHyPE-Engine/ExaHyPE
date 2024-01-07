# This file is part of the ExaHyPE 2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import argparse
import subprocess

sys.path.insert(0, os.path.abspath("../../../python"))
import peano4
import exahype2

# Define default values
default_values = {
    "dimensions": 2,
    "cell_size": 0.1,
    "amr_levels": 4,
    "patch_size": 16,
    "rk_order": 2,
    "dg_order": 5,
    "end_time": 3.0,
    "plot_interval": 0.1,
    "advection_speed": 1.0,
    "domain_size": [1.0, 1.0, 1.0],
    "domain_offset": [0.0, 0.0, 0.0],
}

# Define solver types and their corresponding classes
available_solver_types = {
    "FVRusanovGlobalFixed": exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
    "FVRusanovGlobalFixedEnclave": exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
    "FVRusanovGlobalAdaptive": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
    "FVRusanovGlobalAdaptiveEnclave": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
    "RKDGRusanovGlobalFixed": exahype2.solvers.rkdg.rusanov.GlobalFixedTimeStep,
    "RKDGRusanovGlobalAdaptive": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
    "RKDGRusanovGlobalAdaptiveEnclave": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
    "ADERDGRusanovGlobalFixed": exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep,
    "ADERDGRusanovGlobalAdaptive": exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep,
}


def main():
    my_args = parse_arguments()
    my_solver = setup_solver(my_args)
    my_project = setup_project(my_args, my_solver)
    print(my_args)
    print(my_solver)
    print(my_project)


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - Advection Application Script"
    )
    parser.add_argument(
        "-s",
        "--solver",
        choices=available_solver_types.keys(),
        default="ADERDGRusanovGlobalAdaptive",
        help="|".join(available_solver_types.keys()),
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default="solution",
        help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.",
    )
    available_build_modes = ["Release", "Trace", "Assert", "Stats", "Debug"]
    parser.add_argument(
        "-m",
        "--build-mode",
        choices=available_build_modes,
        default="Release",
        help="|".join(available_build_modes),
    )
    parser.add_argument(
        "-d",
        "--dim",
        type=int,
        default=default_values["dimensions"],
        help="Number of space dimensions.",
    )
    parser.add_argument(
        "-et",
        "--end-time",
        type=float,
        default=default_values["end_time"],
        help="End time of the simulation.",
    )
    parser.add_argument(
        "-pi",
        "--plot-interval",
        type=float,
        default=default_values["plot_interval"],
        help="Time between plots and also between database entries when probes are used.",
    )
    parser.add_argument(
        "-amr",
        "--amr-levels",
        type=int,
        default=default_values["amr_levels"],
        help="Number of AMR grid levels on top of max. size of a cell.",
    )
    parser.add_argument(
        "-damr",
        "--dynamic-amr",
        action="store_true",
        default=False,
        help="Use dynamic AMR.",
    )
    parser.add_argument(
        "-stateless",
        "--stateless",
        action="store_true",
        default=True,
        help="Use stateless PDE terms (GPU offloading requires a GPU enabled Peano build and an enclave solver).",
    )
    parser.add_argument(
        "-f",
        "--fuse-tasks",
        type=int,
        default=16384,
        help="Number of enclave tasks to fuse into one meta task.",
    )
    parser.add_argument(
        "-vis",
        "--visualise",
        action="store_true",
        default=False,
        help="Visualise the output data using the postprocessor.",
    )
    parser.add_argument(
        "-pbc-x",
        "--periodic-boundary-conditions-x",
        action="store_true",
        help="Use periodic boundary conditions in the x-axis.",
    )
    parser.add_argument(
        "-pbc-y",
        "--periodic-boundary-conditions-y",
        action="store_true",
        help="Use periodic boundary conditions in the y-axis.",
    )
    parser.add_argument(
        "-pbc-z",
        "--periodic-boundary-conditions-z",
        action="store_true",
        help="Use periodic boundary conditions in the z-axis.",
    )
    parser.add_argument(
        "-maxh",
        "--max-cell-size",
        type=float,
        default=default_values["cell_size"],
        help="Maximum size of a single cell on each axis.",
    )
    parser.add_argument(
        "-ps",
        "--patch-size",
        type=int,
        default=default_values["patch_size"],
        help="Number of finite volumes per axis (dimension) per patch.",
    )
    parser.add_argument(
        "-rk-order",
        "--rk-order",
        type=int,
        default=default_values["rk_order"],
        help="Order of time discretisation for Runge-Kutta solvers.",
    )
    parser.add_argument(
        "-dg-order",
        "--dg-order",
        type=int,
        default=default_values["dg_order"],
        help="Order of space discretisation for Discontinuous Galerkin.",
    )
    available_load_balancing_strategies = [
        "None",
        "SpreadOut",
        "SpreadOutHierarchically",
        "SpreadOutOnceGridStagnates",
        "RecursiveBipartition",
    ]
    parser.add_argument(
        "-lb",
        "--load-balancing-strategy",
        choices=available_load_balancing_strategies,
        default="SpreadOutOnceGridStagnates",
        help="|".join(available_load_balancing_strategies),
    )
    parser.add_argument(
        "-lb-q",
        "--load-balancing-quality",
        type=float,
        default=0.99,
        help="The quality of the load balancing.",
    )
    parser.add_argument(
        "-lb-min",
        "--lb-min-size",
        type=int,
        default=0,
        help="If a tree (partition) is smaller than the given number of cells, we do not split it up further.",
    )
    parser.add_argument(
        "--lb-trees-init",
        type=int,
        default=-1,
        help="Number of trees (partitions) per rank during initial decomposition.",
    )
    parser.add_argument(
        "--lb-trees",
        type=int,
        default=-2,
        help="Number of trees (partitions) per rank after initial decomposition.",
    )
    parser.add_argument(
        "--threads",
        type=int,
        default=0,
        help="Number of threads per rank.",
    )
    parser.add_argument(
        "--timeout",
        dest="timeout",
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
        "--no-make",
        action="store_true",
        help="Do not compile the code after generation.",
    )
    args = parser.parse_args()

    if args.dim not in [2, 3]:
        raise Exception(f"Dimensions must be 2 or 3, you supplied {args.dim}")

    if args.build_mode not in available_build_modes:
        raise Exception(
            f"Build mode must be one of {', '.join(available_build_modes)}, you supplied {args.build_mode}"
        )

    if args.solver not in available_solver_types:
        raise Exception(
            f"Solver must be one of {', '.join(available_solver_types.keys())}, you supplied {args.solver}"
        )

    if args.load_balancing_strategy not in available_load_balancing_strategies:
        raise Exception(
            f"Load balancing strategy must be one of {', '.join(available_load_balancing_strategies)}, you supplied {args.build_mode}"
        )

    return args


def setup_solver(args):
    solver_params = {
        "name": "AdvectionSolver",
        "unknowns": {"u": 1, "v": 1, "w": 1} if args.dim == 3 else {"u": 1, "v": 1},
        "auxiliary_variables": 0,
    }

    if args.stateless:
        solver_params.update(
            {
                "pde_terms_without_state": True,
            }
        )

    if "Fixed" in args.solver:
        solver_params.update(
            {
                "normalised_time_step_size": solver_params["min_volume_h"]
                / args.patch_size
                * 0.5,
            }
        )
    elif "Adaptive" in args.solver:
        solver_params.update(
            {
                "time_step_relaxation": 0.5,
            }
        )

    if "FV" in args.solver:
        solver_params.update(
            {
                "patch_size": args.patch_size,
                "max_volume_h": args.max_cell_size,
                "min_volume_h": args.max_cell_size * 3.0 ** (-args.amr_levels),
            }
        )
    elif "RKDG" in args.solver:
        solver_params.update(
            {
                "max_cell_h": args.max_cell_size,
                "min_cell_h": args.max_cell_size * 3.0 ** (-args.amr_levels),
                "rk_order": args.rk_order,
                "polynomials": exahype2.solvers.GaussLegendreBasis(args.dg_order),
                "face_projections": exahype2.solvers.rkdg.FaceProjections.Solution,
            }
        )
    elif "ADERDG" in args.solver:
        solver_params.update(
            {
                "max_cell_h": args.max_cell_size,
                "min_cell_h": args.max_cell_size * 3.0 ** (-args.amr_levels),
                "order": args.dg_order,
                "is_linear": True,
                "use_kernel_generator": True,
                "architecture": "noarch",
            }
        )

    solver = available_solver_types[args.solver](**solver_params)
    solver.plot_description = ", ".join(solver_params["unknowns"].keys())

    solver.set_implementation(
        initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
        boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
        flux="::applications::exahype2::advection::flux(Q, normal, F);",
        eigenvalues="return ::applications::exahype2::advection::maxEigenvalue();",
        refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
        source_term=exahype2.solvers.PDETerms.None_Implementation,
        ncp=exahype2.solvers.PDETerms.None_Implementation,
    )

    solver.add_user_solver_includes(
        """
#include "AdvectionKernels.h"
"""
    )
    return solver


def setup_project(args, solver):
    project = exahype2.Project(
        namespace=["applications", "exahype2", "advection"],
        project_name="Advection",
        directory=".",
        executable=f"advection-{args.dim}d-{args.solver}{'-Stateless' if args.stateless else ''}-{args.build_mode}",
    )

    project.add_solver(solver)

    project.set_global_simulation_parameters(
        dimensions=args.dim,
        size=default_values["domain_size"][0 : args.dim],
        offset=default_values["domain_offset"][0 : args.dim],
        min_end_time=args.end_time,
        max_end_time=args.end_time,
        first_plot_time_stamp=0.0,
        time_in_between_plots=args.plot_interval,
        periodic_BC=[
            args.periodic_boundary_conditions_x,
            args.periodic_boundary_conditions_y,
            args.periodic_boundary_conditions_z,
        ],
    )

    solution_dir = f"{args.output}-{project.executable_name}"
    project.set_output_path(solution_dir)
    if not os.path.exists(solution_dir):
        os.makedirs(solution_dir)

    if args.visualise:
        output_patch_file = f"{solution_dir}/solution-AdvectionSolver.peano-patch-file"
        if os.path.isfile(output_patch_file):
            subprocess.call(
                [
                    "python3",
                    "../../../python/peano4/visualisation/render.py",
                    str(output_patch_file),
                ]
            )
        sys.exit(0)

    if args.load_balancing_strategy != "None":
        project.set_load_balancing(
            f"toolbox::loadbalancing::strategies::{args.load_balancing_strategy}",
            f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, {args.lb_min_size}, {args.lb_trees_init}, {args.lb_trees})",
        )

    project.set_number_of_threads(args.threads)

    project.set_multicore_orchestration(
        "::tarch::multicore::orchestration::Hardcoded::createFuseAll(%s, true, true, ::tarch::accelerator::Device::getInstance().getRankLocalDeviceId())"
        % str(args.fuse_tasks)
    )

    project.set_timeout(args.timeout)

    build_mode_class = getattr(getattr(peano4, "output"), "CompileMode")
    build_mode = getattr(build_mode_class, args.build_mode)
    project.set_Peano4_installation("../../../", mode=build_mode)
    project = project.generate_Peano4_project(verbose=True)

    if args.fpe:
        project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

    project.constants.export_constexpr_with_type(
        "ADVECTION_SPEED", str(default_values["advection_speed"]), "double"
    )
    project.constants.export_boolean("USE_DYNAMIC_AMR", args.dynamic_amr)

    project.output.makefile.add_cpp_file("AdvectionSolver.cpp")

    project.build(
        make=not args.no_make, make_clean_first=True, throw_away_data_after_build=True
    )
    return project


if __name__ == "__main__":
    main()
