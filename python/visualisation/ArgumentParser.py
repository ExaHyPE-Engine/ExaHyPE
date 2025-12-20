# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import argparse


class ArgumentParser(argparse.ArgumentParser):
    def __init__(self):
        super(ArgumentParser, self).__init__(description="Peano Visualisation Module")

        self.add_argument(
            "--filter-fine-grid",
            action="store_true",
            default=False,
            help="Display only fine grid",
        )
        self.add_argument(
            "--filter-average",
            action="store_true",
            default=False,
            help="Average over cell data (reduces resolution/memory)",
        )
        self.add_argument(
            "--norm-calculator",
            action="store_true",
            default=False,
            help="Compute L2 (Euclidean) norm of the data vector at each point (magnitude of multi-component fields)",
        )
        self.add_argument(
            "--shrink-cells",
            type=float,
            default=1.0,
            help="Shrink or expand patches/cells. Off by default, i.e. set to 1.0",
        )
        self.add_argument("--dir", default=".", help="Output directory")
        self.add_argument(
            "-v",
            "--verbose",
            action="store_true",
            default=False,
            help="Run in a chatty mode",
        )
        self.add_argument(
            "-s",
            "--single-file",
            action="store_true",
            default=False,
            help="Input file is single snapshot (and not meta file)",
        )
        self.add_argument(
            "-j",
            "--max-workers",
            type=int,
            nargs="?",
            const=None,
            default=1,
            help="Number of workers processing the snapshots in parallel",
        )
        self.add_argument(
            "-start",
            "--start-snapshot-index",
            dest="start",
            type=int,
            help="Set the snapshot number where the merge starts (all snapshots before will be ignored), default is 0 (starts from beginning)",
            default=0,
        )
        self.add_argument(
            "-end",
            "--end-snapshot-index",
            dest="end",
            type=int,
            help="Set the snapshot number where the merge ends (all snapshots after will be ignored), default is -1 (ends at the last)",
            default=-1,
        )
        self.add_argument(dest="filename", help="Input file name")
        self.add_argument(
            "--dpi",
            type=int,
            default=300,
            help="Resolution of the output image in DPI.",
        )
        self.add_argument(
            "--flip-view",
            choices=["x", "y", "z"],
            help="Mirror the camera around the chosen axis",
        )
        self.add_argument(
            "-u",
            "--unknown",
            type=int,
            default=0,
            help="Index of the unknown to visualise (e.g., 0, 1, 2...).",
        )
        self.add_argument(
            "--slice-axis",
            choices=["x", "y", "z"],
            help="Axis to slice the 3D volume along (e.g., 'x').",
        )
        self.add_argument(
            "--slice-position",
            type=float,
            help="Position along the axis to create the slice (e.g., 0.5).",
        )
        self.add_argument(
            "--slice-width",
            type=float,
            default=0.02,
            help="The thickness of the slice.",
        )
        self.add_argument(
            "--combine-unknowns",
            type=str,
            default=None,
            help='A string of indices to sum, separated by "+" (starts at 0). Example: "1+2+3"',
        )
        self.add_argument(
            "--raytracing",
            "-rt",
            action="store_true",
            dest="enable_raytracing",
            help="Enable GPU raytracing for 3D PyVista plots (raster formats only, may not work with large datasets)",
        )
