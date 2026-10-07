# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import sys

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components

# The first import works in the package, and the second one in a direct run from the
# postprocessing/ directory.
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


SKIPPED_FIELDS = (
    "CurveIndex",
    "LimiterStatus",
    "RefinementLevel",
    "Subdomain",
    "Subtree",
)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Integrate the fields of a solution output (any plotter format) over the "
        "domain in every snapshot and report their change against a reference snapshot. Cell "
        "data integrates as volume averages. Point data integrates with the Gauss-Legendre or "
        "Gauss-Lobatto quadrature of the Galerkin cells the plotted mesh connects. A Galerkin cell "
        "that the subcell limiter recomputes plots its subcells at its nodes, so a snapshot with such "
        "cells only approximates the integral; the report marks it."
    )
    parser.add_argument(
        "input", help="A solver name, an index/snapshot file, or an output directory"
    )
    parser.add_argument(
        "--fields",
        nargs="+",
        help="Fields to integrate (default: every field but "
        + ", ".join(SKIPPED_FIELDS)
        + ")",
    )
    parser.add_argument(
        "--reference",
        type=int,
        default=0,
        help="Index of the snapshot the changes refer to (default: 0)",
    )
    parser.add_argument(
        "--rtol",
        type=float,
        help="Fail when a relative change exceeds this tolerance (default: report only)",
    )
    parser.add_argument(
        "--atol",
        type=float,
        default=0.0,
        help="Absolute change below which an integral counts as conserved (default: 0)",
    )
    return parser


def cell_measures(snapshot: MeshSnapshot) -> np.ndarray:
    """Returns the area or volume of every axis-aligned cell of the snapshot."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    cells = snapshot.cells
    if isinstance(cells, np.ndarray):
        corners = points[cells]
        return np.prod(corners.max(axis=1) - corners.min(axis=1), axis=1)
    return np.asarray(
        [
            np.prod(points[cell].max(axis=0) - points[cell].min(axis=0))
            for cell in cells
        ]
    )


def galerkin_cells(snapshot: MeshSnapshot) -> list[np.ndarray]:
    """Returns the point indices of every connected component of the plotted mesh. Every
    Galerkin cell plots its nodes as a mesh of its own, so a component is one Galerkin cell."""
    cells = snapshot.cells
    rows, columns = [], []
    for cell in cells:
        cell = np.asarray(cell)
        rows.append(cell)
        columns.append(np.roll(cell, 1))
    rows = np.concatenate(rows)
    columns = np.concatenate(columns)
    size = len(snapshot.points)
    graph = coo_matrix((np.ones(len(rows)), (rows, columns)), shape=(size, size))
    count, labels = connected_components(graph, directed=False)
    order = np.argsort(labels, kind="stable")
    boundaries = np.searchsorted(labels[order], np.arange(count + 1))
    return [order[boundaries[c] : boundaries[c + 1]] for c in range(count)]


def unit_rule(kind: str, nodes: int) -> tuple[np.ndarray, np.ndarray]:
    """Returns the nodes and weights of the Gauss-Legendre or Gauss-Lobatto rule on [0, 1]."""
    if kind == "legendre":
        points, weights = np.polynomial.legendre.leggauss(nodes)
    else:
        degree = np.polynomial.legendre.Legendre.basis(nodes - 1)
        points = np.concatenate([[-1.0], np.sort(degree.deriv().roots().real), [1.0]])
        weights = 2.0 / (nodes * (nodes - 1) * degree(points) ** 2)
    return 0.5 * (points + 1.0), 0.5 * weights


def quadrature_weights(snapshot: MeshSnapshot, tolerance: float = 1e-4) -> np.ndarray:
    """Returns the quadrature weight of every point of a snapshot whose points are the
    tensor-product Gauss-Legendre or Gauss-Lobatto nodes of axis-aligned Galerkin cells."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    dimensions = points.shape[1]
    weights = np.zeros(len(points))
    for cell in galerkin_cells(snapshot):
        nodes = int(round(len(cell) ** (1.0 / dimensions)))
        if nodes < 2 or nodes**dimensions != len(cell):
            raise ValueError(
                f"A Galerkin cell of {len(cell)} points is no tensor-product layout in "
                f"{dimensions} dimensions."
            )
        positions = points[cell]
        lower, upper = positions.min(axis=0), positions.max(axis=0)
        cell_weights = None
        for kind in ("legendre", "lobatto"):
            unit_nodes, unit_weights = unit_rule(kind, nodes)
            width = (upper - lower) / (unit_nodes[-1] - unit_nodes[0])
            origin = lower - unit_nodes[0] * width
            local = (positions - origin) / width
            index = np.abs(local[..., None] - unit_nodes).argmin(axis=-1)
            if np.all(np.abs(local - unit_nodes[index]) < tolerance):
                cell_weights = np.prod(unit_weights[index] * width, axis=1)
                break
        if cell_weights is None:
            raise ValueError(
                "The points of a Galerkin cell are neither Gauss-Legendre nor Gauss-Lobatto "
                "nodes."
            )
        weights[cell] = cell_weights
    return weights


def integrate(
    snapshot: MeshSnapshot, fields: list[str], point_weights: np.ndarray | None = None
) -> dict[str, np.ndarray]:
    """Returns the integral of every component of every field over the domain."""
    result = {}
    measures = None
    for name in fields:
        values, location = snapshot.field(name)
        values = np.asarray(values, dtype=np.float64)
        if values.ndim == 1:
            values = values[:, None]
        if location == "cell":
            if measures is None:
                measures = cell_measures(snapshot)
            result[name] = measures @ values
        else:
            if point_weights is None:
                point_weights = quadrature_weights(snapshot)
            result[name] = point_weights @ values
    return result


def count_limited_cells(snapshot: MeshSnapshot) -> int:
    """Returns the number of Galerkin cells whose plot shows the subcells of the limiter. Every
    plotted cell of a Galerkin cell carries its LimiterStatus, Limited (2) or Troubled (3) there."""
    if "LimiterStatus" not in snapshot.cell_data:
        return 0
    status = np.asarray(snapshot.cell_data["LimiterStatus"]).reshape(-1)
    limited_points = np.unique(np.asarray(snapshot.cells)[status >= 2].reshape(-1))
    if len(limited_points) == 0:
        return 0
    return sum(
        1 for cell in galerkin_cells(snapshot) if np.isin(cell[0], limited_points)
    )


def default_fields(snapshot: MeshSnapshot) -> list[str]:
    return [name for name in snapshot.variables if name not in SKIPPED_FIELDS]


def main(argv: list[str] | None = None) -> None:
    args = build_parser().parse_args(argv)
    try:
        source = SolutionSource.locate(args.input)
    except Exception as error:
        print(f"Cannot read {args.input!r}: {error}", file=sys.stderr)
        sys.exit(2)

    reference_snapshot = source.snapshot(args.reference)
    fields = args.fields or default_fields(reference_snapshot)
    reference = integrate(reference_snapshot, fields)

    failed = False
    approximate = False
    for index in range(source.count):
        snapshot = source.snapshot(index)
        integrals = integrate(snapshot, fields)
        time = snapshot.time
        label = f"t={time:<12g}" if time is not None else f"frame {index:<8}"
        entries = []
        for name in fields:
            for component, (value, initial) in enumerate(
                zip(integrals[name], reference[name])
            ):
                change = value - initial
                relative = change / abs(initial) if initial != 0.0 else float("inf")
                suffix = f"[{component}]" if len(integrals[name]) > 1 else ""
                entries.append(f"{name}{suffix}={value:.12e} ({relative:+.3e})")
                if (
                    args.rtol is not None
                    and abs(change) > args.atol
                    and abs(relative) > args.rtol
                ):
                    failed = True
        limited = count_limited_cells(snapshot)
        if limited > 0:
            approximate = True
            entries.append(f"[{limited} limited cells: approximate]")
        print(f"  {label} " + "  ".join(entries))

    if approximate:
        print(
            "Snapshots with limited cells plot subcells at the nodes. Their integrals are no "
            "measure of conservation."
        )
    if failed:
        print(f"An integral changes by more than the relative tolerance {args.rtol:g}.")
        sys.exit(1)
    if args.rtol is not None:
        print(f"{source.describe()} conserves {', '.join(fields)} within {args.rtol:g}.")


if __name__ == "__main__":
    main()
