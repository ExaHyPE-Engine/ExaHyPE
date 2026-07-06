#!/usr/bin/env python3
"""Compare two solution outputs field by field and report the first mismatch.

Both inputs are read through the shared :mod:`solution_io` readers, so any plotter
output format works - VTKHDF, HDF5/XDMF, NetCDF/UGRID or VTU - and the two inputs
need not even be in the same format. The comparison is numeric (with tolerances)
rather than byte-for-byte, so float32 storage, compression, attribute order or a
reversed space-filling-curve ordering do not register as differences:

  * structure - snapshot count, time stamps, spatial dimension, point and cell
    counts, the set of field names and their component counts;
  * geometry - point coordinates and cell connectivity;
  * data - every field's values.

Pass ``--unordered`` when the two outputs enumerate the same mesh in a different
order (common across formats or runs). Then each cell is reduced to a signature -
its vertex coordinates plus its cell-field values and the node-field values at its
vertices - and the sorted signatures are compared, which is invariant to cell and
point ordering and to duplicated patch-boundary vertices.

Exit status: 0 if the solutions match, 1 on the first mismatch, 2 if an input
cannot be read or compared.
"""

from __future__ import annotations

import argparse
import math
import sys

import numpy as np

# Shared readers; the dual import works both as a package and when this script is
# run directly from the postprocessing/ directory (see render_solution.py).
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare two solution outputs (any plotter format) and report the first mismatch."
    )
    parser.add_argument("left", help="First solution (solver name, index/snapshot file, or output directory)")
    parser.add_argument("right", help="Second solution (solver name, index/snapshot file, or output directory)")
    parser.add_argument("--rtol", type=float, default=1.0e-5, help="Relative tolerance (default: 1e-5)")
    parser.add_argument("--atol", type=float, default=1.0e-8, help="Absolute tolerance (default: 1e-8)")
    parser.add_argument("--field", help="Compare only this field (default: all fields)")
    parser.add_argument(
        "--unordered",
        action="store_true",
        help="Match cells and points by location before comparing, for outputs whose "
        "ordering differs (e.g. across formats or runs).",
    )
    return parser


def main(argv: list[str] | None = None) -> None:
    args = build_parser().parse_args(argv)
    try:
        left = SolutionSource.locate(args.left)
        right = SolutionSource.locate(args.right)
        print(f"Left  {args.left!r}: {left.describe()}", file=sys.stderr)
        print(f"Right {args.right!r}: {right.describe()}", file=sys.stderr)
        order = "unordered (matched by location)" if args.unordered else "ordered"
        scope = f"field {args.field!r}" if args.field else "all fields"
        print(
            f"Comparing {scope}, {order}, rtol={args.rtol:g} atol={args.atol:g} ...",
            file=sys.stderr,
        )
        mismatch = compare_sources(
            left, right, rtol=args.rtol, atol=args.atol, field=args.field, unordered=args.unordered
        )
    except (OSError, ValueError, KeyError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(2)

    if mismatch is None:
        print(f'Solutions match: "{args.left}" and "{args.right}".')
        raise SystemExit(0)
    print(f"Mismatch: {mismatch}", file=sys.stderr)
    raise SystemExit(1)


def compare_sources(
    left: SolutionSource,
    right: SolutionSource,
    *,
    rtol: float,
    atol: float,
    field: str | None = None,
    unordered: bool = False,
) -> str | None:
    if left.count != right.count:
        return f"snapshot count differs: {left.count} != {right.count}."
    for index in range(left.count):
        message = _compare_times(left.times[index], right.times[index], index, rtol, atol)
        if message is not None:
            return message
        message = compare_snapshots(
            left.snapshot(index), right.snapshot(index), index, rtol, atol, field, unordered
        )
        if message is not None:
            return message
    return None


def compare_snapshots(
    left: MeshSnapshot,
    right: MeshSnapshot,
    index: int,
    rtol: float,
    atol: float,
    field: str | None,
    unordered: bool,
) -> str | None:
    if left.points.shape[1] != right.points.shape[1]:
        return f"snapshot {index} spatial dimension differs: {left.points.shape[1]} != {right.points.shape[1]}."
    if left.points.shape[0] != right.points.shape[0]:
        return f"snapshot {index} point count differs: {left.points.shape[0]} != {right.points.shape[0]}."
    if left.num_cells != right.num_cells:
        return f"snapshot {index} cell count differs: {left.num_cells} != {right.num_cells}."

    left_fields, right_fields = set(left.variables), set(right.variables)
    if left_fields != right_fields:
        return (
            f"snapshot {index} fields differ: only in left {sorted(left_fields - right_fields)}, "
            f"only in right {sorted(right_fields - left_fields)}."
        )
    if field is not None and field not in left_fields:
        return f"field {field!r} is not present (available: {sorted(left_fields)})."
    selected = [field] if field is not None else left.variables

    # Centering and component counts must agree before any value comparison.
    for name in selected:
        left_values, left_center = left.field(name)
        right_values, right_center = right.field(name)
        if left_center != right_center:
            return f"snapshot {index} field {name!r} centering differs: {left_center} != {right_center}."
        left_shape = np.atleast_1d(np.asarray(left_values)).shape[1:]
        right_shape = np.atleast_1d(np.asarray(right_values)).shape[1:]
        if left_shape != right_shape:
            return (
                f"snapshot {index} field {name!r} component count differs: {left_shape} != {right_shape}."
            )

    if unordered:
        return _compare_unordered(left, right, index, selected, rtol, atol)
    return _compare_ordered(left, right, index, selected, rtol, atol)


def _compare_ordered(
    left: MeshSnapshot, right: MeshSnapshot, index: int, selected, rtol: float, atol: float
) -> str | None:
    message = _compare_array(left.points, right.points, f"snapshot {index} point coordinates", rtol, atol)
    if message is not None:
        return message
    message = _compare_cells(left.cells, right.cells, index)
    if message is not None:
        return message
    for name in selected:
        message = _compare_array(
            left.field(name)[0], right.field(name)[0], f"snapshot {index} field {name!r}", rtol, atol
        )
        if message is not None:
            return message
    return None


def _compare_unordered(
    left: MeshSnapshot, right: MeshSnapshot, index: int, selected, rtol: float, atol: float
) -> str | None:
    message = _compare_array(
        _sorted_rows(left.points), _sorted_rows(right.points), f"snapshot {index} point coordinates", rtol, atol
    )
    if message is not None:
        return message

    # One signature per cell (vertex coordinates + cell-field values + node-field values at the
    # cell's vertices). Sorting the signatures is invariant to cell and point ordering, ties each
    # node value to its cell so duplicated patch-boundary vertices stay aligned, and includes the
    # vertex coordinates so a different connectivity is still detected.
    left_signatures = _cell_signatures(left, selected)
    right_signatures = _cell_signatures(right, selected)
    if left_signatures.shape != right_signatures.shape:
        return (
            f"snapshot {index} cells differ in shape (topology mismatch): "
            f"{left_signatures.shape} != {right_signatures.shape}."
        )
    return _compare_array(
        _sorted_rows(left_signatures),
        _sorted_rows(right_signatures),
        f"snapshot {index} cells (geometry and field values, matched by location)",
        rtol,
        atol,
    )


def _cell_signatures(snapshot: MeshSnapshot, selected) -> np.ndarray:
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError("--unordered comparison requires uniform-topology meshes; use the ordered comparison.")
    count = cells.shape[0]
    parts = [snapshot.points[cells].reshape(count, -1)]  # vertex coordinates: geometry + connectivity
    for name in selected:
        values, center = snapshot.field(name)
        values = np.asarray(values, dtype=float)
        if center == "cell":
            parts.append(values.reshape(count, -1))
        else:
            parts.append(values[cells].reshape(count, -1))  # node values at the cell's vertices
    return np.concatenate(parts, axis=1)


def _compare_times(
    left: float | None, right: float | None, index: int, rtol: float, atol: float
) -> str | None:
    # A missing time stamp is encoded as None (XDMF grid without <Time>) or NaN (formats that
    # derive the time from a filename or step index); treat both as "unknown" and skip the check.
    left = float("nan") if left is None else left
    right = float("nan") if right is None else right
    if math.isnan(left) or math.isnan(right):
        return None
    if not math.isclose(left, right, rel_tol=rtol, abs_tol=atol):
        return f"snapshot {index} time differs: {left} != {right}."
    return None


def _compare_array(left, right, label: str, rtol: float, atol: float) -> str | None:
    left = np.asarray(left, dtype=float)
    right = np.asarray(right, dtype=float)
    if left.shape != right.shape:
        return f"{label} shape differs: {left.shape} != {right.shape}."
    if left.size == 0 or np.array_equal(left, right):
        return None
    close = np.isclose(left, right, rtol=rtol, atol=atol, equal_nan=True)
    if close.all():
        return None
    position = tuple(int(value) for value in np.argwhere(~close)[0])
    return (
        f"{label} differs at index {position}: {float(left[position])!r} != {float(right[position])!r} "
        f"(rtol={rtol:g}, atol={atol:g})."
    )


def _compare_cells(left_cells, right_cells, index: int) -> str | None:
    left_array = isinstance(left_cells, np.ndarray)
    right_array = isinstance(right_cells, np.ndarray)
    if left_array and right_array:
        if left_cells.shape != right_cells.shape:
            return f"snapshot {index} connectivity shape differs: {left_cells.shape} != {right_cells.shape}."
        if not np.array_equal(left_cells, right_cells):
            cell = int(np.argwhere(np.any(left_cells != right_cells, axis=1))[0][0])
            return f"snapshot {index} connectivity differs at cell {cell}."
        return None
    for cell, (left_nodes, right_nodes) in enumerate(zip(left_cells, right_cells, strict=True)):
        if not np.array_equal(np.asarray(left_nodes), np.asarray(right_nodes)):
            return f"snapshot {index} connectivity differs at cell {cell}."
    return None


def _sorted_rows(matrix: np.ndarray) -> np.ndarray:
    matrix = np.asarray(matrix, dtype=float)
    if matrix.ndim == 1:
        matrix = matrix[:, None]
    keys = tuple(np.round(matrix[:, col], 9) for col in range(matrix.shape[1] - 1, -1, -1))
    return matrix[np.lexsort(keys)]


if __name__ == "__main__":
    main()
