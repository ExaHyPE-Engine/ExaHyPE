# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os
import sys
from functools import partial
from typing import Callable, List, Optional

import visualisation.filters


def selective_sum(values, *, indices_to_sum):
    if len(values) == 0:
        return [0.0]
    if len(values) == 1:
        return values
    total = 0.0
    for idx in indices_to_sum:
        if idx < len(values):
            total += values[idx]
        else:
            print(
                f"WARNING: Index {idx} is out of bounds for unknowns list (size {len(values)}). Skipping."
            )
    return [total]


class Conversion:
    def __init__(self, args):
        self.args = args
        self.visualiser = None
        self.total_snapshot_number: int = 0
        self.start_snapshot: int = 0
        self.end_snapshot: int = 0
        self._indices_to_sum: List[int] = []

    def run(self) -> None:
        """Execute the full conversion pipeline."""
        self._sanity_checks()
        self._determine_snapshot_range()

        # Parse and store combine_unknowns once so subclasses and filters
        # can reuse the parsed list of indices without duplicating logic.
        self._indices_to_sum = self._parse_combine_unknowns() or []

        self.visualiser = self.build_visualiser(
            start=self.start_snapshot, end=self.end_snapshot
        )
        self._configure_filters()
        self._display()

    def build_visualiser(self, start: int, end: int):
        """Create and return a configured visualiser instance.
        Must be implemented by subclasses.
        """
        raise NotImplementedError

    def _sanity_checks(self) -> None:
        if not os.path.exists(self.args.filename):
            raise Exception(
                f"Specified input file '{self.args.filename}' does not exist, exiting..."
            )

        # Count snapshots by scanning for lines containing 'timestamp'
        self.total_snapshot_number = 0
        with open(self.args.filename, "r") as metafile:
            for line in metafile.readlines():
                if "timestamp" in line:
                    self.total_snapshot_number += 1

    def _determine_snapshot_range(self) -> None:
        args = self.args
        if (
            (args.start > args.end and args.end != -1)
            or args.start < 0
            or args.end < -1
        ):
            raise Exception("Specified snapshot index is wrong, please check.")
        elif args.end == -1:
            self.start_snapshot = args.start
            # Process all snapshots if no end snapshot is specified.
            self.end_snapshot = self.total_snapshot_number - 1
        elif args.end > (self.total_snapshot_number - 1):
            self.start_snapshot = args.start
            self.end_snapshot = self.total_snapshot_number - 1
            print(
                "WARNING: specified end snapshot index exceeds the total number, reset to the last snapshot."
            )
        else:
            self.start_snapshot = args.start
            self.end_snapshot = args.end

        print(
            "The meta file includes",
            self.total_snapshot_number,
            "snapshots, processing snapshot",
            self.start_snapshot,
            "to",
            self.end_snapshot,
        )

    def _parse_combine_unknowns(self) -> Optional[List[int]]:
        """Parse --combine-unknowns like '0+2+4' into [0,2,4]."""
        s = getattr(self.args, "combine_unknowns", None)
        if not s:
            return None
        try:
            indices = [int(i) for i in s.split("+")]
        except (ValueError, AttributeError):
            print(
                f"ERROR: Invalid format for --combine-unknowns. Expected a string like '0+2+4', but got '{s}'."
            )
            sys.exit(1)

        if len(indices) < 2:
            print(
                f"ERROR: --combine-unknowns requires at least two indices, but got {len(indices)}. Exiting."
            )
            sys.exit(1)
        print(f"Summing unknowns at specified indices: {indices}")
        return indices

    def _selective_sum_functor(
        self, indices_to_sum: List[int]
    ) -> Callable[[List[float]], List[float]]:
        """Return a functor that sums only the specified indices from the list of values."""

    def _selective_sum_functor(self, indices_to_sum):
        return partial(selective_sum, indices_to_sum=indices_to_sum)

    def _configure_filters(self) -> None:
        args = self.args
        # Use the indices parsed earlier in run (if any). _indices_to_sum
        # will be an empty list when not provided.
        indices = self._indices_to_sum
        if indices:
            functor = self._selective_sum_functor(indices)
            combine_filter = visualisation.filters.Calculator(
                functor=functor, output_cardinality=1, verbose=args.verbose
            )
            combine_filter.run_on_individual_pieces_of_data = True
            self.visualiser.append_filter(combine_filter)

        if getattr(args, "filter_average", False):
            self.visualiser.append_filter(
                visualisation.filters.AverageOverCell(args.verbose)
            )

        if getattr(args, "filter_fine_grid", False):
            self.visualiser.append_filter(
                visualisation.filters.ExtractFineGrid(False, args.verbose)
            )

        if getattr(args, "norm_calculator", False):
            # Use L2 (Euclidean) norm by default for proper vector magnitude calculation
            from peano4.visualisation.filters.Calculator import l2_norm

            self.visualiser.append_filter(
                visualisation.filters.Calculator(functor=l2_norm, verbose=args.verbose)
            )

        if getattr(args, "shrink_cells", 1.0) != 1.0:
            self.visualiser.append_filter(
                visualisation.filters.ShrinkPatches(
                    args.shrink_cells, verbose=args.verbose
                )
            )

    def _display(self) -> None:
        args = self.args
        if getattr(args, "single_file", False):
            self.visualiser.display_single_file()
        else:
            self.visualiser.display(args.max_workers)
