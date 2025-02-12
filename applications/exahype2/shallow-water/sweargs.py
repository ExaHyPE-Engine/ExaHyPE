# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import peano4
import exahype2

class SWEArgumentParser(argparse.ArgumentParser):
    def __init__(self):
        super(SWEArgumentParser, self).__init__(
            description="ExaHyPE 2 - Shallow Water Application Argument Parser"
        )

        self.add_argument(
            "-dt",
            "--time-step-size",
            type=float,
            default=0.01,
            help="Time step size for fixed time-stepping.",
        )
        self.add_argument(
            "-cfl",
            "--time-step-relaxation",
            type=float,
            default=0.5,
            help="Time step relaxation safety factor for adaptive time-stepping.",
        )

        self.add_argument(
            "-width",
            "--width",
            type=str,
            help="Specify size of domain in meters as [x, y] as string (e.g. [10, 10], [7e6, 4e6]).",
        )

        self.add_argument(
            "-offset",
            "--offset",
            type=str,
            default=[0, 0],
            help="Specify offset of domain in meters as [x, y] as string (e.g. [-10, -10], [-7e6, -4e6]).",
        )

        self.add_argument(
            "-o",
            "--output",
            type=str,
            default="solutions",
            help="Output path for project solutions output. The project will create a new folder at the given path. Default is 'solutions'.",
        )

        self.add_argument(
            "-pbc-x",
            "--periodic-boundary-conditions-x",
            action="store_true",
            help="Use periodic boundary conditions in the x-axis.",
        )
        self.add_argument(
            "-pbc-y",
            "--periodic-boundary-conditions-y",
            action="store_true",
            help="Use periodic boundary conditions in the y-axis.",
        )

        self.add_argument(
            "-m",
            "--build-mode",
            choices=peano4.output.CompileModes,
            default=peano4.output.CompileModes[0], # Release
            help="|".join(peano4.output.CompileModes),
        )

        self.add_argument(
            "-et",
            "--end-time",
            type=float,
            default=5.0,
            help="End time of the simulation.",
        )
        self.add_argument(
            "-ns",
            "--number-of-snapshots",
            type=int,
            default=20,
            help="Number of snapshots (plots).",
        )

        self.add_argument(
            "-ps",
            "--patch-size",
            type=int,
            default=16,
            help="Number of finite volumes per axis (dimension) per patch.",
        )
        self.add_argument(
            "-dg",
            "--dg-order",
            type=int,
            default=5,
            help="Order of discretisation for Discontinuous Galerkin.",
        )
        self.add_argument(
            "-md",
            "--min-depth",
            type=float,
            default=4,
            help="Determines maximum size of a single cell on each axis.",
        )
        self.add_argument(
            "-amr",
            "--amr-levels",
            type=int,
            default=0,
            help="Number of AMR grid levels on top of max. size of a cell.",
        )

        self.add_argument(
            "--storage",
            type=str,
            choices=[e.name for e in exahype2.solvers.Storage],
            default="CallStack",
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
        self.add_argument(
            "-lbs",
            "--load-balancing-strategy",
            choices=available_load_balancing_strategies,
            default="SpreadOutOnceGridStagnates",
            help="|".join(available_load_balancing_strategies),
        )
        self.add_argument(
            "-lbq",
            "--load-balancing-quality",
            type=float,
            default=0.99,
            help="The quality of the load balancing.",
        )
        self.add_argument(
            "--trees",
            type=int,
            default=-1,
            help="Number of trees (partitions) per rank after initial decomposition.",
        )
        self.add_argument(
            "--threads",
            type=int,
            default=0,
            help="Number of threads per rank.",
        )
        self.add_argument(
            "-stateless",
            "--stateless",
            action="store_true",
            default=False,
            help="Use stateless PDE terms (needed for GPU offloading).",
        )
        self.add_argument(
            "-fpe",
            "--fpe",
            action="store_true",
            help="Enable a floating-point exception handler.",
        )
