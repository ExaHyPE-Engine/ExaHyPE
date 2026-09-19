# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import csv
import math
import sys

COLUMNS = ("t", "l1-error", "l2-error", "linf-error")
DEFAULT_COLUMN = "linf-error"


def parse_threshold(value: str) -> float:
    """Returns a non-negative threshold; an infinite one admits any finite value."""
    threshold = float(value)
    if math.isnan(threshold) or threshold < 0.0:
        raise argparse.ArgumentTypeError("The threshold has to be non-negative.")
    return threshold


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Checks an error-norm database against a threshold."
    )
    parser.add_argument("database", help="The CSV file an error measurement writes")
    parser.add_argument(
        "threshold", type=parse_threshold, help="The largest admissible value"
    )
    parser.add_argument(
        "--column",
        default=DEFAULT_COLUMN,
        help=f"Norm to check (default: {DEFAULT_COLUMN})",
    )
    return parser


def read_error_norms(path: str) -> tuple[list[str], list[list[str]]]:
    """Returns the column names and the measurement rows of an error database."""
    with open(path, encoding="utf-8", newline="") as database:
        rows = list(csv.reader(database, skipinitialspace=True))
    return (rows[0], rows[1:]) if rows else ([], [])


def worst_measurement(
    header: list[str], rows: list[list[str]], column: str
) -> tuple[float, float]:
    """Returns the time and the value of the row whose column is largest.

    A norm that is not a number wins, so a diverged run reports the snapshot
    that diverged.
    """
    index = header.index(column)
    measurements = [(float(row[0]), float(row[index])) for row in rows]
    return max(
        measurements,
        key=lambda measurement: (math.isnan(measurement[1]), measurement[1]),
    )


def main() -> None:
    args = build_parser().parse_args()

    try:
        header, rows = read_error_norms(args.database)
    except OSError as error:
        print(f"Cannot read {args.database}: {error}")
        sys.exit(2)
    if not rows:
        print(f"{args.database} holds no measurement.")
        sys.exit(2)
    if args.column not in header:
        print(
            f"{args.database} has no column {args.column}; "
            f"it holds {', '.join(header)}."
        )
        sys.exit(2)

    try:
        time, worst = worst_measurement(header, rows, args.column)
    except IndexError:
        print(f"{args.database} has a row with a missing field.")
        sys.exit(2)
    except ValueError as error:
        print(f"Cannot read {args.database}: {error}")
        sys.exit(2)
    print(
        f"{args.database}: worst {args.column} {worst:e} at t={time:e}, "
        f"threshold {args.threshold:e}"
    )
    if not worst <= args.threshold:
        print(f"The {args.column} does not stay within the threshold.")
        sys.exit(1)


if __name__ == "__main__":
    main()
