# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import sys
import argparse
import subprocess

import peano4
import exahype2


class SWE:
    _available_solvers = {
        "RusanovRiemannGlobalFixedFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStep,
        "FWaveRiemannGlobalFixedFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStep,
        "HLLEMRiemannGlobalFixedFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStep,
        "RusanovRiemannGlobalAdaptiveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep,
        "FWaveRiemannGlobalAdaptiveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep,
        "HLLEMRiemannGlobalAdaptiveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep,
        "RusanovRiemannGlobalFixedEnclaveFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStepWithEnclaveTasking,
        "FWaveRiemannGlobalFixedEnclaveFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStepWithEnclaveTasking,
        "HLLEMRiemannGlobalFixedEnclaveFV": exahype2.solvers.fv.riemann.GlobalFixedTimeStepWithEnclaveTasking,
        "RusanovRiemannRiemannGlobalAdaptiveEnclaveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStepWithEnclaveTasking,
        "FWaveRiemannGlobalAdaptiveEnclaveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStepWithEnclaveTasking,
        "HLLEMRiemannGlobalAdaptiveEnclaveFV": exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStepWithEnclaveTasking,
    }

    def setup_parser(self):
        parser = argparse.ArgumentParser(
            description="ExaHyPE 2 - SWE Application Script"
        )

        parser.add_argument(
            "-s",
            "--solver",
            choices=self._available_solvers.keys(),
            help="|".join(self._available_solvers.keys()),
        )
        parser.add_argument(
            "-dt",
            "--time-step-size",
            type=float,
            default=0.01,
            help="Time step size for fixed time-stepping.",
        )
        parser.add_argument(
            "-cfl",
            "--time-step-relaxation",
            type=float,
            default=0.5,
            help="Time step relaxation safety factor for adaptive time-stepping.",
        )

        parser.add_argument(
            "-width",
            "--width",
            type=str,
            help="Specify size of domain in meters as [x, y] as string (e.g. [10, 10], [7e6, 4e6]).",
        )

        parser.add_argument(
            "-offset",
            "--offset",
            type=str,
            default="[0, 0]",
            help="Specify offset of domain in meters as [x, y] as string (e.g. [-10, -10], [-7e6, -4e6]).",
        )

        parser.add_argument(
            "-stateless",
            "--stateless",
            action="store_true",
            default=False,
            help="Use stateless PDE terms (GPU offloading requires a GPU enabled Peano build and an enclave solver).",
        )
        parser.add_argument(
            "-o",
            "--output",
            type=str,
            default="solution",
            help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.",
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

        available_build_modes = ["Release", "Trace", "Asserts", "Stats", "Debug"]
        parser.add_argument(
            "-m",
            "--build-mode",
            choices=available_build_modes,
            default="Release",
            help="|".join(available_build_modes),
        )

        parser.add_argument(
            "-et",
            "--end-time",
            type=float,
            default=5.0,
            help="End time of the simulation.",
        )
        parser.add_argument(
            "-ns",
            "--number-of-snapshots",
            type=int,
            default=10,
            help="Number of snapshots (plots).",
        )

        parser.add_argument(
            "-ps",
            "--patch-size",
            dest="patch_size",
            type=int,
            help="Number of finite volumes per axis (dimension) per patch.",
        )
        parser.add_argument(
            "-md",
            "--min-depth",
            type=float,
            default=1,
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
            "-vis",
            "--visualise",
            action="store_true",
            default=False,
            help="Visualise the output data using the postprocessor.",
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
            "-f",
            "--fuse-tasks",
            type=int,
            default=131072,
            help="Number of enclave tasks to fuse into one meta task.",
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
            "-no-make",
            "--no-make",
            action="store_true",
            help="Do not compile the code after generation.",
        )

        parser.add_argument(
            "--peano-dir",
            default="../../../",
            help="Peano directory",
        )

        return parser

    def setup_solver(self, args):
        solver_params = {
            "name": "SWE",
            "unknowns": {"h": 1, "hu": 2},
            "auxiliary_variables": {"b": 1},
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
                    "normalised_time_step_size": args.time_step_size,
                }
            )
        elif "Adaptive" in args.solver:
            solver_params.update(
                {
                    "time_step_relaxation": args.time_step_relaxation,
                }
            )

        max_h = (
            1.1 * min(self.parse_coordinate_pair(args.width)) / (3.0**args.min_depth)
        )
        min_h = max_h * 3.0 ** (-args.amr_levels)

        if "FV" in args.solver:
            solver_params.update(
                {
                    "patch_size": args.patch_size,
                    "max_volume_h": max_h,
                    "min_volume_h": min_h,
                }
            )

        solver = self._available_solvers[args.solver](**solver_params)
        solver.plot_description = ", ".join(solver_params["unknowns"].keys()) + ", "
        solver.plot_description += ", ".join(
            solver_params["auxiliary_variables"].keys()
        )

        implementation_params = {
            "initial_conditions": exahype2.solvers.PDETerms.User_Defined_Implementation,
            "boundary_conditions": exahype2.solvers.PDETerms.User_Defined_Implementation,
            "refinement_criterion": exahype2.solvers.PDETerms.User_Defined_Implementation,
            "flux": "::applications::exahype2::swe::flux(Q, x, h, t, dt, normal, F);",
            # "ncp": "::applications::exahype2::swe::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);", # TODO: Not working at the moment.
            "eigenvalues": "::applications::exahype2::swe::eigenvalues(Q, x, h, t, dt, normal, L);",
        }

        if "RusanovRiemann" in args.solver:
            implementation_params.update(
                {
                    "riemann_solver": "return ::applications::exahype2::swe::rusanov(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);",
                }
            )
        elif "FWaveRiemann" in args.solver:
            implementation_params.update(
                {
                    "riemann_solver": "return ::applications::exahype2::swe::fwave(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);",
                }
            )
        elif "HLLEMRiemann" in args.solver:
            implementation_params.update(
                {
                    "riemann_solver": "return ::applications::exahype2::swe::hllem(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);",
                }
            )

        solver.set_implementation(**implementation_params)

        return solver

    def setup_project(self, args, solver):
        project = exahype2.Project(
            namespace=["applications", "exahype2", "swe"],
            project_name="SWE",
            directory=".",
            executable=f"ExaHyPE2-SWE-{args.solver}{'-Stateless' if args.stateless else ''}-{args.build_mode}",
        )

        project.add_solver(solver)

        if args.number_of_snapshots <= 0:
            time_in_between_plots = 0.0
        else:
            time_in_between_plots = args.end_time / args.number_of_snapshots
            project.set_output_path(args.output)

        project.set_global_simulation_parameters(
            dimensions=2,
            size=self.parse_coordinate_pair(args.width),
            offset=self.parse_coordinate_pair(args.offset),
            min_end_time=args.end_time,
            max_end_time=args.end_time,
            first_plot_time_stamp=0.0,
            time_in_between_plots=time_in_between_plots,
            periodic_BC=[
                args.periodic_boundary_conditions_x,
                args.periodic_boundary_conditions_y,
            ],
        )

        if args.visualise:
            output_patch_file = f"{args.output}/solution-SWE.peano-patch-file"
            if os.path.isfile(output_patch_file):
                subprocess.call(
                    [
                        "python3",
                        f"{args.peano_dir}/python/peano4/visualisation/render.py",
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
            "::tarch::multicore::orchestration::Hardcoded::createFuseAll(%s, true, true, 0)"
            % str(args.fuse_tasks)
        )

        project.set_timeout(args.timeout)

        build_mode_class = getattr(getattr(peano4, "output"), "CompileMode")
        build_mode = getattr(build_mode_class, args.build_mode)
        project.set_Peano4_installation(args.peano_dir, mode=build_mode)
        project = project.generate_Peano4_project(verbose=True)

        project.constants.export_constexpr_with_type("g", str(9.81), "double")

        if args.fpe:
            project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

        return project

    def build(self, args, project):
        project.build(
            make=not args.no_make,
            make_clean_first=True,
            throw_away_data_after_build=True,
        )

    def parse_coordinate_pair(self, coords):
        coordinate_filter = "((-?[0-9]+\.?[0-9]*(e-?[0-9]+)?)|(inf))"
        if re.search(
            "^\[%s\\s*\,\\s*%s\]$" % (coordinate_filter, coordinate_filter), coords
        ):
            splits = re.split("[\[\]\,]", coords)
            return [float(splits[1]), float(splits[2])]
        else:
            raise ValueError(
                "Could not parse the illegal coordinates: {}!".format(coords)
            )
