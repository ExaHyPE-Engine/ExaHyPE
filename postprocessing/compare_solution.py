# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import math
import sys

import numpy as np
from scipy.spatial import cKDTree

# The dual import works both as a package and when this script is
# run directly from the postprocessing/ directory (see render_solution.py).
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Compare two solution outputs (any plotter format) and report the first mismatch."
    )
    parser.add_argument(
        "left",
        help="First solution (solver name, index/snapshot file, or output directory)",
    )
    parser.add_argument(
        "right",
        help="Second solution (solver name, index/snapshot file, or output directory)",
    )
    parser.add_argument(
        "--rtol", type=float, default=0.0, help="Relative tolerance (default: 0, exact)"
    )
    parser.add_argument(
        "--atol", type=float, default=0.0, help="Absolute tolerance (default: 0, exact)"
    )
    parser.add_argument("--field", help="Compare only this field (default: all fields)")
    parser.add_argument(
        "--skip-fields",
        nargs="+",
        default=["Subdomain"],
        help="Fields to ignore (default: Subdomain, which depends on the domain decomposition)",
    )
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
            left,
            right,
            rtol=args.rtol,
            atol=args.atol,
            field=args.field,
            unordered=args.unordered,
            skip_fields=args.skip_fields,
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
    skip_fields: list[str] | None = None,
) -> str | None:
    if left.count != right.count:
        return f"Snapshot count differs: {left.count} != {right.count}."
    for index in range(left.count):
        message = _compare_times(
            left.times[index], right.times[index], index, rtol, atol
        )
        if message is not None:
            return message
        message = compare_snapshots(
            left.snapshot(index),
            right.snapshot(index),
            index,
            rtol,
            atol,
            field,
            unordered,
            skip_fields,
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
    skip_fields: list[str] | None = None,
) -> str | None:
    if left.points.shape[1] != right.points.shape[1]:
        return f"Snapshot {index} spatial dimension differs: {left.points.shape[1]} != {right.points.shape[1]}."
    if not unordered and left.points.shape[0] != right.points.shape[0]:
        # Unordered comparisons work on per-cell signatures, so runs may duplicate
        # patch-boundary vertices differently and still match.
        return f"Snapshot {index} point count differs: {left.points.shape[0]} != {right.points.shape[0]}."
    if left.num_cells != right.num_cells:
        return f"Snapshot {index} cell count differs: {left.num_cells} != {right.num_cells}."

    # Select the comparison scope first: Only the selected fields have to exist and agree on
    # both sides. A skipped field present on one side only (e.g. a decomposition-dependent
    # Subdomain marker) or an unselected extra field under --field is no mismatch.
    left_fields, right_fields = set(left.variables), set(right.variables)
    if field is not None:
        if field not in left_fields or field not in right_fields:
            return (
                f"Field {field!r} is not present in both inputs "
                f"(left: {sorted(left_fields)}, right: {sorted(right_fields)})."
            )
        selected = [field]
    else:
        skipped = set(skip_fields or [])
        left_selected, right_selected = left_fields - skipped, right_fields - skipped
        if left_selected != right_selected:
            return (
                f"Snapshot {index} fields differ: only in left {sorted(left_selected - right_selected)}, "
                f"only in right {sorted(right_selected - left_selected)}."
            )
        selected = [name for name in left.variables if name in left_selected]

    # Centering and component counts must agree before any value comparison.
    for name in selected:
        left_values, left_center = left.field(name)
        right_values, right_center = right.field(name)
        if left_center != right_center:
            return f"Snapshot {index} field {name!r} centering differs: {left_center} != {right_center}."
        left_shape = np.atleast_1d(np.asarray(left_values)).shape[1:]
        right_shape = np.atleast_1d(np.asarray(right_values)).shape[1:]
        if left_shape != right_shape:
            return f"Snapshot {index} field {name!r} component count differs: {left_shape} != {right_shape}."

    if unordered:
        return _compare_unordered(left, right, index, selected, rtol, atol)
    return _compare_ordered(left, right, index, selected, rtol, atol)


def _compare_ordered(
    left: MeshSnapshot,
    right: MeshSnapshot,
    index: int,
    selected: list[str],
    rtol: float,
    atol: float,
) -> str | None:
    message = _compare_array(
        left.points, right.points, f"Snapshot {index} point coordinates", rtol, atol
    )
    if message is not None:
        return message
    message = _compare_cells(left.cells, right.cells, index)
    if message is not None:
        return message
    for name in selected:
        message = _compare_array(
            left.field(name)[0],
            right.field(name)[0],
            f"Snapshot {index} field {name!r}",
            rtol,
            atol,
        )
        if message is not None:
            return message
    return None


def _compare_unordered(
    left: MeshSnapshot,
    right: MeshSnapshot,
    index: int,
    selected: list[str],
    rtol: float,
    atol: float,
) -> str | None:
    # One signature per cell (vertex coordinates + cell-field values + node-field values at the
    # cell's vertices, with the vertices in canonical order). Cells are paired by a
    # nearest-neighbour query on (centre, size): Distinct cells are separated by at least one
    # cell width, far above any storage rounding, so the pairing holds at every mesh scale
    # where a sorted-signature comparison would misalign rows on rounding noise.
    left_signatures, left_keys = _cell_signatures(left, selected)
    right_signatures, right_keys = _cell_signatures(right, selected)
    if left_signatures.shape != right_signatures.shape:
        return (
            f"Snapshot {index} cells differ in shape (topology mismatch): "
            f"{left_signatures.shape} != {right_signatures.shape}."
        )

    geometry_tolerance = 0.45 * min(left_keys[:, -1].min(), right_keys[:, -1].min())
    distances, partners = cKDTree(left_keys).query(right_keys)
    if float(distances.max()) > geometry_tolerance or len(np.unique(partners)) != len(
        partners
    ):
        return (
            f"Snapshot {index} cells cannot be matched by location (topology mismatch)."
        )

    return _compare_array(
        left_signatures[partners],
        right_signatures,
        f"Snapshot {index} cells (geometry and field values, matched by location)",
        rtol,
        atol,
    )


def _cell_signatures(
    snapshot: MeshSnapshot, selected: list[str]
) -> tuple[np.ndarray, np.ndarray]:
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError(
            "The --unordered comparison requires uniform-topology meshes; use the ordered comparison."
        )
    count = cells.shape[0]
    nodes_per_cell = cells.shape[1]

    # Per-vertex features: Coordinates plus the node-field values at the vertex. Cell-centred
    # values join the signature after the vertex block.
    corners = np.asarray(snapshot.points, dtype=float)[cells]
    vertex_parts = [corners]
    cell_parts = []
    for name in selected:
        values, center = snapshot.field(name)
        values = np.asarray(values, dtype=float)
        if center == "cell":
            cell_parts.append(values.reshape(count, -1))
        else:
            vertex_parts.append(values[cells].reshape(count, nodes_per_cell, -1))
    vertex_features = np.concatenate(vertex_parts, axis=2)

    # Canonicalise the local vertex order: Sorting each cell's vertex rows makes the signature
    # invariant under rotated or reflected connectivity of the same cell. Vertices of one cell
    # that differ do so by a full cell extent in some axis, while the same coordinate can carry
    # rounding noise from different writers, so the sort keys are the coordinates quantised to
    # half the cell's own extent per axis, which is exact at every mesh scale and aspect ratio
    # and immune to that noise.
    dimensions = np.asarray(snapshot.points, dtype=float).shape[1]
    extent_per_axis = corners.max(axis=1) - corners.min(axis=1)
    quantum = 0.5 * np.where(extent_per_axis > 0.0, extent_per_axis, 1.0)[:, None, :]
    quantised = np.round(
        (corners - corners.min(axis=1, keepdims=True)) / quantum
    ).astype(np.int64)
    keys = tuple(
        quantised[:, :, component] for component in range(dimensions - 1, -1, -1)
    )
    order = np.lexsort(keys, axis=1)
    vertex_features = np.take_along_axis(vertex_features, order[:, :, None], axis=1)

    signatures = np.concatenate(
        [vertex_features.reshape(count, -1)] + cell_parts, axis=1
    )
    matching_keys = np.column_stack(
        [corners.mean(axis=1), (corners.max(axis=1) - corners.min(axis=1)).max(axis=1)]
    )
    return signatures, matching_keys


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
        return f"Snapshot {index} time differs: {left} != {right}."
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
            return f"Snapshot {index} connectivity shape differs: {left_cells.shape} != {right_cells.shape}."
        if not np.array_equal(left_cells, right_cells):
            cell = int(np.argwhere(np.any(left_cells != right_cells, axis=1))[0][0])
            return f"Snapshot {index} connectivity differs at cell {cell}."
        return None
    for cell, (left_nodes, right_nodes) in enumerate(
        zip(left_cells, right_cells, strict=True)
    ):
        if not np.array_equal(np.asarray(left_nodes), np.asarray(right_nodes)):
            return f"Snapshot {index} connectivity differs at cell {cell}."
    return None


if __name__ == "__main__":
    main()
