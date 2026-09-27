# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from __future__ import annotations

from dataclasses import dataclass, field as dataclass_field
import json
import math
import os
from pathlib import Path
import re
import xml.etree.ElementTree as ET

import numpy as np

# --------------------------------------------------------------------------- #
# XDMF topology tables
# --------------------------------------------------------------------------- #
# XDMF mixed-topology element code -> number of nodes (HDF5 /topology stream).
XDMF_NODE_COUNTS = {4: 3, 5: 4, 6: 4, 7: 5, 8: 6, 9: 8}
# XDMF poly types carry an explicit node count in the stream instead.
XDMF_POLY_TYPES = {1, 2, 3}


# --------------------------------------------------------------------------- #
# Common mesh model
# --------------------------------------------------------------------------- #
@dataclass
class MeshSnapshot:
    """One snapshot in a common form, whatever format it was read from.

    ``cells`` is an ``(Ncells, k)`` integer array when every cell has the same
    number of corners, otherwise a list of per-cell index arrays. ``points`` is
    ``(Npoints, dim)`` with ``dim`` 2 or 3. Scalar fields are 1D, vector and
    tensor fields keep their component axis.
    """

    name: str
    time: float | None
    points: np.ndarray
    cells: object
    node_data: dict[str, np.ndarray]
    cell_data: dict[str, np.ndarray]
    field_order: list[tuple[str, str]] = dataclass_field(default_factory=list)
    source: Path | None = None

    @property
    def variables(self) -> list[str]:
        return [name for name, _ in self.field_order]

    @property
    def num_cells(self) -> int:
        cells = self.cells
        return cells.shape[0] if isinstance(cells, np.ndarray) else len(cells)

    def field(self, name: str) -> tuple[np.ndarray, str]:
        if name in self.node_data:
            return self.node_data[name], "node"
        if name in self.cell_data:
            return self.cell_data[name], "cell"
        raise KeyError(f"No field named {name!r} in {self.name}")


@dataclass
class SnapshotRef:
    """A picklable description of where one snapshot lives.

    A worker process, or a lazy viewer, can load the snapshot on demand without
    re-parsing an index.
    """

    fmt: str
    path: str
    time: float | None
    name: str
    payload: dict = dataclass_field(default_factory=dict)


def _squeeze_field(values: np.ndarray) -> np.ndarray:
    """A single-column field is a scalar; keep everything else as written."""
    values = np.asarray(values)
    if values.ndim == 2 and values.shape[1] == 1:
        return values[:, 0]
    return values


def _pack_cells(cell_list: list[np.ndarray]) -> object:
    """An ``(Ncells, k)`` array when every cell has the same corner count, else a list."""
    if not cell_list:
        return np.empty((0, 0), dtype=np.int64)
    widths = {len(cell) for cell in cell_list}
    if len(widths) == 1:
        return np.asarray(cell_list, dtype=np.int64)
    return [np.asarray(cell, dtype=np.int64) for cell in cell_list]


def _ordered_field_names(
    node_data: dict[str, np.ndarray], cell_data: dict[str, np.ndarray]
) -> list[tuple[str, str]]:
    return [(name, "node") for name in node_data] + [
        (name, "cell") for name in cell_data
    ]


# --------------------------------------------------------------------------- #
# Snapshot time encoded in a file name (e.g. elastic-0_500000.h5 -> 0.5)
# --------------------------------------------------------------------------- #
def _time_from_name(path: Path) -> float:
    try:
        return float(path.stem.rsplit("-", 1)[1].replace("_", "."))
    except (IndexError, ValueError):
        return float("nan")


def _time_sort_key(path: Path):
    time = _time_from_name(path)
    return (1, 0.0, path.name) if time != time else (0, time, path.name)


# --------------------------------------------------------------------------- #
# VTKHDF reader (single growing file, all steps concatenated)
# --------------------------------------------------------------------------- #
def _scan_vtkhdf(path: Path) -> tuple[str, list[SnapshotRef]]:
    import h5py

    with h5py.File(path, "r") as handle:
        group = handle["VTKHDF"]
        if "Steps" in group:
            times = np.asarray(group["Steps"]["Values"]).reshape(-1)
            count = int(group["Steps"].attrs.get("NSteps", len(times)))
        else:
            times = np.array([float("nan")])
            count = 1
    refs = [
        SnapshotRef("vtkhdf", str(path), float(times[i]), path.stem, {"step": i})
        for i in range(count)
    ]
    return path.stem, refs


def _load_vtkhdf_step(ref: SnapshotRef) -> MeshSnapshot:
    import h5py

    step = ref.payload["step"]
    with h5py.File(ref.path, "r") as handle:
        group = handle["VTKHDF"]
        n_points = np.asarray(group["NumberOfPoints"]).reshape(-1)
        n_cells = np.asarray(group["NumberOfCells"]).reshape(-1)
        n_conn = np.asarray(group["NumberOfConnectivityIds"]).reshape(-1)
        if "Steps" in group:
            steps = group["Steps"]
            point_offsets = np.asarray(steps["PointOffsets"]).reshape(-1)
            cell_offsets = np.asarray(steps["CellOffsets"]).reshape(-1)
            conn_offsets = np.asarray(steps["ConnectivityIdOffsets"]).reshape(-1)
        else:
            point_offsets = cell_offsets = conn_offsets = np.zeros(1, dtype=np.int64)

        npt = int(n_points[step])
        ncl = int(n_cells[step])
        ncn = int(n_conn[step])
        point_start = int(point_offsets[step])
        cell_start = int(cell_offsets[step])
        conn_start = int(conn_offsets[step])

        points = np.asarray(
            group["Points"][point_start : point_start + npt, :], dtype=float
        )

        # Each step's Offsets block restarts at zero and has Ncells + 1 entries;
        # its block starts after every earlier step's block.
        block_start = int(np.sum(n_cells[:step] + 1))
        offsets_block = np.asarray(
            group["Offsets"][block_start : block_start + ncl + 1]
        )
        connectivity = np.asarray(group["Connectivity"][conn_start : conn_start + ncn])
        cells = _cells_from_offsets(connectivity, offsets_block, ncl)

        node_data = _read_vtkhdf_fields(group, "PointData", point_start, npt)
        cell_data = _read_vtkhdf_fields(group, "CellData", cell_start, ncl)

    points = _drop_trivial_z(points)
    return MeshSnapshot(
        name=ref.name,
        time=ref.time,
        points=points,
        cells=cells,
        node_data=node_data,
        cell_data=cell_data,
        field_order=_ordered_field_names(node_data, cell_data),
        source=Path(ref.path),
    )


def _read_vtkhdf_fields(
    group, kind: str, start: int, length: int
) -> dict[str, np.ndarray]:
    if kind not in group:
        return {}
    fields = {}
    for name, dataset in group[kind].items():
        fields[name] = _squeeze_field(np.asarray(dataset[start : start + length, :]))
    return fields


def _cells_from_offsets(
    connectivity: np.ndarray, offsets: np.ndarray, num_cells: int
) -> object:
    """Turn a flat connectivity stream plus its (num_cells + 1) offsets into cells.

    Cells that all have the same corner count are reshaped in one go.
    """
    if num_cells == 0:
        return np.empty((0, 0), dtype=np.int64)
    widths = np.diff(offsets)
    if np.all(widths == widths[0]):
        return connectivity.reshape(num_cells, int(widths[0])).astype(np.int64)
    return _pack_cells(
        [connectivity[offsets[c] : offsets[c + 1]] for c in range(num_cells)]
    )


# --------------------------------------------------------------------------- #
# HDF5 / XDMF reader (temporal index inlines a grid per step)
# --------------------------------------------------------------------------- #
def _data_item_ref(index_dir: Path, text: str | None) -> tuple[Path, str]:
    if text is None:
        raise ValueError("Missing DataItem reference")
    location = text.strip()
    file_name, dataset = location.split(":/", 1)
    return (index_dir / file_name).resolve(), dataset


def _grid_to_payload(index_dir: Path, grid: ET.Element) -> dict:
    topology = grid.find("Topology")
    geometry = grid.find("Geometry")
    if topology is None or geometry is None:
        raise ValueError("XDMF grid is missing Topology or Geometry")
    topology_ref = _data_item_ref(index_dir, topology.find("DataItem").text)
    # XYZ geometry is one interleaved array; X_Y_Z is one DataItem per axis.
    geometry_refs = [
        _data_item_ref(index_dir, di.text) for di in geometry.findall("DataItem")
    ]
    attributes = []
    for attribute in grid.findall("Attribute"):
        center = attribute.attrib.get("Center")
        if center not in {"Node", "Cell"}:
            continue
        file_name, dataset = _data_item_ref(index_dir, attribute.find("DataItem").text)
        attributes.append((attribute.attrib["Name"], center, str(file_name), dataset))
    return {
        "topology": (str(topology_ref[0]), topology_ref[1]),
        "geometry": [(str(f), d) for f, d in geometry_refs],
        "attributes": attributes,
    }


def _scan_xdmf(path: Path) -> tuple[str, list[SnapshotRef]]:
    index_dir = path.parent
    root = ET.parse(path).getroot()
    temporal = root.find(".//Grid[@CollectionType='Temporal']")
    refs: list[SnapshotRef] = []
    if temporal is not None:
        for grid in temporal.findall("./Grid"):
            time_node = grid.find("Time")
            time = None if time_node is None else float(time_node.attrib["Value"])
            refs.append(
                SnapshotRef(
                    "xdmf",
                    str(path),
                    time,
                    path.stem,
                    _grid_to_payload(index_dir, grid),
                )
            )
        return path.stem, refs

    grid = root.find(".//Grid")
    if grid is None:
        raise ValueError(f"No Grid element found in {path}")
    payload = _grid_to_payload(index_dir, grid)
    return path.stem, [
        SnapshotRef("xdmf", str(path), _time_from_name(path), path.stem, payload)
    ]


def _load_xdmf_step(ref: SnapshotRef) -> MeshSnapshot:
    import h5py

    payload = ref.payload
    handles: dict[str, "h5py.File"] = {}

    def read(file_name: str, dataset: str) -> np.ndarray:
        handle = handles.get(file_name)
        if handle is None:
            handle = handles[file_name] = h5py.File(file_name, "r")
        return np.asarray(handle[dataset])

    try:
        geometry = [np.asarray(read(*ref), dtype=float) for ref in payload["geometry"]]
        coords = geometry[0] if len(geometry) == 1 else np.stack(geometry, axis=1)
        points = _drop_trivial_z(coords)
        cells = _decode_xdmf_topology(read(*payload["topology"]))
        node_data: dict[str, np.ndarray] = {}
        cell_data: dict[str, np.ndarray] = {}
        for name, center, file_name, dataset in payload["attributes"]:
            values = _squeeze_field(read(file_name, dataset))
            (node_data if center == "Node" else cell_data)[name] = values
    finally:
        for handle in handles.values():
            handle.close()

    return MeshSnapshot(
        name=ref.name,
        time=ref.time,
        points=points,
        cells=cells,
        node_data=node_data,
        cell_data=cell_data,
        field_order=_ordered_field_names(node_data, cell_data),
        source=Path(ref.path),
    )


def _decode_xdmf_topology(topology: np.ndarray) -> object:
    flat = np.asarray(topology).reshape(-1)
    if flat.size == 0:
        return np.empty((0, 0), dtype=np.int64)

    # Fast path: When every cell is the same fixed-arity element the stream has a
    # constant stride and reshapes in one go, without a Python loop over (up to
    # millions of) cells.
    first = int(flat[0])
    if first in XDMF_NODE_COUNTS:
        stride = XDMF_NODE_COUNTS[first] + 1
        if flat.size % stride == 0:
            block = flat.reshape(-1, stride)
            if np.all(block[:, 0] == first):
                return block[:, 1:].astype(np.int64)

    cells: list[np.ndarray] = []
    index = 0
    size = flat.size
    while index < size:
        cell_type = int(flat[index])
        index += 1
        if cell_type in XDMF_POLY_TYPES:
            width = int(flat[index])
            index += 1
        elif cell_type in XDMF_NODE_COUNTS:
            width = XDMF_NODE_COUNTS[cell_type]
        else:
            raise ValueError(f"Unsupported XDMF topology type {cell_type}")
        cells.append(flat[index : index + width])
        index += width
    return _pack_cells(cells)


def _scan_bare_hdf5(path: Path) -> tuple[str, list[SnapshotRef]]:
    return path.stem, [
        SnapshotRef("h5", str(path), _time_from_name(path), path.stem, {})
    ]


def _load_bare_hdf5(ref: SnapshotRef) -> MeshSnapshot:
    import h5py

    with h5py.File(ref.path, "r") as handle:
        geometry = handle["geometry"]
        if isinstance(geometry, h5py.Group):
            coords = np.stack(
                [np.asarray(geometry[a], dtype=float) for a in ("x", "y", "z")], axis=1
            )
        else:
            coords = np.asarray(geometry, dtype=float)
        points = _drop_trivial_z(coords)
        cells = _decode_xdmf_topology(np.asarray(handle["topology"]))
        node_data = {
            name: _squeeze_field(np.asarray(dataset))
            for name, dataset in handle.get("data", {}).items()
        }
        cell_data = {
            name: _squeeze_field(np.asarray(dataset))
            for name, dataset in handle.get("cell_data", {}).items()
        }
    return MeshSnapshot(
        name=ref.name,
        time=ref.time,
        points=points,
        cells=cells,
        node_data=node_data,
        cell_data=cell_data,
        field_order=_ordered_field_names(node_data, cell_data),
        source=Path(ref.path),
    )


# --------------------------------------------------------------------------- #
# NetCDF UGRID reader (a .nc per step, listed in a .nc.series JSON index)
# --------------------------------------------------------------------------- #
def _scan_netcdf(path: Path) -> tuple[str, list[SnapshotRef]]:
    if str(path).endswith(".nc.series"):
        base = path.name[: -len(".nc.series")]
        directory = path.parent
        document = json.loads(path.read_text())
        refs = []
        for entry in document.get("files", []):
            snapshot = directory / entry["name"]
            refs.append(
                SnapshotRef("netcdf", str(snapshot), float(entry["time"]), base, {})
            )
        return base, refs
    return path.stem, [
        SnapshotRef("netcdf", str(path), _time_from_name(path), path.stem, {})
    ]


def _load_netcdf(ref: SnapshotRef) -> MeshSnapshot:
    from netCDF4 import Dataset

    with Dataset(ref.path) as dataset:
        mesh = _netcdf_mesh_variable(dataset)
        coordinate_names = mesh.node_coordinates.split()
        points = np.stack(
            [
                np.asarray(dataset.variables[name][:], dtype=float)
                for name in coordinate_names
            ],
            axis=1,
        )
        topology_dimension = int(np.asarray(mesh.topology_dimension).reshape(-1)[0])
        connectivity_name = (
            mesh.volume_node_connectivity
            if topology_dimension == 3
            else mesh.face_node_connectivity
        )
        connectivity_variable = dataset.variables[connectivity_name]
        start_index = int(
            np.asarray(getattr(connectivity_variable, "start_index", 0)).reshape(-1)[0]
        )
        fill = getattr(connectivity_variable, "_FillValue", -1)
        raw = np.ma.filled(connectivity_variable[:], fill).astype(np.int64)
        if np.all(raw != fill):  # fixed width with no ragged padding: one array
            cells = raw - start_index
        else:
            cells = _pack_cells([(row[row != fill] - start_index) for row in raw])

        node_dim = dataset.variables[coordinate_names[0]].dimensions[0]
        cell_dim = connectivity_variable.dimensions[0]
        node_data = _netcdf_fields(dataset, node_dim, coordinate_names)
        cell_data = _netcdf_fields(dataset, cell_dim, coordinate_names)
        time = ref.time
        if "time" in dataset.variables:
            time = float(np.asarray(dataset.variables["time"][:]).reshape(-1)[0])

    points = _drop_trivial_z(points)
    return MeshSnapshot(
        name=ref.name,
        time=time,
        points=points,
        cells=cells,
        node_data=node_data,
        cell_data=cell_data,
        field_order=_ordered_field_names(node_data, cell_data),
        source=Path(ref.path),
    )


def _netcdf_mesh_variable(dataset):
    for variable in dataset.variables.values():
        if getattr(variable, "cf_role", None) == "mesh_topology":
            return variable
    raise ValueError("NetCDF file has no mesh_topology variable")


_COMPONENT_SUFFIX = re.compile(r"^(.*)_(\d+)$")
_COMPONENT_LONG_NAME = re.compile(r"^(.*) \(component (\d+)\)$")


def _netcdf_fields(
    dataset, dimension: str, coordinate_names: list[str]
) -> dict[str, np.ndarray]:
    """Group the variables defined over ``dimension`` into scalar and vector fields.

    A field's ``name_0``, ``name_1`` ... components are regrouped into one
    ``(N, components)`` array, guided by each variable's ``long_name``.
    """
    scalars: dict[str, str] = {}
    components: dict[str, dict[int, str]] = {}
    reserved = set(coordinate_names) | {"time"}
    for name, variable in dataset.variables.items():
        if variable.dimensions != (dimension,) or name in reserved:
            continue
        # A variable named after its own dimension is a CF coordinate variable (here the
        # plotter's dimension-scale placeholder), not a data field.
        if name == dimension:
            continue
        if getattr(variable, "cf_role", None) is not None:
            continue
        long_name = getattr(variable, "long_name", None)
        tagged = (
            _COMPONENT_LONG_NAME.match(long_name)
            if isinstance(long_name, str)
            else None
        )
        if tagged:
            components.setdefault(tagged.group(1), {})[int(tagged.group(2))] = name
        elif long_name is not None:
            scalars[name] = name  # writer-declared scalar, kept under its own name
        else:
            # No metadata (a foreign/legacy NetCDF): Fall back to the name-suffix heuristic.
            match = _COMPONENT_SUFFIX.match(name)
            if match and match.group(1):
                components.setdefault(match.group(1), {})[int(match.group(2))] = name
            else:
                scalars[name] = name

    fields: dict[str, np.ndarray] = {}
    for base, name in scalars.items():
        fields[base] = np.asarray(dataset.variables[name][:])
    for base, members in components.items():
        ordered = [
            np.asarray(dataset.variables[members[c]][:]) for c in sorted(members)
        ]
        fields[base] = np.stack(ordered, axis=1)
    return fields


# --------------------------------------------------------------------------- #
# VTU reader (a .pvd collection of per-step .pvtu, each a set of per-rank .vtu)
# --------------------------------------------------------------------------- #
def _local_tag(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def _scan_vtu(path: Path) -> tuple[str, list[SnapshotRef]]:
    suffix = path.suffix.lower()
    if suffix == ".pvd":
        directory = path.parent
        root = ET.parse(path).getroot()
        refs = []
        for element in root.iter():
            if _local_tag(element.tag) != "DataSet":
                continue
            snapshot = directory / element.attrib["file"]
            time = float(element.attrib.get("timestep", "nan"))
            refs.append(SnapshotRef("vtu", str(snapshot), time, path.stem, {}))
        refs.sort(
            key=lambda ref: (
                math.isnan(ref.time if ref.time is not None else float("nan")),
                ref.time or 0.0,
            )
        )
        return path.stem, refs
    return path.stem, [
        SnapshotRef("vtu", str(path), _time_from_name(path), path.stem, {})
    ]


def _load_vtu(ref: SnapshotRef) -> MeshSnapshot:
    # VTK reads appended data and multi-piece grids; meshio supplies the fallback.
    path = Path(ref.path)
    try:
        return _load_vtu_vtk(ref, path)
    except ImportError:
        return _load_vtu_meshio(ref, path)


def _load_vtu_vtk(ref: SnapshotRef, path: Path) -> MeshSnapshot:
    import vtk
    from vtk.util.numpy_support import vtk_to_numpy

    if path.suffix.lower() == ".pvtu":
        reader = vtk.vtkXMLPUnstructuredGridReader()
    else:
        reader = vtk.vtkXMLUnstructuredGridReader()
    reader.SetFileName(str(path))
    reader.Update()
    grid = reader.GetOutput()

    points = _drop_trivial_z(
        np.empty((0, 3))
        if grid.GetNumberOfPoints() == 0
        else np.asarray(vtk_to_numpy(grid.GetPoints().GetData()), dtype=float)
    )
    cell_array = grid.GetCells()
    connectivity = np.asarray(
        vtk_to_numpy(cell_array.GetConnectivityArray()), dtype=np.int64
    )
    offsets = np.asarray(vtk_to_numpy(cell_array.GetOffsetsArray()), dtype=np.int64)
    cells = _cells_from_offsets(connectivity, offsets, grid.GetNumberOfCells())

    node_data = _vtk_field_arrays(grid.GetPointData(), vtk_to_numpy)
    cell_data = _vtk_field_arrays(grid.GetCellData(), vtk_to_numpy)
    return MeshSnapshot(
        name=ref.name,
        time=ref.time,
        points=points,
        cells=cells,
        node_data=node_data,
        cell_data=cell_data,
        field_order=_ordered_field_names(node_data, cell_data),
        source=path,
    )


def _vtk_field_arrays(field_data, vtk_to_numpy) -> dict[str, np.ndarray]:
    arrays = {}
    for index in range(field_data.GetNumberOfArrays()):
        arrays[field_data.GetArrayName(index)] = _squeeze_field(
            np.asarray(vtk_to_numpy(field_data.GetArray(index)))
        )
    return arrays


def _load_vtu_meshio(ref: SnapshotRef, path: Path) -> MeshSnapshot:
    piece_files = _pvtu_pieces(path) if path.suffix.lower() == ".pvtu" else [path]

    point_blocks: list[np.ndarray] = []
    cell_blocks: list[np.ndarray] = []
    node_acc: dict[str, list[np.ndarray]] = {}
    cell_acc: dict[str, list[np.ndarray]] = {}
    point_offset = 0
    for piece in piece_files:
        with piece.open("rb") as source:
            header = source.read(65536)
        piece_tags = re.findall(rb"<Piece\b[^>]*>", header)
        if (
            b"</VTKFile>" in header
            and piece_tags
            and all(
                dict(
                    re.findall(
                        rb"\b(NumberOfPoints|NumberOfCells)\s*=\s*[\"'](\d+)[\"']", tag
                    )
                )
                == {b"NumberOfPoints": b"0", b"NumberOfCells": b"0"}
                for tag in piece_tags
            )
        ):
            continue
        if len(piece_tags) > 1:
            import warnings

            warnings.warn(
                f"{piece.name} holds several <Piece> elements; meshio reads only one. "
                "Install vtk for complete multi-piece VTU reading.",
                stacklevel=2,
            )
        points, blocks, node_data, cell_data = _read_vtu_piece(piece)
        point_blocks.append(points)
        for block in blocks:
            cell_blocks.append(block + point_offset)
        for name, values in node_data.items():
            node_acc.setdefault(name, []).append(values)
        for name, values in cell_data.items():
            cell_acc.setdefault(name, []).append(values)
        point_offset += len(points)

    points = np.concatenate(point_blocks) if point_blocks else np.empty((0, 3))
    node_data = {name: np.concatenate(values) for name, values in node_acc.items()}
    cell_data = {name: np.concatenate(values) for name, values in cell_acc.items()}
    return MeshSnapshot(
        name=ref.name,
        time=ref.time,
        points=_drop_trivial_z(np.asarray(points, dtype=float)),
        cells=_pack_cell_blocks(cell_blocks),
        node_data={name: _squeeze_field(values) for name, values in node_data.items()},
        cell_data={name: _squeeze_field(values) for name, values in cell_data.items()},
        field_order=_ordered_field_names(node_data, cell_data),
        source=path,
    )


def _pack_cell_blocks(blocks: list[np.ndarray]) -> object:
    """Concatenate same-arity cell blocks into one array, else keep a per-cell list."""
    blocks = [block for block in blocks if block.size]
    if not blocks:
        return np.empty((0, 0), dtype=np.int64)
    if len({block.shape[1] for block in blocks}) == 1:
        return np.concatenate(blocks, axis=0).astype(np.int64)
    return [row.astype(np.int64) for block in blocks for row in block]


def _pvtu_pieces(path: Path) -> list[Path]:
    directory = path.parent
    root = ET.parse(path).getroot()
    pieces = []
    for element in root.iter():
        if _local_tag(element.tag) == "Piece" and "Source" in element.attrib:
            pieces.append(directory / element.attrib["Source"])
    return pieces


def _read_vtu_piece(path: Path):
    import meshio

    mesh = meshio.read(path)
    cell_blocks = [np.asarray(block.data, dtype=np.int64) for block in mesh.cells]
    node_data = {name: np.asarray(values) for name, values in mesh.point_data.items()}
    cell_data = {
        name: np.concatenate([np.asarray(chunk) for chunk in chunks])
        for name, chunks in mesh.cell_data.items()
    }
    return np.asarray(mesh.points, dtype=float), cell_blocks, node_data, cell_data


# --------------------------------------------------------------------------- #
# Format dispatch
# --------------------------------------------------------------------------- #
def _drop_trivial_z(points: np.ndarray) -> np.ndarray:
    """Drop a trailing column of zeros so a planar mesh is treated as 2D.

    Geometry is always stored with three columns.
    """
    if points.ndim == 2 and points.shape[1] == 3 and np.all(points[:, 2] == 0.0):
        return points[:, :2]
    return points


def load_snapshot_ref(ref: SnapshotRef) -> MeshSnapshot:
    loaders = {
        "vtkhdf": _load_vtkhdf_step,
        "xdmf": _load_xdmf_step,
        "h5": _load_bare_hdf5,
        "netcdf": _load_netcdf,
        "vtu": _load_vtu,
    }
    return loaders[ref.fmt](ref)


def _scan_path(path: Path) -> tuple[str, list[SnapshotRef]]:
    if str(path).endswith(".nc.series"):
        return _scan_netcdf(path)
    suffix = path.suffix.lower()
    if suffix == ".vtkhdf":
        return _scan_vtkhdf(path)
    if suffix in {".xmf", ".xdmf"}:
        return _scan_xdmf(path)
    if suffix == ".h5":
        return _scan_bare_hdf5(path)
    if suffix in {".nc", ".nc4"}:
        return _scan_netcdf(path)
    if suffix in {".pvd", ".pvtu", ".vtu"}:
        return _scan_vtu(path)
    raise ValueError(f"Unsupported input type: {path}")


# An index file pattern per format, tried in this order during discovery.
INDEX_PATTERNS = ("{name}.vtkhdf", "{name}.xmf", "{name}.nc.series", "{name}.pvd")
SNAPSHOT_PATTERNS = ("{name}-*.vtkhdf", "{name}-*.xmf", "{name}-*.nc", "{name}-*.h5")
OUTPUT_SUBFOLDERS = ("", "patch", "face", "particle")


class SolutionSource:
    """A time-ordered series of snapshots, lazily loaded one frame at a time."""

    def __init__(
        self, name: str, refs: list[SnapshotRef], path: str | Path | None = None
    ):
        if not refs:
            raise ValueError(f"No snapshots found for {name!r}")
        self.name = name
        self.refs = refs
        self.path = (
            path  # the file/index/directory this source was opened from (for messages)
        )
        self._cache_index: int | None = None
        self._cache_snapshot: MeshSnapshot | None = None
        self._variables: list[str] | None = None

    @property
    def count(self) -> int:
        return len(self.refs)

    @property
    def times(self) -> list[float | None]:
        return [ref.time for ref in self.refs]

    def load(self, index: int) -> MeshSnapshot:
        return load_snapshot_ref(self.refs[index])

    def snapshot(self, index: int) -> MeshSnapshot:
        if self._cache_index != index:
            self._cache_snapshot = self.load(index)
            self._cache_index = index
        return self._cache_snapshot

    @property
    def variables(self) -> list[str]:
        if self._variables is None:
            self._variables = self.snapshot(0).variables
        return self._variables

    @property
    def format(self) -> str:
        names = {
            "vtkhdf": "VTKHDF",
            "xdmf": "HDF5/XDMF",
            "h5": "HDF5",
            "netcdf": "NetCDF",
            "vtu": "VTU",
        }
        return names.get(self.refs[0].fmt, self.refs[0].fmt) if self.refs else "unknown"

    def describe(self) -> str:
        """A one-line summary for messages: source, format, snapshot count and time span."""
        where = (
            self.path
            if self.path is not None
            else (self.refs[0].path if self.refs else "?")
        )
        finite = [
            t
            for t in self.times
            if t is not None and not (isinstance(t, float) and math.isnan(t))
        ]
        span = f", t = {min(finite):g} … {max(finite):g}" if finite else ""
        return f"{where} ({self.format}, {self.count} snapshot{'' if self.count == 1 else 's'}{span})"

    @classmethod
    def open(cls, path: str | Path) -> "SolutionSource":
        path = Path(path)
        if path.is_dir():
            return cls.open(_index_in_directory(path))
        if not path.exists():
            raise FileNotFoundError(f"No such file: {path}")
        name, refs = _scan_path(path)
        refs = [ref for ref in refs if Path(ref.path).exists()]
        return cls(name, refs, path)

    @classmethod
    def discover(
        cls, name: str, directory: str | Path | None = None
    ) -> "SolutionSource":
        index = _discover_index(name, directory)
        if index is not None:
            return cls.open(index)
        snapshots = _discover_snapshots(name, directory)
        if snapshots:
            refs: list[SnapshotRef] = []
            for snapshot in snapshots:
                refs.extend(_scan_path(snapshot)[1])
            refs.sort(
                key=lambda ref: (
                    math.isnan(ref.time if ref.time is not None else float("nan")),
                    ref.time or 0.0,
                )
            )
            return cls(name, refs, snapshots[0])
        searched = ", ".join(
            str(directory) for directory in _candidate_directories(directory)
        )
        raise FileNotFoundError(
            f"No plotter output found for {name!r}. Looked in: {searched}. "
            "Run the cells that build and execute the solver first."
        )

    @classmethod
    def locate(cls, spec, directory: str | Path | None = None) -> "SolutionSource":
        """A source for ``spec``, opened from a path or discovered by solver name.

        An existing file or directory path is opened, anything else is taken to be
        a solver name and discovered. A SolutionSource is returned unchanged, which
        also tolerates the sibling-module class duplication (see
        render_solution.render_solution_image).
        """
        if not isinstance(spec, (str, os.PathLike)):
            return spec
        path = Path(spec)
        if path.exists():
            return cls.open(path)
        if path.is_absolute():
            raise FileNotFoundError(f"No such file or directory: {path}")
        return cls.discover(str(spec), directory)


def _index_in_directory(directory: Path) -> Path:
    """The single discoverable index in a directory, preferring a sub-folder."""
    candidates: list[Path] = []
    for subfolder in OUTPUT_SUBFOLDERS:
        base = directory / subfolder if subfolder else directory
        if not base.is_dir():
            continue
        for pattern in ("*.vtkhdf", "*.nc.series", "*.pvd"):
            candidates.extend(sorted(base.glob(pattern)))
        candidates.extend(sorted(_temporal_xmf_files(base)))
    if not candidates:
        raise FileNotFoundError(f"No plotter index found under {directory}")
    if len({path.name for path in candidates}) > 1:
        listing = ", ".join(str(path) for path in candidates)
        raise ValueError(f"Several solver outputs found; point at one of: {listing}")
    return candidates[0]


def _temporal_xmf_files(directory: Path) -> list[Path]:
    indices = []
    for path in directory.glob("*.xmf"):
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError:
            continue
        if root.find(".//Grid[@CollectionType='Temporal']") is not None:
            indices.append(path)
    return indices


def _candidate_directories(directory: str | Path | None) -> list[Path]:
    if directory is not None:
        roots = [Path(directory)]
    else:
        cwd = Path(".")
        roots = [cwd, Path("output")] + sorted(cwd.glob("*.output"))
        # The output directory can be named anything (e.g. solutions/ or <project>.output),
        # so also look one level down: Every immediate sub-directory and its patch/face/particle.
        roots += [
            child
            for child in sorted(cwd.iterdir())
            if child.is_dir() and not child.name.startswith(".")
        ]
    directories: list[Path] = []
    for root in roots:
        for subfolder in OUTPUT_SUBFOLDERS:
            base = root / subfolder if subfolder else root
            if base.is_dir() and base not in directories:
                directories.append(base)
    return directories


def _discover_index(name: str, directory: str | Path | None) -> Path | None:
    for base in _candidate_directories(directory):
        for pattern in INDEX_PATTERNS:
            candidate = base / pattern.format(name=name)
            if candidate.exists():
                return candidate
    return None


def _discover_snapshots(name: str, directory: str | Path | None) -> list[Path]:
    for base in _candidate_directories(directory):
        bare = base / f"{name}.h5"
        if bare.exists():
            return [bare]
        for pattern in SNAPSHOT_PATTERNS:
            matches = sorted(base.glob(pattern.format(name=name)), key=_time_sort_key)
            if matches:
                return matches
    return []
