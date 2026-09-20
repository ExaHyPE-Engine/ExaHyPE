# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import sys

import numpy as np
from scipy.spatial import cKDTree

# The dual import works both as a package and when this script is
# run directly from the postprocessing/ directory (see render_solution.py).
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


MIRRORS = ("x", "y", "z", "xy", "xz", "yz")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Check a solution output (any plotter format) for mirror symmetry."
    )
    parser.add_argument(
        "input", help="A solver name, an index/snapshot file, or an output directory"
    )
    parser.add_argument(
        "--centre",
        type=float,
        nargs="+",
        help="Symmetry centre (default: the midpoint of the mesh bounding box)",
    )
    parser.add_argument(
        "--mirrors",
        nargs="+",
        choices=MIRRORS,
        help="Mirrors to check: an axis name reflects that axis, a pair swaps the two axes "
        "(default: every axis of the mesh plus the xy swap)",
    )
    parser.add_argument(
        "--rtol", type=float, default=0.0, help="Relative tolerance (default: 0)"
    )
    # Mirrored cells sum their kernel terms in a different node order, so one rounding
    # error per evaluation is admissible; a few hundred time steps stay well below this
    # multiple of the machine epsilon. Pass 0 to demand exact symmetry.
    parser.add_argument(
        "--atol",
        type=float,
        default=100.0 * sys.float_info.epsilon,
        help="Absolute tolerance (default: one hundred machine epsilons)",
    )
    parser.add_argument(
        "--frame", type=int, help="Check only this snapshot index (default: all)"
    )
    parser.add_argument(
        "--skip-fields",
        nargs="+",
        default=["Subdomain", "Subtree", "CurvePosition"],
        help="Fields to ignore (default: Subdomain, Subtree and CurvePosition, which depend on "
        "the domain decomposition and on the curve order)",
    )
    parser.add_argument(
        "--vector-fields",
        nargs="+",
        default=[],
        help="Fields to transform as geometric vectors (default: every 3-component field)",
    )
    parser.add_argument(
        "--tensor-fields",
        nargs="+",
        default=[],
        help="Fields to transform as rank-2 tensors (default: every 9-component field)",
    )
    parser.add_argument(
        "--scalar-fields",
        nargs="+",
        default=[],
        help="Fields to treat as per-component scalars, e.g. packed state vectors",
    )
    return parser


def field_kind(
    name: str,
    components: int,
    vector_fields: list[str],
    tensor_fields: list[str],
    scalar_fields: list[str],
) -> str:
    if name in vector_fields:
        return "vector"
    if name in tensor_fields:
        return "tensor"
    if name in scalar_fields:
        return "scalar"
    if components in (2, 3):
        return "vector"
    if components == 9:
        return "tensor"
    return "scalar"


def mirror_transform(
    coordinates: np.ndarray, centre: np.ndarray, mirror: str
) -> np.ndarray:
    result = coordinates.copy()
    if len(mirror) == 1:
        axis = "xyz".index(mirror)
        result[..., axis] = 2.0 * centre[axis] - result[..., axis]
    else:
        first_axis, second_axis = "xyz".index(mirror[0]), "xyz".index(mirror[1])
        result[..., first_axis], result[..., second_axis] = (
            coordinates[..., second_axis] - centre[second_axis] + centre[first_axis],
            coordinates[..., first_axis] - centre[first_axis] + centre[second_axis],
        )
    return result


def mirror_values(values: np.ndarray, mirror: str, kind: str) -> np.ndarray:
    if kind == "scalar" or values.ndim == 1 or values.shape[-1] == 1:
        return values
    if kind == "tensor":
        tensor = values.reshape(values.shape[:-1] + (3, 3)).copy()
        if len(mirror) == 1:
            axis = "xyz".index(mirror)
            tensor[..., axis, :] = -tensor[..., axis, :]
            tensor[..., :, axis] = -tensor[..., :, axis]
        else:
            first_axis, second_axis = "xyz".index(mirror[0]), "xyz".index(mirror[1])
            tensor[..., [first_axis, second_axis], :] = tensor[
                ..., [second_axis, first_axis], :
            ]
            tensor[..., :, [first_axis, second_axis]] = tensor[
                ..., :, [second_axis, first_axis]
            ]
        return tensor.reshape(values.shape)
    result = values.copy()
    if len(mirror) == 1:
        axis = "xyz".index(mirror)
        if axis < result.shape[-1]:
            result[..., axis] = -result[..., axis]
    else:
        first_axis, second_axis = "xyz".index(mirror[0]), "xyz".index(mirror[1])
        if second_axis < result.shape[-1]:
            result[..., [first_axis, second_axis]] = result[
                ..., [second_axis, first_axis]
            ]
    return result


def cell_geometry(snapshot: MeshSnapshot) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """Per-cell corner coordinates, centres and sizes in float64."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError("Mixed cell types are not supported")
    corners = points[cells]
    centres = corners.mean(axis=1)
    sizes = corners.max(axis=1) - corners.min(axis=1)
    return corners, centres, sizes


def check_snapshot(
    snapshot: MeshSnapshot,
    centre: np.ndarray,
    mirror: str,
    rtol: float,
    atol: float,
    skip_fields: list[str],
    vector_fields: list[str] | None = None,
    tensor_fields: list[str] | None = None,
    scalar_fields: list[str] | None = None,
) -> str | None:
    """Returns None if the snapshot is symmetric under the mirror, else a message."""
    corners, centres, sizes = cell_geometry(snapshot)
    dimension = centres.shape[1]
    mirrored_centres = mirror_transform(centres, centre, mirror)

    mirrored_sizes = sizes.copy()
    if len(mirror) == 2:
        first_axis, second_axis = "xyz".index(mirror[0]), "xyz".index(mirror[1])
        mirrored_sizes[:, [first_axis, second_axis]] = mirrored_sizes[
            :, [second_axis, first_axis]
        ]

    # Pair every mirrored cell with its nearest original cell. Sorting signature
    # rows instead mispairs cells whose stored coordinates sit across a rounding
    # boundary under the mirror arithmetic.
    tree = cKDTree(np.column_stack([centres, sizes]))
    distances, partners = tree.query(
        np.column_stack([mirrored_centres, mirrored_sizes])
    )

    # Geometry tolerance per pair: Far above the float32 storage rounding of the
    # coordinates and far below the cell's own width, so a cell displaced by a
    # fraction of its size never passes, however small the cell.
    per_cell_tolerance = 1.0e-3 * sizes.min(axis=1)
    if bool((distances > per_cell_tolerance).any()) or len(np.unique(partners)) != len(
        partners
    ):
        return "The mesh topology is not symmetric"

    def per_cell_values(name: str, transformed: bool) -> np.ndarray:
        values, centering = snapshot.field(name)
        values = np.asarray(values, dtype=np.float64)
        if transformed:
            components = values.shape[-1] if values.ndim > 1 else 1
            kind = field_kind(
                name,
                components,
                vector_fields or [],
                tensor_fields or [],
                scalar_fields or [],
            )
            values = mirror_values(values, mirror, kind)
        if centering == "cell":
            return values.reshape(len(centres), -1)
        # Nodal data: Bring each cell's nodes into a canonical order through the node
        # offsets from the cell centre, quantised well below the node spacing.
        base = mirrored_centres if transformed else centres
        offsets = (
            mirror_transform(corners, centre, mirror) if transformed else corners
        ) - base[:, None, :]
        quantised = np.round(64.0 * offsets / sizes[:, None, :]).astype(np.int64)
        node_ordering_key = sum(
            quantised[:, :, d] * (1000 ** (dimension - 1 - d)) for d in range(dimension)
        )
        rows = np.arange(len(centres))[:, None]
        return values[snapshot.cells][
            rows, np.argsort(node_ordering_key, axis=1)
        ].reshape(len(centres), -1)

    names = [name for name in snapshot.variables if name not in skip_fields]
    original = np.column_stack([per_cell_values(name, False) for name in names])
    mirrored = np.column_stack([per_cell_values(name, True) for name in names])
    if not np.allclose(original[partners], mirrored, rtol=rtol, atol=atol):
        worst = float(np.abs(original[partners] - mirrored).max())
        return f"The field values break the symmetry (worst deviation {worst:.6e})"
    return None


def main(argv: list[str] | None = None) -> None:
    args = build_parser().parse_args(argv)
    try:
        source = SolutionSource.locate(args.input)
    except Exception as error:
        print(f"Cannot read {args.input!r}: {error}", file=sys.stderr)
        sys.exit(2)

    first = source.snapshot(0)
    points = np.asarray(first.points, dtype=np.float64)
    dimension = points.shape[1]

    if args.centre is not None:
        centre = np.asarray(args.centre, dtype=np.float64)
    else:
        centre = 0.5 * (points.min(axis=0) + points.max(axis=0))

    if args.mirrors is not None:
        mirrors = args.mirrors
    else:
        mirrors = ["xyz"[d] for d in range(dimension)] + ["xy"]

    frames = [args.frame] if args.frame is not None else range(source.count)
    failed = False
    for index in frames:
        snapshot = source.snapshot(index)
        verdicts = []
        for mirror in mirrors:
            message = check_snapshot(
                snapshot,
                centre,
                mirror,
                args.rtol,
                args.atol,
                args.skip_fields,
                args.vector_fields,
                args.tensor_fields,
                args.scalar_fields,
            )
            if message is not None:
                failed = True
                verdicts.append(f"{mirror}: {message}")
        time = snapshot.time
        label = f"t={time:g}" if time is not None else f"frame {index}"
        if verdicts:
            print(f"  {label}: " + "; ".join(verdicts))
        else:
            print(
                f"  {label}: Symmetric under {', '.join(mirrors)} ({snapshot.num_cells} cells)"
            )

    if failed:
        print("The solution is not mirror-symmetric.")
        sys.exit(1)
    print(f"{source.describe()} is mirror-symmetric under {', '.join(mirrors)}.")


if __name__ == "__main__":
    main()
