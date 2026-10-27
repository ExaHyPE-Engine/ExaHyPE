# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

import argparse
import dataclasses
import sys
from collections.abc import Collection

import numpy as np
from scipy.spatial import cKDTree

# The first import works in the package. The second one works in a direct run from the
# postprocessing/ directory.
try:
    from postprocessing.solution_io import MeshSnapshot, SolutionSource
except ImportError:
    from solution_io import MeshSnapshot, SolutionSource


MIRRORS = ("x", "y", "z", "xy", "xz", "yz")

SYMMETRIC_TENSOR_COMPONENTS = ("xx", "xy", "xz", "yy", "yz", "zz")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Check a solution output (any plotter format) for mirror symmetry.",
        epilog="Cell centres and node positions mirror bitwise about a domain centre at the "
        "origin. A PDE term that subtracts a point from a position stays bitwise symmetric "
        "only in such a domain.",
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
    parser.add_argument(
        "--atol",
        type=float,
        default=0.0,
        help="Absolute tolerance (default: 0, bitwise symmetry)",
    )
    parser.add_argument(
        "--frame", type=int, help="Check only this snapshot index (default: all)"
    )
    parser.add_argument(
        "--skip-fields",
        nargs="+",
        default=["Subdomain", "CurveIndex", "Number"],
        help="Fields to ignore (default: the decomposition fields Subdomain and CurveIndex and the "
        "particle identifier Number)",
    )
    parser.add_argument(
        "--vector-fields",
        nargs="+",
        default=[],
        help="Fields to transform as geometric vectors (default: every field with two or three "
        "components)",
    )
    parser.add_argument(
        "--tensor-fields",
        nargs="+",
        default=[],
        help="Fields to transform as rank-2 tensors (default: every 9-component field)",
    )
    parser.add_argument(
        "--symmetric-tensor-fields",
        nargs="+",
        default=[],
        help="Fields to transform as symmetric rank-2 tensors, written as FIELD for the six "
        "components xx, xy, xz, yy, yz and zz or as FIELD:COMPONENT,COMPONENT,... for another "
        "order, e.g. sigma:xx,yy,xy",
    )
    parser.add_argument(
        "--scalar-fields",
        nargs="+",
        default=[],
        help="Fields to treat as per-component scalars, e.g. packed state vectors",
    )
    parser.add_argument(
        "--vector-parts",
        nargs="+",
        default=[],
        help="Packed fields whose components FIRST to LAST form a geometric vector, written as "
        "FIELD:FIRST:LAST. A field takes several parts. Every component outside the parts of a "
        "field becomes a scalar field",
    )
    parser.add_argument(
        "--vector-groups",
        nargs="+",
        default=[],
        help="Scalar fields that form one geometric vector together, written as FIELD,FIELD or "
        "FIELD,FIELD,FIELD",
    )
    return parser


def symmetric_tensor_components(
    specifications: list[str],
) -> dict[str, tuple[str, ...]]:
    """Returns the component names of each symmetric tensor field, read from FIELD or from
    FIELD:COMPONENT,COMPONENT,..."""
    result = {}
    for specification in specifications:
        name, _, components = specification.partition(":")
        result[name] = (
            tuple(components.split(",")) if components else SYMMETRIC_TENSOR_COMPONENTS
        )
    return result


def field_kind(
    name: str,
    components: int,
    vector_fields: list[str],
    tensor_fields: list[str],
    scalar_fields: list[str],
    symmetric_tensor_fields: Collection[str] | None = None,
) -> str:
    if name in vector_fields:
        return "vector"
    if name in tensor_fields:
        return "tensor"
    if name in (symmetric_tensor_fields or []):
        return "symmetric tensor"
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


def mirror_values(
    values: np.ndarray,
    mirror: str,
    kind: str,
    components: tuple[str, ...] = SYMMETRIC_TENSOR_COMPONENTS,
) -> np.ndarray:
    if kind == "scalar" or values.ndim == 1 or values.shape[-1] == 1:
        return values
    if kind == "symmetric tensor":
        # The letters of a component name such as xy index the row and the column of its entry.
        rows = ["xyz".index(component[0]) for component in components]
        columns = ["xyz".index(component[1]) for component in components]
        tensor = np.zeros(values.shape[:-1] + (3, 3))
        tensor[..., rows, columns] = values
        tensor[..., columns, rows] = values
        mirrored = mirror_values(
            tensor.reshape(values.shape[:-1] + (9,)), mirror, "tensor"
        )
        return mirrored.reshape(values.shape[:-1] + (3, 3))[..., rows, columns]
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
    """Returns the corner coordinates, the centres and the sizes of the cells in float64."""
    points = np.asarray(snapshot.points, dtype=np.float64)
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError("Mixed cell types are not supported")
    corners = points[cells]
    centres = corners.mean(axis=1)
    sizes = corners.max(axis=1) - corners.min(axis=1)
    return corners, centres, sizes


def regroup_fields(
    snapshot: MeshSnapshot, vector_parts: list[str], vector_groups: list[str]
) -> MeshSnapshot:
    """Returns the snapshot with packed fields split and scalar fields grouped into vectors.

    A vector part FIELD:FIRST:LAST turns the components FIRST to LAST of FIELD into one vector
    field. Every component of FIELD outside its vector parts becomes a scalar field. A vector group
    FIELD,FIELD[,FIELD] turns its scalar fields into one vector field.
    """
    node_data = dict(snapshot.node_data)
    cell_data = dict(snapshot.cell_data)
    field_order = list(snapshot.field_order)

    def replace(names: list[str], fields: list[tuple[str, np.ndarray]]) -> None:
        position = [name for name, _ in field_order].index(names[0])
        centering = field_order[position][1]
        data = node_data if centering == "node" else cell_data
        for name in names:
            del data[name]
        kept = [entry for entry in field_order[:position] if entry[0] not in names]
        later = [entry for entry in field_order[position:] if entry[0] not in names]
        field_order[:] = kept + [(name, centering) for name, _ in fields] + later
        for name, values in fields:
            data[name] = values

    parts_of_field: dict[str, list[tuple[int, int]]] = {}
    for part in vector_parts:
        name, first, last = part.split(":")
        parts_of_field.setdefault(name, []).append((int(first), int(last)))
    for name, parts in parts_of_field.items():
        values = np.asarray(snapshot.field(name)[0])
        fields = [
            (f"{name}[{component}]", values[..., component])
            for component in range(values.shape[-1])
            if not any(first <= component <= last for first, last in parts)
        ]
        fields += [
            (f"{name}[{first}:{last}]", values[..., first : last + 1])
            for first, last in parts
        ]
        replace([name], fields)
    for group in vector_groups:
        names = group.split(",")
        columns = [np.asarray((node_data | cell_data)[name]) for name in names]
        values = np.stack([column.reshape(len(column)) for column in columns], axis=-1)
        replace(names, [(group, values)])
    return dataclasses.replace(
        snapshot, node_data=node_data, cell_data=cell_data, field_order=field_order
    )


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
    symmetric_tensor_fields: dict[str, tuple[str, ...]] | None = None,
) -> str | None:
    """Returns None if the snapshot is symmetric under the mirror, else a message.

    The symmetric tensor fields map each field to the names of its components.
    """
    corners, centres, sizes = cell_geometry(snapshot)
    dimension = centres.shape[1]
    mirrored_centres = mirror_transform(centres, centre, mirror)

    mirrored_sizes = sizes.copy()
    if len(mirror) == 2:
        first_axis, second_axis = "xyz".index(mirror[0]), "xyz".index(mirror[1])
        mirrored_sizes[:, [first_axis, second_axis]] = mirrored_sizes[
            :, [second_axis, first_axis]
        ]

    # The mirror arithmetic rounds the coordinates. Every mirrored cell pairs with its nearest
    # original cell.
    tree = cKDTree(np.column_stack([centres, sizes]))
    distances, partners = tree.query(
        np.column_stack([mirrored_centres, mirrored_sizes])
    )

    # The tolerance lies far above the float32 rounding of the coordinates and far
    # below the cell width.
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
                symmetric_tensor_fields,
            )
            values = mirror_values(
                values,
                mirror,
                kind,
                (symmetric_tensor_fields or {}).get(name, SYMMETRIC_TENSOR_COMPONENTS),
            )
        if centering == "cell":
            return values.reshape(len(centres), -1)
        # Nodal values sort by the offsets of their nodes from the cell centre. The quantisation of
        # the offsets lies well below the node spacing.
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

    tensor_components = symmetric_tensor_components(args.symmetric_tensor_fields)
    frames = [args.frame] if args.frame is not None else range(source.count)
    failed = False
    for index in frames:
        snapshot = regroup_fields(
            source.snapshot(index), args.vector_parts, args.vector_groups
        )
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
                tensor_components,
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
