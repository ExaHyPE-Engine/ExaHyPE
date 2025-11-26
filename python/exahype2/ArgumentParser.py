# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import peano4.output.CompileMode


class ArgumentParser(argparse.ArgumentParser):
    def __init__(self, title=None):
        super(ArgumentParser, self).__init__(
            description=title if title is not None else "ExaHyPE 2 Argument Parser"
        )

        self.add_argument(
            "-d",
            "--dimensions",
            type=int,
            default=2,
            help="Number of space dimensions.",
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
            "-pbc-z",
            "--periodic-boundary-conditions-z",
            action="store_true",
            help="Use periodic boundary conditions in the z-axis.",
        )
        self.add_argument(
            "-m",
            "--build-mode",
            choices=peano4.output.CompileModes,
            default=peano4.output.CompileModes[0],  # Release
            help="|".join(peano4.output.CompileModes),
        )
        self.add_argument(
            "-et",
            "--end-time",
            type=float,
            default=0.5,
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
            "-dofs",
            "--degrees-of-freedom",
            type=int,
            default=4,
            help="Degrees of freedom of a patch. For FV: Number of finite volumes per axis (dimension) per patch. For DG: Order of discretisation.",
        )
        self.add_argument(
            "-md",
            "--min-depth",
            type=int,
            default=6,
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
            "-lbq",
            "--load-balancing-quality",
            type=float,
            default=0.99,
            help="The quality of the load balancing.",
        )
        self.add_argument(
            "--trees",
            type=int,
            default="-1",
            help="Number of trees (partitions) per rank.",
        )

        self.add_argument(
            "-ti",
            "--print-time-interval",
            type=int,
            default=10,
            help="Time interval to print time step information.",
        )
        self.add_argument(
            "-td",
            "--target-device",
            default="tarch::accelerator::Device::DefaultDevice",
            help="Target device for GPU offloading.",
        )
        self.add_argument(
            "-enclave",
            "--enclave-tasking",
            default=False,
            action="store_true",
            help="Use enclave tasking.",
        )
        self.add_argument(
            "--ntasks",
            type=int,
            default=1,
            help="Number of fused enclave tasks to offload asynchronously.",
        )
        self.add_argument(
            "-fpe",
            "--fpe",
            action="store_true",
            help="Enable a floating-point exception handler.",
        )
        self.add_argument(
            "-pl",
            "--plotter",
            type=str,
            default="PeanoPatch",
            help="Choose which plotter to use: 'HDF5' or 'PeanoPatch'.",
        )
