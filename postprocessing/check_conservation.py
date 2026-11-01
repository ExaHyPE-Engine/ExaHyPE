# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import math
import sys

import numpy as np

# The first import works in the package. The second one works in a direct run from the
# postprocessing/ directory.
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Check that the domain integral of fields of a solution output stays "
        "constant over all snapshots.",
        epilog="A cell field integrates over the bounding boxes of the plotted cells. A node "
        "field integrates with the Gauss-Legendre quadrature of the cells of a discontinuous "
        "Galerkin solver, whose nodes the plotter writes cell by cell with the first axis "
        "fastest.",
    )
    parser.add_argument(
        "input", help="A solver name, an index/snapshot file, or an output directory"
    )
    parser.add_argument(
        "--fields",
        nargs="+",
        default=["rho"],
        help="Scalar fields to integrate (default: rho)",
    )
    parser.add_argument(
        "--gauss-legendre-nodes",
        type=int,
        help="Gauss-Legendre nodes per axis of a cell, required for node fields",
    )
    parser.add_argument(
        "--time-steps",
        type=int,
        required=True,
        help="Largest number of time steps between the first and the last snapshot",
    )
    parser.add_argument(
        "--precision",
        choices=("double", "float"),
        default="double",
        help="Storage precision of the solver (default: double)",
    )
    return parser


def get_round_off_bound(time_steps: int, precision: str) -> float:
    """Returns the largest change of an integral relative to the integral of its absolute
    value. Every time step rounds every stored value once. The measurement rounds once.
    """
    epsilon = np.finfo(np.float64 if precision == "double" else np.float32).eps
    return (time_steps + 1) * float(epsilon)


def integrate_cell_field(snapshot: MeshSnapshot, values: np.ndarray) -> np.ndarray:
    """Returns the integrals of values and of their absolute values over the plotted cells."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    cells = snapshot.cells
    if isinstance(cells, np.ndarray):
        corners = points[cells]
        widths = corners.max(axis=1) - corners.min(axis=1)
    else:
        widths = np.asarray(
            [points[cell].max(axis=0) - points[cell].min(axis=0) for cell in cells]
        )
    measures = np.prod(widths, axis=1)
    return np.asarray(
        [math.fsum(values * measures), math.fsum(np.abs(values) * measures)]
    )


def integrate_node_field(
    snapshot: MeshSnapshot, values: np.ndarray, nodes_per_axis: int
) -> np.ndarray:
    """Returns the integrals of values and of their absolute values with the Gauss-Legendre
    quadrature of nodes_per_axis nodes per axis."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    dimension = points.shape[1]
    nodes_per_cell = nodes_per_axis**dimension
    if points.shape[0] % nodes_per_cell != 0:
        raise ValueError(
            f"The conservation check expects a multiple of {nodes_per_cell} points in "
            f"{snapshot.name}, the nodes of a cell with {nodes_per_axis} Gauss-Legendre "
            "nodes per axis."
        )
    nodes, weights = np.polynomial.legendre.leggauss(nodes_per_axis)
    span_in_unit_interval = 0.5 * (nodes[-1] - nodes[0]) if nodes_per_axis > 1 else 1.0
    weights = 0.5 * weights
    weights_of_cell = weights
    for _ in range(1, dimension):
        weights_of_cell = np.multiply.outer(weights, weights_of_cell)
    weights_of_cell = weights_of_cell.reshape(-1)
    nodes_of_cells = points.reshape(-1, nodes_per_cell, dimension)
    widths = (
        nodes_of_cells.max(axis=1) - nodes_of_cells.min(axis=1)
    ) / span_in_unit_interval
    measures = np.prod(widths, axis=1)
    values_of_cells = values.reshape(-1, nodes_per_cell)
    return np.asarray(
        [
            math.fsum(measures * (values_of_cells @ weights_of_cell)),
            math.fsum(measures * (np.abs(values_of_cells) @ weights_of_cell)),
        ]
    )


def integrate(
    snapshot: MeshSnapshot, name: str, nodes_per_axis: int | None
) -> np.ndarray:
    """Returns the integral of the field name and the integral of its absolute value."""
    values, location = snapshot.field(name)
    values = np.asarray(values, dtype=np.float64)
    if values.ndim != 1:
        raise ValueError("The argument --fields accepts scalar fields only.")
    if location == "cell":
        return integrate_cell_field(snapshot, values)
    if nodes_per_axis is None:
        raise ValueError(
            f"The node field {name!r} needs the argument --gauss-legendre-nodes."
        )
    return integrate_node_field(snapshot, values, nodes_per_axis)


def main(argv: list[str] | None = None) -> None:
    args = build_parser().parse_args(argv)
    try:
        source = SolutionSource.locate(args.input)
    except Exception as error:
        print(f"Cannot read {args.input!r}: {error}", file=sys.stderr)
        sys.exit(2)

    bound = get_round_off_bound(args.time_steps, args.precision)
    initial = {}
    failed = False
    for index in range(source.count):
        snapshot = source.snapshot(index)
        verdicts = []
        for name in args.fields:
            integral, absolute = integrate(snapshot, name, args.gauss_legendre_nodes)
            if name not in initial:
                initial[name] = (integral, absolute)
            change = (integral - initial[name][0]) / initial[name][1]
            if abs(change) > bound:
                failed = True
            verdicts.append(f"{name} {integral:.17g} (relative change {change:.3e})")
        time = snapshot.time
        label = f"t={time:g}" if time is not None else f"frame {index}"
        print(f"  {label}: " + "; ".join(verdicts))

    if failed:
        print(
            f"An integral of {', '.join(args.fields)} changes by more than {bound:.3e}."
        )
        sys.exit(1)
    print(f"{source.describe()} conserves the integrals of {', '.join(args.fields)}.")


if __name__ == "__main__":
    main()
