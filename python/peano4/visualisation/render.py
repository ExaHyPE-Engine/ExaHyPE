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
    parser.add_argument(
        "--eliminate-relative-paths",
        action="store_true",
        default=False,
        help="If you invoke the script on a different directory than your current working directory, ensure that all meta files written do not hold relative paths",
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

    visualiser = None
    if args.type == "vtu-unstructured":
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
