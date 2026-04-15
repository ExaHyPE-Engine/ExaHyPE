#!/usr/bin/env python3

from __future__ import annotations

import argparse
from concurrent.futures import Executor, ProcessPoolExecutor, ThreadPoolExecutor
from dataclasses import dataclass
import math
import multiprocessing
import os
from pathlib import Path
import xml.etree.ElementTree as ET

import numpy as np


NETCDF_SUFFIXES = {".nc", ".nc4", ".netcdf"}
AXIS_TO_INDEX = {"x": 0, "y": 1, "z": 2}
FORMAT_CHOICES = ("png", "pdf", "svg", "pgf")


@dataclass
class MeshSnapshot:
    name: str
    time: float | None
    points: np.ndarray
    cells: np.ndarray
    cell_data: dict[str, np.ndarray]
    node_data: dict[str, np.ndarray]
    field_order: list[tuple[str, str]]
    source: Path


@dataclass
class SelectedField:
    values: np.ndarray
    label: str
    token: str
    center: str


def build_parser() -> argparse.ArgumentParser:
    default_jobs = max(1, os.cpu_count() or 1)
    parser = argparse.ArgumentParser(
        description="Render solution data from HDF5/XMF, VTU, or NetCDF inputs"
    )
    parser.add_argument("input", help="Input file or directory")
    parser.add_argument("--output-dir", default=".", help="Output directory")
    parser.add_argument(
        "--format",
        choices=FORMAT_CHOICES,
        default="png",
        help="Image format for generated renderings",
    )
    parser.add_argument("--field", help="Field or variable name to render")
    parser.add_argument(
        "--component",
        type=int,
        help="Component of a vector-valued field to render",
    )
    parser.add_argument(
        "--dpi",
        type=int,
        default=600,
        help="DPI for raster outputs",
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        nargs="?",
        const=default_jobs,
        default=1,
        help=(
            "Number of worker processes for multi-snapshot inputs. "
            "Pass -j/--jobs without a value to use all available cores"
        ),
    )
    parser.add_argument(
        "--slice-axis",
        choices=tuple(AXIS_TO_INDEX.keys()),
        help="Slice a 3D mesh along the chosen axis",
    )
    parser.add_argument(
        "--slice-position",
        type=float,
        help="Position of the 3D slice along the slice axis",
    )
    parser.add_argument(
        "--slice-width",
        type=float,
        default=0.0,
        help="Slice band width along the slice axis",
    )
    parser.add_argument("--colormap", default="viridis", help="Matplotlib colormap")
    parser.add_argument("--vmin", type=float, help="Lower color limit")
    parser.add_argument("--vmax", type=float, help="Upper color limit")
    parser.add_argument(
        "--animation",
        choices=("gif", "mp4"),
        help="Assemble generated PNG frames into an animation",
    )
    parser.add_argument("--variable", help="NetCDF variable name to render")
    parser.add_argument(
        "--time-index",
        type=int,
        default=0,
        help="Time index used when the variable has more than 2 dimensions",
    )
    parser.add_argument(
        "--level-index",
        type=int,
        help="Explicit level index for 3D variables before plotting a 2D slice",
    )
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    input_path = Path(args.input)
    kind = detect_input_kind(input_path)

    if kind == "hdf5":
        if args.variable and not args.field:
            args.field = args.variable
        run_hdf5(args)
        return

    if kind == "vtu":
        if args.variable and not args.field:
            args.field = args.variable
        run_vtu(args)
        return

    if kind == "netcdf":
        if args.field and not args.variable:
            args.variable = args.field
        run_netcdf(args)
        return

    raise ValueError(f"Unsupported input type: {input_path}")


def detect_input_kind(path: Path) -> str:
    if path.is_dir():
        if any(path.glob("*.vtu")):
            return "vtu"
        raise ValueError(f"Cannot detect a supported input type in directory {path}")

    suffix = path.suffix.lower()
    if suffix in {".xmf", ".h5"}:
        return "hdf5"
    if suffix == ".vtu":
        return "vtu"
    if suffix in NETCDF_SUFFIXES:
        return "netcdf"
    raise ValueError(f"Unsupported input type: {path}")


def run_hdf5(args: argparse.Namespace) -> None:
    snapshots = load_hdf5_snapshots(Path(args.input), args)
    output_dir = ensure_output_dir(args.output_dir)
    stem = Path(args.input).stem

    frame_paths = render_selected_mesh_snapshots(
        snapshots, list(range(len(snapshots))), stem, output_dir, args
    )
    write_animation_if_requested(frame_paths, output_dir, stem, args)


def load_hdf5_snapshots(path: Path, args: argparse.Namespace) -> list[MeshSnapshot]:
    if path.suffix.lower() == ".h5":
        return [load_bare_hdf5(path, args)]
    if path.suffix.lower() != ".xmf":
        raise ValueError(f"Unsupported input type: {path}")

    root = ET.parse(path).getroot()
    temporal_grids = root.findall(".//Grid[@CollectionType='Temporal']")
    if temporal_grids:
        snapshots: list[MeshSnapshot] = []
        for grid in temporal_grids[0].findall("./Grid"):
            time_value = grid.find("Time")
            include = grid.find("{http://www.w3.org/2001/XInclude}include")
            if include is None:
                raise ValueError(f"Temporal grid entry in {path} is missing xi:include")
            step_path = (path.parent / include.attrib["href"]).resolve()
            snapshots.append(
                load_xmf_step(
                    step_path,
                    None if time_value is None else time_value.attrib.get("Value"),
                )
            )
        return snapshots

    return [load_xmf_step(path, None)]


def load_xmf_step(path: Path, time_value: str | None) -> MeshSnapshot:
    root = ET.parse(path).getroot()
    grid = root.find(".//Grid")
    if grid is None:
        raise ValueError(f"No Grid element found in {path}")

    topology = grid.find("Topology")
    geometry = grid.find("Geometry")
    if topology is None or geometry is None:
        raise ValueError(f"Grid in {path} does not contain topology and geometry")

    topology_data = read_data_item(path, topology.find("DataItem"))
    geometry_data = read_data_item(path, geometry.find("DataItem"))
    cells = decode_topology(topology_data, topology.attrib.get("TopologyType", "Mixed"))

    cell_data: dict[str, np.ndarray] = {}
    node_data: dict[str, np.ndarray] = {}
    field_order: list[tuple[str, str]] = []
    for attribute in grid.findall("Attribute"):
        center = attribute.attrib.get("Center")
        if center not in {"Cell", "Node"}:
            continue
        data_item = attribute.find("DataItem")
        if center == "Cell":
            cell_data[attribute.attrib["Name"]] = read_data_item(path, data_item)
            field_order.append((attribute.attrib["Name"], "cell"))
        else:
            node_data[attribute.attrib["Name"]] = read_data_item(path, data_item)
            field_order.append((attribute.attrib["Name"], "node"))

    return MeshSnapshot(
        name=path.stem,
        time=None if time_value is None else float(time_value),
        points=np.asarray(geometry_data, dtype=float),
        cells=cells,
        cell_data=cell_data,
        node_data=node_data,
        field_order=field_order,
        source=path,
    )


def load_bare_hdf5(path: Path, args: argparse.Namespace) -> MeshSnapshot:
    import h5py

    with h5py.File(path, "r") as handle:
        points = np.asarray(handle["geometry"], dtype=float)
        cells = decode_topology(np.asarray(handle["topology"]), "Mixed")
        cell_data = {
            name: np.asarray(dataset)
            for name, dataset in handle.get("cell_data", {}).items()
            if hasattr(dataset, "shape")
        }
        node_data = {
            name: np.asarray(dataset)
            for name, dataset in handle.get("data", {}).items()
            if hasattr(dataset, "shape")
        }
        field_order = [(name, "cell") for name in cell_data] + [
            (name, "node") for name in node_data
        ]
        if not cell_data and not node_data and args.field:
            array = np.asarray(handle[args.field])
            if len(array) == len(points):
                node_data[args.field] = array
                field_order.append((args.field, "node"))
            else:
                cell_data[args.field] = array
                field_order.append((args.field, "cell"))

    return MeshSnapshot(
        name=path.stem,
        time=None,
        points=points,
        cells=cells,
        cell_data=cell_data,
        node_data=node_data,
        field_order=field_order,
        source=path,
    )


def read_data_item(xmf_path: Path, data_item: ET.Element | None) -> np.ndarray:
    import h5py

    if data_item is None or data_item.text is None:
        raise ValueError(f"Missing DataItem in {xmf_path}")

    location = data_item.text.strip()
    if data_item.attrib.get("Format", "").upper() == "XML":
        return np.fromstring(location, sep=" ")
    if ":/" not in location:
        raise ValueError(f"Unsupported DataItem location '{location}' in {xmf_path}")

    file_name, dataset_name = location.split(":/", 1)
    hdf5_path = (xmf_path.parent / file_name).resolve()
    with h5py.File(hdf5_path, "r") as handle:
        return np.asarray(handle[dataset_name])


def decode_topology(topology: np.ndarray, topology_type: str) -> np.ndarray:
    if topology_type != "Mixed":
        if topology.ndim == 2:
            return np.asarray(topology, dtype=int)
        raise ValueError(f"Unsupported non-mixed topology '{topology_type}'")

    flat = np.asarray(topology).reshape(-1)
    if flat.size == 0:
        raise ValueError("Empty topology array")

    node_counts = {5: 4, 9: 8}
    cells = []
    index = 0
    while index < flat.size:
        cell_type = int(flat[index])
        if cell_type not in node_counts:
            raise ValueError(f"Unsupported mixed topology cell type {cell_type}")
        width = node_counts[cell_type]
        start = index + 1
        end = start + width
        cells.append(flat[start:end])
        index = end
    return np.asarray(cells, dtype=int)


def run_vtu(args: argparse.Namespace) -> None:
    snapshots = load_vtu_snapshots(Path(args.input))
    output_dir = ensure_output_dir(args.output_dir)
    stem = Path(args.input).stem

    frame_paths = render_selected_mesh_snapshots(
        snapshots, list(range(len(snapshots))), stem, output_dir, args
    )
    write_animation_if_requested(frame_paths, output_dir, stem, args)


def load_vtu_snapshots(path: Path) -> list[MeshSnapshot]:
    if path.is_dir():
        files = sorted(path.glob("*.vtu"))
        if not files:
            raise ValueError(f"No VTU files found in {path}")
    else:
        files = [path]
    return [load_single_vtu(file_path) for file_path in files]


def load_single_vtu(path: Path) -> MeshSnapshot:
    import meshio

    mesh = meshio.read(path)
    if not mesh.cells:
        raise ValueError(f"No cells found in {path}")

    preferred_types = ("quad", "hexahedron")
    selected_type = None
    for cell_type in preferred_types:
        if any(block.type == cell_type for block in mesh.cells):
            selected_type = cell_type
            break
    if selected_type is None:
        supported = ", ".join(sorted({block.type for block in mesh.cells}))
        raise ValueError(f"Unsupported VTU cell types in {path}: {supported}")

    cells = np.concatenate(
        [block.data for block in mesh.cells if block.type == selected_type],
        axis=0,
    )
    cell_data = {
        name: np.concatenate(
            [
                np.asarray(chunk)
                for block, chunk in zip(mesh.cells, chunks)
                if block.type == selected_type
            ],
            axis=0,
        )
        for name, chunks in mesh.cell_data.items()
    }
    node_data = {name: np.asarray(values) for name, values in mesh.point_data.items()}
    field_order = [(name, "cell") for name in cell_data] + [
        (name, "node") for name in node_data
    ]

    return MeshSnapshot(
        name=path.stem,
        time=None,
        points=np.asarray(mesh.points, dtype=float),
        cells=np.asarray(cells, dtype=int),
        cell_data=cell_data,
        node_data=node_data,
        field_order=field_order,
        source=path,
    )


def run_netcdf(args: argparse.Namespace) -> None:
    output_dir = ensure_output_dir(args.output_dir)
    variable_name, array = load_variable(Path(args.input), args)
    image = prepare_image(array, args)
    output_path = output_dir / f"{Path(args.input).stem}-{variable_name}.{args.format}"
    print(
        f"Render snapshot 1: {Path(args.input).stem} with shape {tuple(image.shape)} "
        f'and variable "{variable_name}"'
    )

    render_scalar_image(
        data=image,
        output_path=output_path,
        colormap=args.colormap,
        dpi=args.dpi,
        vmin=args.vmin,
        vmax=args.vmax,
        title=format_default_title(variable_name, None),
    )


def load_variable(path: Path, args: argparse.Namespace) -> tuple[str, np.ndarray]:
    from netCDF4 import Dataset

    with Dataset(path) as dataset:
        variable_name = args.variable or pick_variable(dataset)
        if variable_name not in dataset.variables:
            available = ", ".join(sorted(dataset.variables))
            raise ValueError(
                f"Unknown variable '{variable_name}'. Available variables: {available}"
            )
        array = np.asarray(dataset.variables[variable_name][:])
    return variable_name, array


def pick_variable(dataset) -> str:
    for name, variable in dataset.variables.items():
        if np.issubdtype(variable.dtype, np.number) and len(variable.dimensions) >= 2:
            return name
    raise ValueError("No numeric variable with at least two dimensions found")


def prepare_image(array: np.ndarray, args: argparse.Namespace) -> np.ndarray:
    data = np.asarray(array).squeeze()
    while data.ndim > 3:
        data = data[args.time_index]
    if data.ndim == 3:
        if args.level_index is not None:
            data = data[args.level_index]
        else:
            data = data[data.shape[0] // 2]
    if data.ndim != 2:
        raise ValueError(f"Expected a 2D image after slicing, got shape {data.shape}")
    return data


def render_selected_mesh_snapshots(
    snapshots: list[MeshSnapshot],
    indices: list[int],
    stem: str,
    output_dir: Path,
    args: argparse.Namespace,
) -> list[Path]:
    if args.jobs <= 1 or len(indices) <= 1:
        return [
            render_loaded_mesh_snapshot(
                snapshot=snapshots[index],
                output_dir=output_dir,
                stem=stem,
                index=index,
                args=args,
            )
            for index in indices
        ]

    work_items = []
    for index in indices:
        work_items.append(
            {
                "snapshot": snapshots[index],
                "output_dir": str(output_dir),
                "stem": stem,
                "index": index,
                "args": mesh_render_args(args),
            }
        )

    with create_parallel_executor(args.jobs) as executor:
        return list(executor.map(render_loaded_mesh_snapshot_worker, work_items))


def render_loaded_mesh_snapshot(
    snapshot: MeshSnapshot,
    output_dir: Path,
    stem: str,
    index: int,
    args: argparse.Namespace,
) -> Path:
    selected_field = select_field(
        snapshot.cell_data,
        snapshot.node_data,
        snapshot.field_order,
        args.field,
        args.component,
    )
    total_unknowns = count_field_components(
        snapshot.cell_data
    ) + count_field_components(snapshot.node_data)
    output_path = (
        output_dir / f"{stem}-{index:04d}-{selected_field.token}.{args.format}"
    )
    print(
        f"Render snapshot {index + 1}: {snapshot.name} with "
        f"{snapshot.cells.shape[0]} cells, {total_unknowns} unknowns, "
        f'and field "{selected_field.label}"',
        flush=True,
    )
    render_mesh_snapshot(
        snapshot=snapshot,
        values=selected_field.values,
        field_name=selected_field.label,
        output_path=output_path,
        colormap=args.colormap,
        dpi=args.dpi,
        vmin=args.vmin,
        vmax=args.vmax,
        field_center=selected_field.center,
        show_mesh=True,
        slice_axis=args.slice_axis,
        slice_position=args.slice_position,
        slice_width=args.slice_width,
    )
    return output_path


def render_loaded_mesh_snapshot_worker(work_item: dict) -> Path:
    snapshot = work_item["snapshot"]
    output_dir = Path(work_item["output_dir"])
    stem = work_item["stem"]
    index = work_item["index"]
    args = argparse.Namespace(**work_item["args"])
    return render_loaded_mesh_snapshot(snapshot, output_dir, stem, index, args)


def mesh_render_args(args: argparse.Namespace) -> dict:
    return {
        "field": args.field,
        "component": args.component,
        "format": args.format,
        "colormap": args.colormap,
        "dpi": args.dpi,
        "vmin": args.vmin,
        "vmax": args.vmax,
        "slice_axis": args.slice_axis,
        "slice_position": args.slice_position,
        "slice_width": args.slice_width,
    }


def write_animation_if_requested(
    frame_paths: list[Path],
    output_dir: Path,
    stem: str,
    args: argparse.Namespace,
) -> None:
    if not args.animation:
        return
    if args.format != "png":
        raise ValueError("--animation requires --format png")
    field_name = extract_frame_field_name(frame_paths[0], stem)
    animation = save_animation(
        frame_paths, output_dir, f"{stem}-{field_name}", args.animation
    )


def ensure_output_dir(path: str | Path) -> Path:
    output = Path(path)
    output.mkdir(parents=True, exist_ok=True)
    return output


def extract_frame_field_name(frame_path: Path, stem: str) -> str:
    prefix = f"{stem}-"
    stem_text = frame_path.stem
    if not stem_text.startswith(prefix):
        return stem_text
    remainder = stem_text[len(prefix) :]
    parts = remainder.split("-", maxsplit=1)
    if len(parts) != 2:
        return remainder
    return parts[1]


def select_field(
    cell_data: dict[str, np.ndarray],
    node_data: dict[str, np.ndarray],
    field_order: list[tuple[str, str]],
    requested_field: str | None,
    component: int | None,
) -> SelectedField:
    if not cell_data and not node_data:
        raise ValueError("No mesh data is available to render")
    if requested_field is None:
        requested_field = choose_default_field(field_order)

    if requested_field in cell_data:
        values = np.asarray(cell_data[requested_field])
        center = "cell"
    elif requested_field in node_data:
        values = np.asarray(node_data[requested_field])
        center = "node"
    else:
        available = ", ".join(sorted(set(cell_data) | set(node_data)))
        raise ValueError(
            f"Unknown field '{requested_field}'. Available fields: {available}"
        )
    if values.ndim == 1:
        return SelectedField(
            values=values,
            label=requested_field,
            token=requested_field,
            center=center,
        )
    if values.ndim == 2:
        if values.shape[1] == 1:
            return SelectedField(
                values=values[:, 0],
                label=requested_field,
                token=requested_field,
                center=center,
            )
        if component is None:
            component = 0
        if component < 0 or component >= values.shape[1]:
            raise ValueError(
                f"Component {component} is out of range for field '{requested_field}'"
            )
        return SelectedField(
            values=values[:, component],
            label=format_component_label(requested_field, component),
            token=format_component_token(requested_field, component),
            center=center,
        )
    raise ValueError(f"Unsupported field rank {values.ndim} for '{requested_field}'")


def choose_default_field(field_order: list[tuple[str, str]]) -> str:
    if not field_order:
        raise ValueError("No mesh data is available to render")
    return field_order[0][0]


def format_component_label(field_name: str, component: int) -> str:
    return f"{field_name} Component{component}"


def format_component_token(field_name: str, component: int) -> str:
    return f"{field_name}Component{component}"


def save_animation(
    frame_paths: list[Path], output_dir: Path, stem: str, kind: str, fps: int = 8
) -> Path:
    import imageio.v2 as imageio

    frames = [imageio.imread(path) for path in frame_paths]
    if not frames:
        raise ValueError("No frames available for animation assembly")
    target = output_dir / f"{stem}.{kind}"
    if kind == "gif":
        imageio.mimsave(target, frames, fps=fps)
    else:
        imageio.mimsave(target, frames, fps=fps, codec="libx264")
    return target


def render_scalar_image(
    data: np.ndarray,
    output_path: Path,
    colormap: str,
    dpi: int,
    vmin: float | None,
    vmax: float | None,
    title: str | None = None,
    x_label: str = "x",
    y_label: str = "y",
) -> Path:
    plt, _, _, _ = get_matplotlib_modules()
    fig, ax = plt.subplots(figsize=(8, 6))
    image = ax.imshow(
        data,
        origin="lower",
        cmap=colormap,
        aspect="auto",
        interpolation="nearest",
        vmin=vmin,
        vmax=vmax,
    )
    if title:
        ax.set_title(title)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    fig.colorbar(image, ax=ax)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)
    return output_path


def render_mesh_snapshot(
    snapshot: MeshSnapshot,
    values: np.ndarray,
    field_name: str,
    output_path: Path,
    colormap: str,
    dpi: int,
    vmin: float | None,
    vmax: float | None,
    field_center: str,
    show_mesh: bool,
    slice_axis: str | None,
    slice_position: float | None,
    slice_width: float,
) -> Path:
    if snapshot.points.shape[1] < 2:
        raise ValueError("At least two point dimensions are required")

    drop_axis: int | None = None
    if slice_axis is None:
        planar = infer_planar_axes(snapshot.points)
        if planar is None:
            drop_axis = snapshot.points.shape[1] - 1
            keep_axes = tuple(
                axis for axis in range(snapshot.points.shape[1]) if axis != drop_axis
            )
        else:
            _, keep_axes = planar
    else:
        drop_axis = AXIS_TO_INDEX[slice_axis]
        keep_axes = tuple(
            axis for axis in range(snapshot.points.shape[1]) if axis != drop_axis
        )

    if field_center == "node":
        return render_nodal_snapshot(
            snapshot=snapshot,
            values=values,
            field_name=field_name,
            keep_axes=keep_axes,
            drop_axis=drop_axis,
            slice_position=slice_position,
            slice_width=slice_width,
            output_path=output_path,
            colormap=colormap,
            dpi=dpi,
            vmin=vmin,
            vmax=vmax,
            show_mesh=show_mesh,
        )

    return render_polygon_snapshot(
        snapshot=snapshot,
        values=values,
        field_name=field_name,
        keep_axes=keep_axes,
        drop_axis=drop_axis,
        slice_position=slice_position,
        slice_width=slice_width,
        output_path=output_path,
        colormap=colormap,
        dpi=dpi,
        vmin=vmin,
        vmax=vmax,
        show_mesh=show_mesh,
    )


def axis_label(axis_index: int) -> str:
    return ("x", "y", "z")[axis_index]


def infer_planar_axes(points: np.ndarray) -> tuple[int, tuple[int, int]] | None:
    spans = np.ptp(points, axis=0)
    min_axis = int(np.argmin(spans))
    max_span = float(np.max(spans))
    if math.isclose(max_span, 0.0):
        raise ValueError("Point cloud has zero extent in every dimension")
    if spans[min_axis] <= max(1.0e-12, 0.05 * max_span):
        keep_axes = tuple(axis for axis in range(points.shape[1]) if axis != min_axis)
        return min_axis, keep_axes
    return None


def render_polygon_snapshot(
    snapshot: MeshSnapshot,
    values: np.ndarray,
    field_name: str,
    keep_axes: tuple[int, int],
    drop_axis: int | None,
    slice_position: float | None,
    slice_width: float,
    output_path: Path,
    colormap: str,
    dpi: int,
    vmin: float | None,
    vmax: float | None,
    show_mesh: bool,
) -> Path:
    cell_points = snapshot.points[snapshot.cells]
    if drop_axis is not None:
        mins = np.min(cell_points[:, :, drop_axis], axis=1)
        maxs = np.max(cell_points[:, :, drop_axis], axis=1)
        if slice_position is None:
            slice_position = float(0.5 * (np.min(mins) + np.max(maxs)))
        band_low = slice_position - slice_width / 2.0
        band_high = slice_position + slice_width / 2.0
        mask = (mins <= band_high) & (maxs >= band_low)
        if not np.any(mask):
            raise ValueError("Requested slice does not intersect any cells")
        cell_points = cell_points[mask]
        values = values[mask]

    plt, poly_collection, _, _ = get_matplotlib_modules()
    projected = cell_points[:, :, keep_axes]
    polygons = [_project_polygon(polygon) for polygon in projected]
    mesh_linewidth = choose_mesh_linewidth(len(polygons), show_mesh)
    fig, ax = plt.subplots(figsize=(8, 8))
    collection = poly_collection(
        polygons,
        array=values,
        cmap=colormap,
        edgecolors="k" if show_mesh else "face",
        linewidths=mesh_linewidth,
    )
    if vmin is not None or vmax is not None:
        collection.set_clim(vmin=vmin, vmax=vmax)
    ax.add_collection(collection)
    points_2d = snapshot.points[:, keep_axes]
    ax.set_xlim(float(np.min(points_2d[:, 0])), float(np.max(points_2d[:, 0])))
    ax.set_ylim(float(np.min(points_2d[:, 1])), float(np.max(points_2d[:, 1])))
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel(axis_label(keep_axes[0]))
    ax.set_ylabel(axis_label(keep_axes[1]))
    ax.set_title(format_default_title(field_name, snapshot.time))
    fig.colorbar(collection, ax=ax)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)
    return output_path


def render_nodal_snapshot(
    snapshot: MeshSnapshot,
    values: np.ndarray,
    field_name: str,
    keep_axes: tuple[int, int],
    drop_axis: int | None,
    slice_position: float | None,
    slice_width: float,
    output_path: Path,
    colormap: str,
    dpi: int,
    vmin: float | None,
    vmax: float | None,
    show_mesh: bool,
) -> Path:
    cells = snapshot.cells
    cell_points = snapshot.points[cells]
    if drop_axis is not None:
        mins = np.min(cell_points[:, :, drop_axis], axis=1)
        maxs = np.max(cell_points[:, :, drop_axis], axis=1)
        if slice_position is None:
            slice_position = float(0.5 * (np.min(mins) + np.max(maxs)))
        band_low = slice_position - slice_width / 2.0
        band_high = slice_position + slice_width / 2.0
        mask = (mins <= band_high) & (maxs >= band_low)
        if not np.any(mask):
            raise ValueError("Requested slice does not intersect any cells")
        cells = cells[mask]
        cell_points = cell_points[mask]

    x_coords: list[float] = []
    y_coords: list[float] = []
    point_values: list[float] = []
    triangles: list[tuple[int, int, int]] = []
    mesh_segments: list[np.ndarray] = []

    for cell_indices, polygon_points in zip(cells, cell_points, strict=True):
        ordered_coords, ordered_values = order_projected_polygon(
            polygon_points[:, keep_axes], values[cell_indices]
        )
        if len(ordered_coords) < 3:
            continue
        start_index = len(x_coords)
        x_coords.extend(ordered_coords[:, 0].tolist())
        y_coords.extend(ordered_coords[:, 1].tolist())
        point_values.extend(ordered_values.tolist())
        for offset in range(1, len(ordered_coords) - 1):
            triangles.append(
                (start_index, start_index + offset, start_index + offset + 1)
            )
        if show_mesh:
            mesh_segments.append(np.vstack([ordered_coords, ordered_coords[0]]))

    if not triangles:
        raise ValueError("No polygons available for nodal rendering")

    plt, _, tri_module, line_collection = get_matplotlib_modules()
    triangulation = tri_module.Triangulation(x_coords, y_coords, np.asarray(triangles))
    fig, ax = plt.subplots(figsize=(8, 8))
    plot = ax.tripcolor(
        triangulation,
        np.asarray(point_values),
        shading="gouraud",
        cmap=colormap,
        vmin=vmin,
        vmax=vmax,
    )
    if show_mesh and mesh_segments:
        mesh_linewidth = choose_mesh_linewidth(len(mesh_segments), show_mesh)
        ax.add_collection(
            line_collection(mesh_segments, colors="k", linewidths=mesh_linewidth)
        )
    points_2d = snapshot.points[:, keep_axes]
    ax.set_xlim(float(np.min(points_2d[:, 0])), float(np.max(points_2d[:, 0])))
    ax.set_ylim(float(np.min(points_2d[:, 1])), float(np.max(points_2d[:, 1])))
    ax.set_aspect("equal", adjustable="box")
    ax.set_xlabel(axis_label(keep_axes[0]))
    ax.set_ylabel(axis_label(keep_axes[1]))
    ax.set_title(format_default_title(field_name, snapshot.time))
    fig.colorbar(plot, ax=ax)
    fig.tight_layout()
    fig.savefig(output_path, dpi=dpi)
    plt.close(fig)
    return output_path


def _project_polygon(points: np.ndarray) -> np.ndarray:
    rounded = np.round(points, 12)
    unique = np.unique(rounded, axis=0)
    if len(unique) <= 2:
        return unique
    center = np.mean(unique, axis=0)
    angles = np.arctan2(unique[:, 1] - center[1], unique[:, 0] - center[0])
    return unique[np.argsort(angles)]


def order_projected_polygon(
    points: np.ndarray, values: np.ndarray
) -> tuple[np.ndarray, np.ndarray]:
    rounded = np.round(points, 12)
    buckets: dict[tuple[float, float], list[float]] = {}
    for point, value in zip(rounded, values, strict=True):
        key = (float(point[0]), float(point[1]))
        buckets.setdefault(key, []).append(float(value))
    unique_points = np.asarray(list(buckets.keys()), dtype=float)
    unique_values = np.asarray(
        [sum(bucket) / len(bucket) for bucket in buckets.values()], dtype=float
    )
    if len(unique_points) <= 2:
        return unique_points, unique_values
    center = np.mean(unique_points, axis=0)
    angles = np.arctan2(
        unique_points[:, 1] - center[1], unique_points[:, 0] - center[0]
    )
    order = np.argsort(angles)
    return unique_points[order], unique_values[order]


def choose_mesh_linewidth(cell_count: int, show_mesh: bool) -> float:
    if not show_mesh:
        return 0.0
    if cell_count <= 10000:
        return 0.35
    if cell_count <= 50000:
        return 0.25
    if cell_count <= 150000:
        return 0.18
    return 0.12


def format_default_title(field_name: str, time_value: float | None) -> str:
    if time_value is None:
        return field_name
    return f"{field_name}, t={time_value:g}"


def create_parallel_executor(jobs: int) -> Executor:
    try:
        context = multiprocessing.get_context("fork")
        return ProcessPoolExecutor(max_workers=jobs, mp_context=context)
    except (ValueError, PermissionError, OSError):
        return ThreadPoolExecutor(max_workers=jobs)


def count_field_components(cell_data: dict[str, np.ndarray]) -> int:
    total = 0
    for values in cell_data.values():
        array = np.asarray(values)
        if array.ndim <= 1:
            total += 1
        else:
            total += array.shape[1]
    return total


def get_matplotlib_modules():
    import matplotlib

    matplotlib.use("Agg")
    from matplotlib import pyplot as plt
    from matplotlib.collections import LineCollection, PolyCollection
    from matplotlib import tri

    return plt, PolyCollection, tri, LineCollection


if __name__ == "__main__":
    main()
