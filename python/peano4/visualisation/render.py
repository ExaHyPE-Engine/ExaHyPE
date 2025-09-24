# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

import peano4.visualisation.filters
import peano4.visualisation.output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Peano 4 - Render Script")
    parser.add_argument(
        "--filter-fine-grid",
        action="store_true",
        default=False,
        help="Display only fine grid",
    )
    parser.add_argument(
        "--filter-average",
        action="store_true",
        default=False,
        help="Average over cell data (reduces resolution/memory)",
    )
    parser.add_argument(
        "--norm-calculator",
        action="store_true",
        default=False,
        help="Compute norm over each point",
    )
    parser.add_argument(
        "--shrink-cells",
        type=float,
        default=1.0,
        help="Shrink or expand patches/cells. Off by default, i.e. set to 1.0",
    )
    parser.add_argument("--dir", default=".", help="Output directory")
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Run in a chatty mode",
    )
    parser.add_argument(
        "-s",
        "--single-file",
        action="store_true",
        default=False,
        help="Input file is single snapshot (and not meta file)",
    )
    parser.add_argument(
        "-j",
        "--max-workers",
        type=int,
        nargs="?",
        const=None,
        default=1,
        help="Number of workers processing the snapshots in parallel",
    )
    parser.add_argument(
        "--type",
        choices=["vtu-unstructured"],
        default="vtu-unstructured",
        help="Output format",
    )
    parser.add_argument(
        "-start",
        "--start-snapshot-index",
        dest="start",
        type=int,
        help="Set the snapshot number where the merge starts (all snapshots before will be ignored), default is 0 (starts from beginning)",
        default=0,
    )
    parser.add_argument(
        "-end",
        "--end-snapshot-index",
        dest="end",
        type=int,
        help="Set the snapshot number where the merge ends (all snapshots after will be ignored), default is -1 (ends at the last)",
        default=-1,
    )
    parser.add_argument(dest="filename", help="Input file name")

    matplotlib_group = parser.add_argument_group("Matplotlib Options")
    matplotlib_group.add_argument(
        "--matplotlib",
        "-m",
        action="store_true",
        help="Use Matplotlib for visualisation",
    )
    matplotlib_group.add_argument(
        "--dpi",
        type=int,
        default=300,
        help="Resolution of the Matplotlib output image in DPI.",
    )
    matplotlib_group.add_argument(
        "-f",
        "--format",
        default="png",
        choices=["png", "pdf", "svg", "pgf"],
        help="Output image format.",
    )
    matplotlib_group.add_argument(
        "-u",
        "--unknown",
        default=None,
        help="Name of the unknown to visualise (e.g., 'rho').",
    )
    matplotlib_group.add_argument(
        "--slice-axis",
        choices=["x", "y", "z"],
        help="Axis to slice the 3D volume along (e.g., 'x').",
    )
    matplotlib_group.add_argument(
        "--slice-position",
        type=float,
        help="Position along the axis to create the slice (e.g., 0.5).",
    )
    matplotlib_group.add_argument(
        "--slice-width", type=float, default=0.01, help="The thickness of the slice."
    )
    matplotlib_group.add_argument(
        "--combine-unknowns",
        type=str,
        default=None,
        help='A string of indices to sum, separated by "+" (starts at 0). Example: "1+2+3"',
    )

    args = parser.parse_args()

    # Start with some sanity checks
    if not os.path.exists(args.filename):
        raise Exception(
            "Specified input file '{}' does not exist, exiting...".format(args.filename)
        )

    total_snapshot_number = 0
    with open(args.filename, "r") as metafile:
        for line in metafile.readlines():
            if "timestamp" in line:
                total_snapshot_number += 1

    if (args.start > args.end and args.end != -1) or args.start < 0 or args.end < -1:
        raise Exception("Specified snapshot index is wrong, please check.")
    elif args.end == -1:
        start_snapshot = args.start
        end_snapshot = (
            total_snapshot_number - 1
        )  # Processing all snapshots if no end snapshot is specified.
    elif args.end > (total_snapshot_number - 1):
        start_snapshot = args.start
        end_snapshot = total_snapshot_number - 1
        print(
            "Warning: specified end snapshot index exceeds the total number, reset to the last snapshot."
        )
    else:
        start_snapshot = args.start
        end_snapshot = args.end

    print(
        "The meta file includes",
        total_snapshot_number,
        "snapshots, processing snapshot",
        start_snapshot,
        "to",
        end_snapshot,
    )

    def sum_all_unknowns(values):
        return [sum(values)]

    def l2_norm_functor(values):
        """L2 norm (magnitude) of all unknowns"""
        import math

        return [math.sqrt(sum(v * v for v in values))]

    visualiser = None
    if args.matplotlib:
        visualiser = peano4.visualisation.output.MatplotlibVisualiser(
            file_name=args.filename,
            dpi=args.dpi,
            output_directory=args.dir,
            verbose=args.verbose,
            # start and end currently only passed for the parent class constructor
            start=start_snapshot,
            end=end_snapshot,
            format=args.format,
            unknown=args.unknown,
            # only relevant for 3d plots
            slice_axis=args.slice_axis,
            slice_position=args.slice_position,
            slice_width=args.slice_width,
        )
    elif args.type == "vtu-unstructured":
        visualiser = peano4.visualisation.output.VTUUnstructuredGrid(
            file_name=args.filename,
            output_directory=args.dir,
            verbose=args.verbose,
            start=start_snapshot,
            end=end_snapshot,
        )

    #
    # Ensure the filter order is cheap-to-expensive
    #
    if args.combine_unknowns:
        functor = None
        indices_to_sum = []
        try:
            # Parse the string "0+2+4" into a list of integers [0, 2, 4]
            indices_to_sum = [int(i) for i in args.combine_unknowns.split("+")]
            print(f"INFO: summing unknowns at specified indices: {indices_to_sum}")
        except (ValueError, AttributeError):
            print(
                f"ERROR: Invalid format for --combine-unknowns. Expected a string like '0+2+4', but got '{args.combine_unknowns}'."
            )
            exit()
        if len(indices_to_sum) < 2:
            print(
                f"ERROR: --combine-unknowns requires at least two indices, but got {len(indices_to_sum)}. Exiting."
            )
            exit()
            # Define a new function dynamically that captures the indices

        def selective_sum_functor(values):
            """Sums only the user-specified indices from the list of values."""
            if len(values) == 0:
                return [0.0]
            # If the data has already been combined (only 1 unknown left),
            # just return it to avoid out-of-bounds errors.
            if len(values) == 1:
                return values

            total = 0.0
            for index in indices_to_sum:
                if index < len(values):
                    total += values[index]
                else:
                    print(
                        f"WARNING: Index {index} is out of bounds for unknowns list (size {len(values)}). Skipping."
                    )
            return [total]

        functor = selective_sum_functor

        if functor:
            combine_filter = peano4.visualisation.filters.Calculator(
                functor=functor, output_cardinality=1, verbose=args.verbose
            )
            combine_filter.run_on_individual_pieces_of_data = True
            visualiser.append_filter(combine_filter)
    if args.filter_average:
        visualiser.append_filter(
            peano4.visualisation.filters.AverageOverCell(args.verbose)
        )
    if args.filter_fine_grid:
        visualiser.append_filter(
            peano4.visualisation.filters.ExtractFineGrid(False, args.verbose)
        )
    if args.norm_calculator:
        visualiser.append_filter(
            peano4.visualisation.filters.Calculator(verbose=args.verbose)
        )
    if args.shrink_cells != 1.0:
        visualiser.append_filter(
            peano4.visualisation.filters.ShrinkPatches(
                args.shrink_cells, verbose=args.verbose
            )
        )

    if args.single_file:
        visualiser.display_single_file()
    else:
        visualiser.display(args.max_workers)
