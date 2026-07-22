#!/usr/bin/env python3
# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Unified renderer for ExaHyPE 2 / Peano 4 plotter output.

Two front-ends share the readers and the renderer:

  * a batch command line that writes image files for a single snapshot or a
    whole time series (see ``--help``), and
  * notebook viewers :func:`show_solution` (2D) and :func:`show_solution_3d`
    (axis-aligned slices through a cube), each with a time slider, a field
    selector and play/stop.
"""

from __future__ import annotations

import argparse
from concurrent.futures import ProcessPoolExecutor
from io import BytesIO
import math
import multiprocessing
import os
from pathlib import Path

import numpy as np

import matplotlib

# Render head-less when running as a plain script or worker process, but leave
# the backend untouched inside an IPython kernel, where forcing Agg would
# silently disable the notebook's own inline plotting.
try:
    get_ipython()  # type: ignore[name-defined]  # only defined inside IPython
except NameError:
    matplotlib.use("Agg", force=True)


# --------------------------------------------------------------------------- #
# Constants
# --------------------------------------------------------------------------- #
AXIS_TO_INDEX = {"x": 0, "y": 1, "z": 2}
AXIS_NAMES = ("x", "y", "z")
IMAGE_FORMATS = ("png", "pdf", "svg", "pgf")


# Fields the solvers always inject; they are kept but never chosen as a default
# and are pushed to the end of the field selector.
HOUSEKEEPING_FIELDS = ("Subdomain", "RefinementLevel")

# Regular Cartesian meshes above this many cells are too large to draw as
# polygons; the viewer requires the imshow fast path for them.
LARGE_MESH = 250_000

# Figure layout (image axes + colour-bar axes as left/bottom/width/height
# fractions) shared by every render path so a frame looks identical whoever drew
# it. Explicit rectangles instead of tight_layout keep this reproducible.
IMAGE_RECT = (0.04, 0.06, 0.74, 0.88)
CBAR_RECT = (0.80, 0.06, 0.03, 0.88)
LABELLED_IMAGE_RECT = (0.10, 0.09, 0.72, 0.84)
LABELLED_CBAR_RECT = (0.85, 0.09, 0.03, 0.84)

# The common mesh model and the per-format readers live in solution_io, shared with
# compare_solution. The dual import makes this work both as a package
# (postprocessing.solution_io) and when the script is run directly from the
# postprocessing/ directory (solution_io as a sibling module).
try:
    from postprocessing.solution_io import (
        MeshSnapshot,
        SnapshotRef,
        SolutionSource,
        load_snapshot_ref,
    )
except ImportError:
    from solution_io import (
        MeshSnapshot,
        SnapshotRef,
        SolutionSource,
        load_snapshot_ref,
    )


# --------------------------------------------------------------------------- #
# Field values
# --------------------------------------------------------------------------- #
def _component_or_magnitude(values: np.ndarray, component: int | None) -> np.ndarray:
    values = np.asarray(values)
    if values.ndim == 1:
        return values
    if values.shape[1] == 1:
        return values[:, 0]
    if component is not None:
        if component < 0 or component >= values.shape[1]:
            raise ValueError(
                f"Component {component} is out of range (0..{values.shape[1] - 1})"
            )
        return values[:, component]
    return np.linalg.norm(values, axis=1)


def cell_scalar(
    snapshot: MeshSnapshot, field: str, component: int | None = None
) -> np.ndarray:
    """One colour value per cell: the field component, or its magnitude.

    Node values are averaged onto their cell.
    """
    raw, center = snapshot.field(field)
    values = _component_or_magnitude(raw, component)
    if center == "cell":
        return np.asarray(values, dtype=float)
    cells = snapshot.cells
    if isinstance(cells, np.ndarray):
        return values[cells].mean(axis=1)
    return np.array([float(values[np.asarray(nodes)].mean()) for nodes in cells])


def field_label(field: str, component: int | None) -> str:
    return field if component is None else f"{field}[{component}]"


def field_token(field: str, component: int | None) -> str:
    return field if component is None else f"{field}-{component}"


def default_field(variables: list[str]) -> str:
    for name in variables:
        if name not in HOUSEKEEPING_FIELDS:
            return name
    return variables[0]


def ordered_field_options(variables: list[str]) -> list[str]:
    regular = [name for name in variables if name not in HOUSEKEEPING_FIELDS]
    housekeeping = [name for name in variables if name in HOUSEKEEPING_FIELDS]
    return regular + housekeeping


def _autoscale(values: np.ndarray, robust: bool) -> tuple[float, float]:
    finite = values[np.isfinite(values)]
    if finite.size == 0:
        return (0.0, 1.0)
    if robust:
        low, high = (float(value) for value in np.percentile(finite, [2.0, 98.0]))
    else:
        low, high = float(finite.min()), float(finite.max())
    return (low - 1.0e-9, high + 1.0e-9) if low == high else (low, high)


# --------------------------------------------------------------------------- #
# Regular Cartesian fast path
# --------------------------------------------------------------------------- #
def regular_grid(snapshot: MeshSnapshot) -> dict | None:
    """The imshow mapping if the 2D cells tile a regular Cartesian lattice."""
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray) or snapshot.points.shape[1] < 2:
        return None
    coordinates = snapshot.points[:, :2]
    centres = coordinates[cells].mean(axis=1)
    cx = np.round(centres[:, 0], 9)
    cy = np.round(centres[:, 1], 9)
    xs = np.unique(cx)
    ys = np.unique(cy)
    if len(xs) * len(ys) != len(centres):
        return None
    ix = np.searchsorted(xs, cx)
    iy = np.searchsorted(ys, cy)
    if np.unique(iy * len(xs) + ix).size != len(ix):
        return None
    extent = [
        float(coordinates[:, 0].min()),
        float(coordinates[:, 0].max()),
        float(coordinates[:, 1].min()),
        float(coordinates[:, 1].max()),
    ]
    return {"xs": xs, "ys": ys, "nx": len(xs), "ny": len(ys), "extent": extent}


def _grid_image(
    snapshot: MeshSnapshot, values: np.ndarray, grid: dict, target_px: int
) -> np.ndarray:
    centres = snapshot.points[:, :2][snapshot.cells].mean(axis=1)
    ix = np.searchsorted(grid["xs"], np.round(centres[:, 0], 9))
    iy = np.searchsorted(grid["ys"], np.round(centres[:, 1], 9))
    np.clip(ix, 0, grid["nx"] - 1, out=ix)
    np.clip(iy, 0, grid["ny"] - 1, out=iy)
    image = np.full((grid["ny"], grid["nx"]), np.nan)
    image[iy, ix] = values
    step = max(1, math.ceil(max(grid["nx"], grid["ny"]) / max(1, target_px)))
    return image[::step, ::step] if step > 1 else image


# --------------------------------------------------------------------------- #
# Polygon path (unstructured / adaptively refined / 3D slice)
# --------------------------------------------------------------------------- #
def _order_polygon(points_2d: np.ndarray) -> np.ndarray:
    rounded = np.round(points_2d, 12)
    unique = np.unique(rounded, axis=0)
    if len(unique) <= 2:
        return unique
    centre = unique.mean(axis=0)
    angles = np.arctan2(unique[:, 1] - centre[1], unique[:, 0] - centre[0])
    return unique[np.argsort(angles)]


def _order_polygons(cell_verts: np.ndarray) -> np.ndarray:
    centre = cell_verts.mean(axis=1, keepdims=True)
    angles = np.arctan2(
        cell_verts[:, :, 1] - centre[:, :, 1], cell_verts[:, :, 0] - centre[:, :, 0]
    )
    order = np.argsort(angles, axis=1)
    return np.take_along_axis(cell_verts, order[:, :, None], axis=1)


def _build_polygons(
    snapshot: MeshSnapshot,
    values: np.ndarray,
    keep_axes: tuple[int, int],
    drop_axis: int | None,
    slice_position: float | None,
    slice_width: float,
) -> tuple[list[np.ndarray], np.ndarray]:
    points = snapshot.points
    cells = snapshot.cells
    band_low = band_high = None
    if drop_axis is not None:
        if slice_position is None:
            column = points[:, drop_axis]
            slice_position = float(0.5 * (column.min() + column.max()))
        band_low = slice_position - slice_width / 2.0
        band_high = slice_position + slice_width / 2.0

    keep = list(keep_axes)
    if isinstance(cells, np.ndarray):
        cell_points = points[cells]
        if drop_axis is not None:
            column = cell_points[:, :, drop_axis]
            mask = (column.min(axis=1) <= band_high) & (column.max(axis=1) >= band_low)
            cell_points = cell_points[mask]
            colours = values[mask]
        else:
            colours = values
        ordered = _order_polygons(cell_points[:, :, keep])
        return list(ordered), np.asarray(colours, dtype=float)

    # Ragged mesh (cells differ in corner count): fall back to the per-cell path.
    polygons: list[np.ndarray] = []
    colours_list: list[float] = []
    for index, nodes in enumerate(cells):
        cell_points = points[np.asarray(nodes)]
        if drop_axis is not None:
            low = float(cell_points[:, drop_axis].min())
            high = float(cell_points[:, drop_axis].max())
            if low > band_high or high < band_low:
                continue
        polygons.append(_order_polygon(cell_points[:, keep]))
        colours_list.append(values[index])
    return polygons, np.asarray(colours_list, dtype=float)


def _planar_drop_axis(points: np.ndarray) -> int | None:
    """The near-flat axis of a 3D mesh embedded in a plane, else None."""
    spans = np.ptp(points, axis=0)
    min_axis = int(np.argmin(spans))
    max_span = float(np.max(spans))
    if max_span == 0.0:
        raise ValueError("Mesh has zero extent in every dimension")
    if spans[min_axis] <= max(1.0e-12, 0.05 * max_span):
        return min_axis
    return None


def choose_mesh_linewidth(cell_count: int) -> float:
    if cell_count <= 10_000:
        return 0.35
    if cell_count <= 50_000:
        return 0.25
    if cell_count <= 150_000:
        return 0.18
    return 0.12


# --------------------------------------------------------------------------- #
# Figure construction
# --------------------------------------------------------------------------- #
def _new_figure(figsize, dpi):
    """A figure rendered by Agg regardless of the active backend.

    The figure stays outside pyplot's manager so nothing auto-displays at the end
    of a notebook cell.
    """
    from matplotlib.backends.backend_agg import FigureCanvasAgg
    from matplotlib.figure import Figure

    figure = Figure(figsize=figsize, dpi=dpi)
    FigureCanvasAgg(figure)
    return figure


def _projection(
    points: np.ndarray, slice_axis: str | None
) -> tuple[tuple[int, int], int | None]:
    dimension = points.shape[1]
    if dimension < 3:
        return (0, 1), None
    if slice_axis is not None:
        drop_axis = AXIS_TO_INDEX[slice_axis]
    else:
        drop_axis = _planar_drop_axis(points)
        if drop_axis is None:
            drop_axis = dimension - 1
    keep_axes = tuple(axis for axis in range(dimension) if axis != drop_axis)
    return keep_axes, drop_axis


def render_field_figure(
    snapshot: MeshSnapshot,
    field: str,
    *,
    component: int | None = None,
    cmap: str = "viridis",
    clim: tuple[float, float] | None = None,
    robust: bool = False,
    show_edges: bool = True,
    slice_axis: str | None = None,
    slice_position: float | None = None,
    slice_width: float = 0.0,
    title: str | None = None,
    labelled: bool = True,
    figsize: tuple[float, float] = (7.2, 6.0),
    dpi: int = 150,
    target_px: int = 2400,
):
    """Render one field of one snapshot onto a fresh figure and return it."""
    values = cell_scalar(snapshot, field, component)
    vmin, vmax = clim if clim is not None else _autoscale(values, robust)
    keep_axes, drop_axis = _projection(snapshot.points, slice_axis)

    figure = _new_figure(figsize, dpi)
    image_rect = LABELLED_IMAGE_RECT if labelled else IMAGE_RECT
    cbar_rect = LABELLED_CBAR_RECT if labelled else CBAR_RECT
    axes = figure.add_axes(image_rect)
    cbar_axes = figure.add_axes(cbar_rect)

    grid = regular_grid(snapshot) if drop_axis is None else None
    if grid is not None:
        image = _grid_image(snapshot, values, grid, target_px)
        artist = axes.imshow(
            image,
            origin="lower",
            extent=grid["extent"],
            aspect="equal",
            cmap=cmap,
            interpolation="nearest",
        )
        artist.set_clim(vmin, vmax)
        figure.colorbar(artist, cax=cbar_axes).set_label(field_label(field, component))
    else:
        from matplotlib.collections import PolyCollection

        polygons, colours = _build_polygons(
            snapshot, values, keep_axes, drop_axis, slice_position, slice_width
        )
        if not polygons:
            raise ValueError("The requested slice does not intersect any cells")
        linewidth = choose_mesh_linewidth(len(polygons)) if show_edges else 0.0
        collection = PolyCollection(
            polygons,
            array=colours,
            cmap=cmap,
            edgecolors="black" if show_edges else "face",
            linewidths=linewidth,
        )
        collection.set_clim(vmin, vmax)
        axes.add_collection(collection)
        axes.margins(0)
        axes.autoscale_view()
        axes.set_aspect("equal", adjustable="box")
        figure.colorbar(collection, cax=cbar_axes).set_label(
            field_label(field, component)
        )

    if labelled:
        axes.set_xlabel(AXIS_NAMES[keep_axes[0]])
        axes.set_ylabel(AXIS_NAMES[keep_axes[1]])
        axes.set_title(
            title
            if title is not None
            else _default_title(field, component, snapshot.time)
        )
    else:
        axes.set_axis_off()
    return figure


def _default_title(field: str, component: int | None, time: float | None) -> str:
    label = field_label(field, component)
    if time is None or (isinstance(time, float) and math.isnan(time)):
        return label
    return f"{label}, t = {time:g}"


def _figure_to_png(figure) -> bytes:
    buffer = BytesIO()
    figure.savefig(buffer, format="png", facecolor="white")
    return buffer.getvalue()


# --------------------------------------------------------------------------- #
# 3D slicing (axis-aligned planes through a uniform hexahedral mesh)
# --------------------------------------------------------------------------- #
def _cell_centres_3d(snapshot: MeshSnapshot) -> np.ndarray:
    cells = snapshot.cells
    if not isinstance(cells, np.ndarray):
        raise ValueError("3D slicing needs a uniform (hexahedral) mesh")
    if snapshot.points.shape[1] < 3:
        raise ValueError("This snapshot is not three-dimensional")
    return snapshot.points[cells].mean(axis=1)


def slice_layers(snapshot: MeshSnapshot, axis: str) -> np.ndarray:
    return np.unique(np.round(_cell_centres_3d(snapshot)[:, AXIS_TO_INDEX[axis]], 9))


def render_slice_figure(
    snapshot: MeshSnapshot,
    field: str,
    axis: str,
    layer_index: int,
    *,
    component: int | None = None,
    cmap: str = "coolwarm",
    clim: tuple[float, float] | None = None,
    title: str | None = None,
    figsize: tuple[float, float] = (6.4, 4.8),
    dpi: int = 150,
):
    a = AXIS_TO_INDEX[axis]
    b0, b1 = [index for index in (0, 1, 2) if index != a]
    layers = slice_layers(snapshot, axis)
    layer_index = int(np.clip(layer_index, 0, len(layers) - 1))
    centres = _cell_centres_3d(snapshot)
    mask = np.round(centres[:, a], 9) == layers[layer_index]
    values = cell_scalar(snapshot, field, component)[mask]
    p0 = np.round(centres[mask, b0], 9)
    p1 = np.round(centres[mask, b1], 9)
    xs, ys = np.unique(p0), np.unique(p1)
    image = np.full((len(ys), len(xs)), np.nan)
    image[np.searchsorted(ys, p1), np.searchsorted(xs, p0)] = values
    extent = [float(xs.min()), float(xs.max()), float(ys.min()), float(ys.max())]

    if clim is not None:
        vmin, vmax = clim
    else:
        vmin, vmax = _autoscale(values, robust=False)
    figure = _new_figure(figsize, dpi)
    axes = figure.add_axes([0.10, 0.10, 0.74, 0.82])
    interpolation = "nearest" if np.isnan(image).any() else "bicubic"
    artist = axes.imshow(
        image,
        origin="lower",
        extent=extent,
        cmap=cmap,
        vmin=vmin,
        vmax=vmax,
        aspect="equal",
        interpolation=interpolation,
    )
    axes.set_xlabel(AXIS_NAMES[b0])
    axes.set_ylabel(AXIS_NAMES[b1])
    if title:
        axes.set_title(title)
    figure.colorbar(artist, cax=figure.add_axes([0.86, 0.10, 0.03, 0.82])).set_label(
        field_label(field, component)
    )
    return figure


# --------------------------------------------------------------------------- #
# Batch command line
# --------------------------------------------------------------------------- #
def build_parser() -> argparse.ArgumentParser:
    default_jobs = max(1, os.cpu_count() or 1)
    parser = argparse.ArgumentParser(
        description="Render ExaHyPE 2 / Peano 4 plotter output (VTKHDF, HDF5/XDMF, "
        "NetCDF/UGRID or VTU) to image files."
    )
    parser.add_argument(
        "input",
        help="A solver name (discovered under ./ and its sub-folders), an index/snapshot file, "
        "or an output directory",
    )
    parser.add_argument("--output-dir", default=".", help="Where to write images")
    parser.add_argument(
        "--format", choices=IMAGE_FORMATS, default="png", help="Image format"
    )
    parser.add_argument(
        "--field", help="Field to render (default: first non-housekeeping)"
    )
    parser.add_argument("--component", type=int, help="Component of a vector field")
    parser.add_argument("--colormap", default="viridis", help="Matplotlib colormap")
    parser.add_argument("--vmin", type=float, help="Lower colour limit")
    parser.add_argument("--vmax", type=float, help="Upper colour limit")
    parser.add_argument(
        "--robust", action="store_true", help="Clip colours to the 2-98th percentile"
    )
    parser.add_argument("--dpi", type=int, default=300, help="DPI for raster outputs")
    parser.add_argument(
        "--no-edges", dest="show_edges", action="store_false", help="Hide mesh edges"
    )
    parser.add_argument(
        "--slice-axis",
        choices=tuple(AXIS_TO_INDEX),
        help="Slice a 3D mesh along an axis",
    )
    parser.add_argument(
        "--slice-position", type=float, help="Slice position along the slice axis"
    )
    parser.add_argument(
        "--slice-width", type=float, default=0.0, help="Slice band width"
    )
    parser.add_argument(
        "--frame", type=int, help="Render only this frame index (default: all frames)"
    )
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        nargs="?",
        const=default_jobs,
        default=1,
        help="Worker processes for multi-frame inputs (bare -j uses all cores)",
    )
    parser.add_argument(
        "--animation",
        choices=("gif", "mp4"),
        help="Assemble PNG frames into an animation",
    )
    return parser


def main(argv: list[str] | None = None) -> None:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.animation and args.format != "png":
        parser.error(
            f"--animation needs --format png (got {args.format!r}); "
            "vector formats cannot be assembled into an animation"
        )
    source = SolutionSource.locate(args.input)
    field = args.field or default_field(source.variables)
    first = source.snapshot(0)
    action = "Reading" if Path(args.input).exists() else "Discovered"
    print(f"{action} {args.input!r}: {source.describe()}")
    print(
        f"  Mesh: {first.points.shape[0]} points, {first.num_cells} cells, "
        f"{first.points.shape[1]}D; fields: {', '.join(source.variables)}"
    )
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.frame is not None:
        frame = args.frame + source.count if args.frame < 0 else args.frame
        if not 0 <= frame < source.count:
            parser.error(
                f"--frame {args.frame} is out of range: this source has "
                f"{source.count} frame(s) (valid indices 0..{source.count - 1})"
            )
        indices = [frame]
    else:
        indices = list(range(source.count))
    clim = None if args.vmin is None and args.vmax is None else (args.vmin, args.vmax)
    options = {
        "field": field,
        "component": args.component,
        "cmap": args.colormap,
        "clim": clim,
        "robust": args.robust,
        "show_edges": args.show_edges,
        "slice_axis": args.slice_axis,
        "slice_position": args.slice_position,
        "slice_width": args.slice_width,
        "dpi": args.dpi,
        "image_format": args.format,
        "output_dir": str(output_dir),
        "stem": source.name,
        "total": len(indices),
    }

    workers = f", {args.jobs} workers" if args.jobs > 1 and len(indices) > 1 else ""
    slice_note = f", slice across {args.slice_axis}" if args.slice_axis else ""
    print(
        f'Rendering field "{field_label(field, args.component)}"{slice_note} '
        f"-> {output_dir}/  ({len(indices)} frame{'' if len(indices) == 1 else 's'}, "
        f"{args.format}, {args.dpi} dpi{workers})"
    )

    if args.jobs > 1 and len(indices) > 1:
        work = [(source.refs[index], index, options) for index in indices]
        with _process_pool(args.jobs) as pool:
            frame_paths = list(pool.map(_render_ref_worker, work))
    else:
        frame_paths = [
            _render_and_save(source.load(index), index, options) for index in indices
        ]

    print(
        f"Wrote {len(frame_paths)} image{'' if len(frame_paths) == 1 else 's'} to {output_dir}/"
    )
    if args.animation:
        stem = f"{source.name}-{field_token(field, args.component)}"
        target = _save_animation(frame_paths, output_dir, stem, args.animation)
        print(f"Wrote animation -> {target}")


def _render_and_save(snapshot: MeshSnapshot, index: int, options: dict) -> Path:
    figure = render_field_figure(
        snapshot,
        options["field"],
        component=options["component"],
        cmap=options["cmap"],
        clim=options["clim"],
        robust=options["robust"],
        show_edges=options["show_edges"],
        slice_axis=options["slice_axis"],
        slice_position=options["slice_position"],
        slice_width=options["slice_width"],
        dpi=options["dpi"],
        labelled=True,
    )
    token = field_token(options["field"], options["component"])
    output_path = (
        Path(options["output_dir"])
        / f"{options['stem']}-{index:04d}-{token}.{options['image_format']}"
    )
    figure.savefig(output_path, dpi=options["dpi"])
    time = snapshot.time
    time_note = (
        ""
        if time is None or (isinstance(time, float) and math.isnan(time))
        else f", t = {time:g}"
    )
    total = options.get("total")
    position = f"{index + 1}/{total}" if total else f"{index + 1}"
    print(
        f"  Frame {position}{time_note}: {snapshot.num_cells} cells -> {output_path.name}",
        flush=True,
    )
    return output_path


def _render_ref_worker(work: tuple[SnapshotRef, int, dict]) -> Path:
    ref, index, options = work
    return _render_and_save(load_snapshot_ref(ref), index, options)


def _process_pool(jobs: int):
    try:
        context = multiprocessing.get_context("fork")
    except ValueError:
        context = multiprocessing.get_context()
    return ProcessPoolExecutor(max_workers=jobs, mp_context=context)


# --------------------------------------------------------------------------- #
# Parallel frame prefetch for the interactive viewers
# --------------------------------------------------------------------------- #
def _run_prefetch(tasks, worker, store, progress=None):
    if not tasks:
        return
    workers = min(os.cpu_count() or 1, 8, len(tasks))
    if workers > 1:
        try:
            with _process_pool(workers) as pool:
                for done, (index, png) in enumerate(pool.map(worker, tasks), start=1):
                    store(index, png)
                    if progress is not None:
                        progress(done, len(tasks))
            return
        except Exception:
            pass  # a broken pool must never break the viewer; render inline instead
    for done, task in enumerate(tasks, start=1):
        index, png = worker(task)
        store(index, png)
        if progress is not None:
            progress(done, len(tasks))


def _prefetch_worker(task: tuple[SnapshotRef, int, dict]) -> tuple[int, bytes]:
    ref, index, options = task
    figure = render_field_figure(load_snapshot_ref(ref), **options)
    return index, _figure_to_png(figure)


def _prefetch_slice_worker(task: tuple[SnapshotRef, int, dict]) -> tuple[int, bytes]:
    ref, index, options = task
    figure = render_slice_figure(
        load_snapshot_ref(ref),
        options["field"],
        options["axis"],
        options["layer"],
        cmap=options["cmap"],
        clim=options["clim"],
        title=options.get("title"),
        figsize=options["figsize"],
        dpi=options["dpi"],
    )
    return index, _figure_to_png(figure)


def _prefetch_into_cache(source, name, options, cache, indices, progress=None):
    tasks = [(source.refs[i], i, options) for i in indices if (name, i) not in cache]
    _run_prefetch(
        tasks,
        _prefetch_worker,
        lambda i, png: cache.__setitem__((name, i), png),
        progress,
    )


def _save_animation(
    frame_paths: list[Path], output_dir: Path, stem: str, kind: str, fps: int = 8
) -> Path:
    import imageio.v2 as imageio

    frames = [imageio.imread(path) for path in frame_paths]
    target = output_dir / f"{stem}.{kind}"
    if kind == "gif":
        imageio.mimsave(target, frames, fps=fps)
    else:
        imageio.mimsave(target, frames, fps=fps, codec="libx264")
    return target


# --------------------------------------------------------------------------- #
# Single-frame helpers for notebooks
# --------------------------------------------------------------------------- #
def render_solution_image(
    source: str | Path | SolutionSource,
    field: str | None = None,
    *,
    index: int = -1,
    component: int | None = None,
    cmap: str = "coolwarm",
    show_edges: bool = False,
    clim: tuple[float, float] | None = None,
    labelled: bool = False,
    width: int = 720,
) -> bytes:
    """Render one snapshot to PNG bytes."""
    # Open path-like inputs; anything else is assumed to be a SolutionSource already. A plain
    # isinstance check would reject a SolutionSource imported through solution_io's sibling-module
    # path (a distinct class object from this module's postprocessing.solution_io.SolutionSource).
    if isinstance(source, (str, os.PathLike)):
        source = SolutionSource.open(source)
    snapshot = source.snapshot(index % source.count)
    chosen = field if field in source.variables else default_field(source.variables)
    figure = render_field_figure(
        snapshot,
        chosen,
        component=component,
        cmap=cmap,
        clim=clim,
        show_edges=show_edges,
        labelled=labelled,
        figsize=(width / 100.0, width / 130.0),
        dpi=130,
    )
    return _figure_to_png(figure)


def plot_solution(source, field=None, **kwargs):
    """Show a single snapshot inline in a notebook."""
    png = render_solution_image(source, field, **kwargs)
    try:
        from IPython.display import Image
    except ImportError:
        return png
    return Image(data=png)


def _global_clim(
    source: SolutionSource, field: str, component: int | None
) -> tuple[float, float]:
    low, high = math.inf, -math.inf
    for index in range(source.count):
        values = cell_scalar(source.snapshot(index), field, component)
        finite = values[np.isfinite(values)]
        if finite.size:
            low = min(low, float(finite.min()))
            high = max(high, float(finite.max()))
    if not math.isfinite(low):
        return (0.0, 1.0)
    return (low, high) if low != high else (low - 1.0e-9, high + 1.0e-9)


# --------------------------------------------------------------------------- #
# Interactive 2D viewer
# --------------------------------------------------------------------------- #
def show_solution(
    solver_name: str,
    field: str | None = None,
    directory: str | Path | None = None,
    *,
    cmap: str = "coolwarm",
    show_edges: bool = False,
    play_interval: int = 200,
    width: int = 900,
    normalise: str = "frame",
    clim: tuple[float, float] | None = None,
    robust: bool = False,
):
    """Interactive viewer with time, frame and field selectors plus play/stop.

    ``normalise="global"`` holds the colour scale fixed across time;
    ``robust=True`` or ``clim=(lo, hi)`` tames outliers. Outside a notebook the
    last frame is rendered and returned as PNG bytes.
    """
    import asyncio

    source = _as_source(solver_name, directory)
    times = source.times
    last = source.count - 1
    variables = ordered_field_options(source.variables)
    chosen = field if field in variables else default_field(variables)

    first = source.snapshot(0)
    if first.points.shape[1] >= 3 and regular_grid(first) is None:
        raise ValueError(
            "This looks like a 3D mesh. Use show_solution_3d() to slice through it."
        )
    if regular_grid(first) is None and first.num_cells > LARGE_MESH:
        raise ValueError(
            f"This mesh has {first.num_cells:,} cells and is not a regular Cartesian "
            "grid, so it cannot be drawn efficiently. Use a coarser mesh or fewer snapshots."
        )

    cache: dict[tuple[str, int], bytes] = {}
    clim_cache: dict[str, tuple[float, float]] = {}

    def clim_for(name: str) -> tuple[float, float] | None:
        if clim is not None:
            return clim
        if normalise != "global":
            return None
        if name not in clim_cache:
            clim_cache[name] = _global_clim(source, name, None)
        return clim_cache[name]

    def options_for(name: str) -> dict:
        return {
            "field": name,
            "cmap": cmap,
            "clim": clim_for(name),
            "robust": robust,
            "show_edges": show_edges,
            "labelled": False,
            "figsize": (width / 100.0, width / 130.0),
            "dpi": 130,
        }

    def render(name: str, index: int) -> bytes:
        key = (name, index)
        if key not in cache:
            figure = render_field_figure(source.snapshot(index), **options_for(name))
            cache[key] = _figure_to_png(figure)
        return cache[key]

    try:
        import ipywidgets as widgets
        from IPython.display import display
    except ImportError:
        return render(chosen, last)

    do_prefetch = source.count <= 400

    slider = widgets.IntSlider(
        value=0,
        min=0,
        max=last,
        step=1,
        description="step",
        readout=False,
        continuous_update=do_prefetch,
        layout=widgets.Layout(width=f"{max(200, width - 120)}px"),
    )
    frame_dropdown = widgets.Dropdown(
        options=[
            (f"frame {i}  (t = {_format_time(times[i])})", i)
            for i in range(source.count)
        ],
        value=0,
        description="frame",
    )
    field_dropdown = widgets.Dropdown(
        options=variables, value=chosen, description="field"
    )
    play_button = widgets.Button(
        description="▶ Play", layout=widgets.Layout(width="80px")
    )
    stop_button = widgets.Button(
        description="■ Stop", disabled=True, layout=widgets.Layout(width="80px")
    )
    status = widgets.HTML()
    image = widgets.Image(
        format="png", layout=widgets.Layout(width=f"{width}px", height="auto")
    )
    state = {"syncing": False, "playing": False}
    prefetched: set[str] = set()

    def status_line(index: int, name: str):
        status.value = (
            f"Frame <b>{index}</b> / {last} &nbsp;&bull;&nbsp; t = {_format_time(times[index])} "
            f"&nbsp;&bull;&nbsp; field = <b>{name}</b>"
        )

    def set_busy(busy: bool):
        for control in (slider, frame_dropdown, field_dropdown, play_button):
            control.disabled = busy
        image.layout.opacity = "0.5" if busy else "1"

    def prefetch(name: str):
        if not do_prefetch or name in prefetched:
            return
        pending = [i for i in range(source.count) if (name, i) not in cache]
        if not pending:
            prefetched.add(name)
            return
        set_busy(True)
        status.value = f"<i>Prefetching {len(pending)} frames of <b>{name}</b> …</i>"
        try:
            _prefetch_into_cache(
                source,
                name,
                options_for(name),
                cache,
                pending,
                progress=lambda done, total: setattr(
                    status,
                    "value",
                    f"<i>Prefetching <b>{name}</b>: {done}/{total} frames (parallel) …</i>",
                ),
            )
            prefetched.add(name)
        finally:
            set_busy(False)
            status_line(slider.value, name)

    def show(index: int, name: str):
        if (name, index) not in cache:
            status.value = f"<i>Rendering frame {index} …</i>"
        image.value = render(name, index)
        status_line(index, name)

    def goto(index: int):
        if state["syncing"]:
            return
        state["syncing"] = True
        try:
            slider.value = index
            frame_dropdown.value = index
        finally:
            state["syncing"] = False
        show(index, field_dropdown.value)

    def set_playing(playing: bool):
        play_button.disabled = playing
        stop_button.disabled = not playing
        for control in (slider, frame_dropdown, field_dropdown):
            control.disabled = playing

    async def play_loop():
        name = field_dropdown.value
        index = slider.value if slider.value < last else 0
        while index <= last and state["playing"]:
            image.value = render(name, index)
            state["syncing"] = True
            slider.value = index
            frame_dropdown.value = index
            state["syncing"] = False
            status_line(index, name)
            await asyncio.sleep(play_interval / 1000.0)
            index += 1
        state["playing"] = False
        set_playing(False)

    def on_play(_):
        if state["playing"]:
            return
        state["playing"] = True
        set_playing(True)
        try:
            asyncio.get_running_loop().create_task(play_loop())
        except RuntimeError:
            asyncio.ensure_future(play_loop())

    def on_field(change):
        prefetch(
            change["new"]
        )  # warm the newly selected field, then jump to its first frame
        goto(0)

    slider.observe(lambda change: goto(change["new"]), names="value")
    frame_dropdown.observe(lambda change: goto(change["new"]), names="value")
    field_dropdown.observe(on_field, names="value")
    play_button.on_click(on_play)
    stop_button.on_click(lambda _: state.update(playing=False))

    display(
        widgets.VBox(
            [
                field_dropdown,
                widgets.HBox([play_button, stop_button, slider, frame_dropdown]),
                status,
                image,
            ]
        )
    )
    show(0, chosen)  # first frame immediately, then warm the rest in the background
    prefetch(chosen)
    return None


# --------------------------------------------------------------------------- #
# Interactive 3D slice viewer
# --------------------------------------------------------------------------- #
def show_solution_3d(
    solver_name: str,
    field: str | None = None,
    directory: str | Path | None = None,
    *,
    axis: str = "z",
    cmap: str = "coolwarm",
    width: int = 720,
    play_interval: int = 200,
    normalise: str = "frame",
):
    """Interactive slice viewer for a 3D solution.

    Pick the slice axis (x/y/z), move the slice slider through the cube, pick a
    field and animate over time. Outside a notebook the mid-plane slice of the
    last frame is returned as PNG bytes.
    """
    import asyncio

    source = _as_source(solver_name, directory)
    times = source.times
    last = source.count - 1
    base = source.snapshot(0)
    variables = ordered_field_options(base.variables)
    chosen = field if field in variables else default_field(variables)

    clim_cache: dict[str, tuple[float, float]] = {}

    def clim_for(name: str) -> tuple[float, float] | None:
        if normalise != "global":
            return None
        if name not in clim_cache:
            clim_cache[name] = _global_clim(source, name, None)
        return clim_cache[name]

    cache: dict[tuple, bytes] = {}

    def render(name: str, slice_axis: str, layer: int, index: int) -> bytes:
        key = (name, slice_axis, layer, index)
        if key not in cache:
            snapshot = source.snapshot(index)
            layers = slice_layers(snapshot, slice_axis)
            position = layers[int(np.clip(layer, 0, len(layers) - 1))]
            title = (
                f"{slice_axis} = {position:.3g}   (t = {_format_time(times[index])})"
            )
            figure = render_slice_figure(
                snapshot,
                name,
                slice_axis,
                layer,
                cmap=cmap,
                clim=clim_for(name),
                title=title,
                figsize=(width / 110.0, width / 150.0),
                dpi=130,
            )
            cache[key] = _figure_to_png(figure)
        return cache[key]

    try:
        import ipywidgets as widgets
        from IPython.display import display
    except ImportError:
        layers = slice_layers(base, axis)
        return render(chosen, axis, len(layers) // 2, last)

    layers = slice_layers(base, axis)
    field_dropdown = widgets.Dropdown(
        options=variables, value=chosen, description="field"
    )
    axis_dropdown = widgets.Dropdown(
        options=list(AXIS_TO_INDEX), value=axis, description="slice ⊥"
    )
    slice_slider = widgets.IntSlider(
        value=len(layers) // 2,
        min=0,
        max=len(layers) - 1,
        step=1,
        description="slice",
        continuous_update=False,
    )
    time_slider = widgets.IntSlider(
        value=0,
        min=0,
        max=last,
        step=1,
        description="time",
        readout=False,
        layout=widgets.Layout(width=f"{max(200, width - 120)}px"),
    )
    play_button = widgets.Button(
        description="▶ Play", layout=widgets.Layout(width="80px")
    )
    stop_button = widgets.Button(
        description="■ Stop", disabled=True, layout=widgets.Layout(width="80px")
    )
    status = widgets.HTML()
    image = widgets.Image(
        format="png", layout=widgets.Layout(width=f"{width}px", height="auto")
    )
    state = {"playing": False}
    prefetched: set = set()  # (field, axis, layer) already warmed across time

    def set_busy(busy: bool):
        for control in (
            field_dropdown,
            axis_dropdown,
            slice_slider,
            time_slider,
            play_button,
        ):
            control.disabled = busy
        image.layout.opacity = "0.5" if busy else "1"

    def draw():
        layers_now = slice_layers(base, axis_dropdown.value)
        layer = int(np.clip(slice_slider.value, 0, len(layers_now) - 1))
        image.value = render(
            field_dropdown.value, axis_dropdown.value, layer, time_slider.value
        )
        status.value = (
            f"frame <b>{time_slider.value}</b> / {last} &nbsp;&bull;&nbsp; "
            f"t = {_format_time(times[time_slider.value])} &nbsp;&bull;&nbsp; "
            f"field = <b>{field_dropdown.value}</b> &nbsp;&bull;&nbsp; "
            f"slice {axis_dropdown.value} = {layers_now[layer]:.3g}"
        )

    def prefetch():
        if state["playing"]:
            return
        name, slice_axis = field_dropdown.value, axis_dropdown.value
        layers_now = slice_layers(base, slice_axis)
        layer = int(np.clip(slice_slider.value, 0, len(layers_now) - 1))
        if (name, slice_axis, layer) in prefetched:
            return
        pending = [
            i for i in range(source.count) if (name, slice_axis, layer, i) not in cache
        ]
        if not pending:
            prefetched.add((name, slice_axis, layer))
            return
        position = layers_now[layer]
        options = {
            "field": name,
            "axis": slice_axis,
            "layer": layer,
            "cmap": cmap,
            "clim": clim_for(name),
            "figsize": (width / 110.0, width / 150.0),
            "dpi": 130,
        }
        tasks = [
            (
                source.refs[i],
                i,
                {
                    **options,
                    "title": f"{slice_axis} = {position:.3g}   (t = {_format_time(times[i])})",
                },
            )
            for i in pending
        ]
        set_busy(True)
        try:
            _run_prefetch(
                tasks,
                _prefetch_slice_worker,
                lambda i, png: cache.__setitem__((name, slice_axis, layer, i), png),
                progress=lambda done, total: setattr(
                    status,
                    "value",
                    f"<i>Prefetching <b>{name}</b> ({slice_axis} = {position:.3g}): "
                    f"{done}/{total} frames (parallel) …</i>",
                ),
            )
            prefetched.add((name, slice_axis, layer))
        finally:
            set_busy(False)
            draw()  # restore the status line after the progress messages

    def refresh():
        draw()
        prefetch()

    def on_axis(change):
        layers_now = slice_layers(base, change["new"])
        slice_slider.max = len(layers_now) - 1
        refresh()

    async def play_loop():
        index = time_slider.value if time_slider.value < last else 0
        while index <= last and state["playing"]:
            time_slider.value = index
            draw()
            await asyncio.sleep(play_interval / 1000.0)
            index += 1
        state["playing"] = False
        play_button.disabled = False
        stop_button.disabled = True

    def on_play(_):
        if state["playing"]:
            return
        state["playing"] = True
        play_button.disabled = True
        stop_button.disabled = False
        try:
            asyncio.get_running_loop().create_task(play_loop())
        except RuntimeError:
            asyncio.ensure_future(play_loop())

    time_slider.observe(
        lambda change: None if state["playing"] else draw(), names="value"
    )
    slice_slider.observe(
        lambda change: None if state["playing"] else refresh(), names="value"
    )
    field_dropdown.observe(
        lambda change: None if state["playing"] else refresh(), names="value"
    )
    axis_dropdown.observe(on_axis, names="value")
    play_button.on_click(on_play)
    stop_button.on_click(lambda _: state.update(playing=False))

    display(
        widgets.VBox(
            [
                widgets.HBox([field_dropdown, axis_dropdown, slice_slider]),
                widgets.HBox([play_button, stop_button, time_slider]),
                status,
                image,
            ]
        )
    )
    refresh()
    return None


# --------------------------------------------------------------------------- #
# Shared helpers
# --------------------------------------------------------------------------- #
def _as_source(solver_name, directory) -> SolutionSource:
    return SolutionSource.locate(solver_name, directory)


def _format_time(time: float | None) -> str:
    if time is None or (isinstance(time, float) and math.isnan(time)):
        return "?"
    return f"{time:g}"


if __name__ == "__main__":
    main()
